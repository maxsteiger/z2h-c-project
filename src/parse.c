#include <arpa/inet.h>
#include <errno.h>
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
        free(header);
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
    if (headerOut == NULL) {
        printf("Emptry header pointer\n");
        return STATUS_ERROR;
    }

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
        printf("Unable to read from file\n");
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

int add_employee(struct dbheader_t *dbhdr, struct employee_t **employeesOut, char *addstring) {

    if (employeesOut == NULL || *employeesOut == NULL) {
        printf("Illegal employeesOut pointer\n");
        return STATUS_ERROR;
    }

    if (addstring == NULL || strlen(addstring) <= 0) {
        printf("Nothing to add\n");
        return STATUS_ERROR;
    }

    if (dbhdr == NULL) {
        printf("DB-Header is null\n");
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

    char *name, *addr, *hours;

    if ((name = strtok(addstring, DELIMITER)) == NULL) {
        printf("Name invalid\n");
        return STATUS_ERROR;
    }
    if ((addr = strtok(NULL, DELIMITER)) == NULL) { // subsequent call when parsing the same string -> param needs to be NULL
        printf("Address invalid\n");
        return STATUS_ERROR;
    }
    if ((hours = strtok(NULL, DELIMITER)) == NULL) {
        printf("Hours invalid\n");
        return STATUS_ERROR;
    }

    struct employee_t *employees;
    dbhdr->count++; // increment "count" to make space for a new employee
    if ((employees = reallocarray(*employeesOut, dbhdr->count, (sizeof(struct employee_t)))) == NULL) {
        perror("reallocarray");
        printf("Unable to reallocate space for new employee");

        return STATUS_ERROR;
    }

    // size_t is optimized for representing the size of object on target plattform
    size_t idx = dbhdr->count - 1;

    // copy non-null bytes from "name" to "employees" at current array position of "count -1"
    // CAREFULL: strncpy does not automatically append null character to *destination* if *source* >= *destination*
    // how do we guard against missing trailing '\0' ?
    if ((strlen(strncpy(employees[idx].name, name, sizeof(employees[idx].name)))) != strlen(name)) {
        printf("String length mismatch after copy\n");
        return STATUS_ERROR;
    }

    if ((strlen(strncpy(employees[idx].address, addr, sizeof(employees[idx].address)))) != strlen(addr)) {
        printf("String length mismatch after copy\n");
        return STATUS_ERROR;
    }

    char *endptr;
    int val;
    errno = 0; // needed so we can check for strtol failure
    if ((val = strtol(hours, &endptr, 10)) < 0) {
        printf("Can't input negative numbers\n");
        return STATUS_ERROR;
    }
    if (errno == ERANGE) { // checks if error occured in strtol
        perror("strtol");
        printf("String conversion to long failed\n");
        return STATUS_ERROR;
    }

    if (endptr == hours) {
        printf("No digits found in %s\n", hours);
        return STATUS_ERROR;
    }

    if (*endptr != '\0') /* Not necessarily an error... */
        printf("Illegal characters after number: \"%s\"\n", endptr);

    employees[idx].hours = val;

    *employeesOut = employees;

    return STATUS_SUCCESS;
}

int read_employees(int fd, struct dbheader_t *dbhdr, struct employee_t **employeesOut) {

    if (employeesOut == NULL) {
        printf("Illegal header pointer\n");
        return STATUS_ERROR;
    }

    if (dbhdr == NULL) {
        printf("DB-Header is null\n");
        return STATUS_ERROR;
    }

    if (fd < 0) {
        printf("Bad file descriptor\n");
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
        printf("Couldn't read file");
        return STATUS_ERROR;
    }

    for (int i = 0; i < count; i++) {
        employees[i].hours = ntohl(employees[i].hours);
    }

    *employeesOut = employees;

    return STATUS_SUCCESS;
}

int output_file(int fd, struct dbheader_t *dbhdr, struct employee_t *employees) {

    if (dbhdr == NULL || employees == NULL) {
        printf("DB-Header or Employees null\n");
        return STATUS_ERROR;
    }

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
