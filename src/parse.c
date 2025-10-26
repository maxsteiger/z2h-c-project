#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"
#include "parse.h"

int create_db_header(struct dbheader_t **headerOut) {
    if (headerOut == NULL) {
        printf("Illegal header pointer\n");
        return STATUS_ERROR;
    }
    // allocate & init memory the size of 1 member of struct dbheader_t which allows us to return the pointer to allocated space
    struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));
    if (header == NULL) {
        perror("calloc");
        printf("Malloc failed to create db header\n");
        return STATUS_ERROR;
    }

    header->version = 0x1;
    header->count = 0;
    header->magic = HEADER_MAGIC;
    header->filesize = sizeof(struct dbheader_t);

    *headerOut = header;
    return STATUS_SUCCESS;
}

int validate_db_header(int fd, struct dbheader_t **headerOut) {
    if (fd < 0) {
        printf("Bad file descriptor\n");
        return STATUS_ERROR;
    }

    struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));
    if (header == NULL) {
        perror("calloc");
        printf("Malloc failed to create db header\n");
        return STATUS_ERROR;
    }

    if (read(fd, header, sizeof(struct dbheader_t)) != sizeof(struct dbheader_t)) {
        perror("read");
        return STATUS_ERROR;
    }

    // reading values from disk & use "ntoh" to ensure correct conversion between host and network byte order
    header->version = ntohs(header->version);
    header->count = ntohs(header->count);
    header->magic = ntohl(header->magic);
    header->filesize = ntohl(header->filesize);

    if (header->version != 1) {
        printf("Improper header version\n");
        return STATUS_ERROR;
    }

    if (header->magic != HEADER_MAGIC) {
        printf("Improper header magic\n");
        return STATUS_ERROR;
    }

    struct stat dbstat = {0};
    fstat(fd, &dbstat);
    if (header->filesize != dbstat.st_size) {
        printf("Corrupt database\n");
        return STATUS_ERROR;
    }

    *headerOut = header;
    return STATUS_SUCCESS;
}
// *dbhdr         -> pointer to populated db header struct
// **employeesOut -> double pointer to a list of employees (employee struct)
int add_employee(struct dbheader_t *dbhdr, struct employee_t **employeesOut, char *addstring) {
    if (dbhdr == NULL) {
        printf("Illegal Header\n");
        return STATUS_ERROR;
    }
    if (employeesOut == NULL || *employeesOut == NULL) {
        printf("No Employees\n");
        return STATUS_ERROR;
    }
    if (!addstring) {
        printf("Somehow addstring still empty\n");
        return STATUS_ERROR;
    }

    char *delims = addstring;
    int count_delims = 0;

    while ((delims = strpbrk(delims, DELIMITER)) != NULL) {
        count_delims++; // count amount of delimiters in input string
        delims++;       // move to next character
    }

    if (count_delims != 2) {
        printf("Incorrect employee format to parse!\n");
        printf("Expected format: \"Name%1$sAddress%1$sWorkHours\"\n", DELIMITER);
        return STATUS_ERROR;
    }

    char *name = strtok(addstring, DELIMITER);
    char *addr = strtok(NULL, DELIMITER); // subsequent call when parsing string "s" needs that same string "s" to be NULL
    char *hours = strtok(NULL, DELIMITER);

    if (!name || !addr || !hours || strlen(name) == 0 || strlen(addr) == 0 || strlen(hours) == 0) {
        printf("Wrong format for adding employee\n");
        return STATUS_ERROR;
    }

    struct employee_t *employees = reallocarray(*employeesOut, dbhdr->count + 1, sizeof(struct employee_t));
    /* struct employee_t *employees = *employeesOut;
    employees = reallocarray(employees, dbhdr->count + 1, sizeof(struct employee_t)); */
    if (employees == NULL) {
        perror("reallocarray");
        printf("Reallocation failed\n");
        return STATUS_ERROR;
    }
    dbhdr->count++; // after allocating more memory, increment "count" to make space for a new employee

    int idx = dbhdr->count - 1;
    // copy non-null bytes from "name" to "employees" at current array position of "count -1"
    // CAREFULL: strncpy does not automatically append null character to *destination* if *source* >= *destination*
    if ((strlen(strncpy(employees[idx].name, name, sizeof(employees[idx].name) - 1))) != strlen(name)) {
        printf("String length mismatch after copy\n");
        return STATUS_ERROR;
    }

    if ((strlen(strncpy(employees[idx].address, addr, sizeof(employees[idx].address) - 1))) != strlen(addr)) {
        printf("String length mismatch after copy\n");
        return STATUS_ERROR;
    }

    // leave space for null-terminator by copying size of field -1
    /* strncpy(employees[idx].name, name, sizeof(employees[idx].name) - 1);
    strncpy(employees[idx].address, addr, sizeof(employees[idx].address) - 1); */

    employees[idx].hours = atoi(hours);

    printf("Successfully added Employee %d: \n", dbhdr->count);
    // printf("(parse): %s %s %d\n", employees[idx].name, employees[idx].address, employees[idx].hours);

    /* printf("%p\n", &employees);
    printf("%p\n", &*employeesOut); */

    *employeesOut = employees;

    // printf("%p\n", &*employeesOut);

    return STATUS_SUCCESS;
}

int read_employees(int fd, struct dbheader_t *dbhdr, struct employee_t **employeesOut) {
    if (fd < 0) {
        printf("Bad file descriptor\n");
        return STATUS_ERROR;
    }

    if (dbhdr == 0) {
        printf("Illegal Header\n");
        return STATUS_ERROR;
    }

    if (employeesOut == NULL) {
        printf("No employees\n");
        return STATUS_ERROR;
    }

    int count = dbhdr->count;

    struct employee_t *employees = calloc(count, sizeof(struct employee_t));
    if (employees == NULL) {
        perror("calloc");
        printf("Malloc failed\n");
        return STATUS_ERROR;
    }

    if ((read(fd, employees, count * sizeof(struct employee_t)) == -1)) {
        perror("read");
        printf("Couldn't read file\n");
        return STATUS_ERROR;
    }

    for (int i = 0; i < count; i++) {
        employees[i].hours = ntohl(employees[i].hours);
    }

    *employeesOut = employees;

    return STATUS_SUCCESS;
}

int output_file(int fd, struct dbheader_t *dbhdr, struct employee_t *employees) {
    if (fd < 0) {
        printf("Bad file descriptor\n");
        return STATUS_ERROR;
    }

    int realcount = dbhdr->count; // save count value to ensure we always work with the same count number

    dbhdr->magic = htonl(dbhdr->magic);
    dbhdr->filesize = htonl(sizeof(struct dbheader_t) + (sizeof(struct employee_t) * realcount));
    dbhdr->count = htons(dbhdr->count);
    dbhdr->version = htons(dbhdr->version);

    lseek(fd, 0, SEEK_SET); // back to beginning of file

    if ((write(fd, dbhdr, sizeof(struct dbheader_t)) == -1)) {
        perror("write");
        printf("Unable to write to file\n");
        return STATUS_ERROR;
    }

    for (int i = 0; i < realcount; i++) {
        employees[i].hours = htonl(employees[i].hours);
        if ((write(fd, &employees[i], sizeof(struct employee_t)) == -1)) {
            perror("write");
            printf("Failed writing employees at position %d\n", i);
            return STATUS_ERROR;
        }
    }

    return STATUS_SUCCESS;
}

void list_employees(struct dbheader_t *dbhdr, struct employee_t *employees) {
    for (int i = 0; i < dbhdr->count; i++) {
        printf("Employee %d\n", i + 1);
        printf("\tName: %s\n", employees[i].name);
        printf("\tAddress: %s\n", employees[i].address);
        printf("\tHours: %d\n", employees[i].hours);
    }
}
