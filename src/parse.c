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
    // allocate & init memory the size of 1 member of struct dbheader_t which allows us to return a pointer to allocated space
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
    if (headerOut == NULL) {
        printf("Passed empty header pointer\n");
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
        free(header);
        return STATUS_ERROR;
    }

    if (header->magic != HEADER_MAGIC) {
        printf("Improper header magic\n");
        free(header);
        return STATUS_ERROR;
    }

    struct stat dbstat = {0};
    fstat(fd, &dbstat);
    if (header->filesize != dbstat.st_size) {
        printf("Corrupt database\n");
        free(header);
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
    if ((addr = strtok(NULL, DELIMITER)) == NULL) { // subsequent call to 'strtok()' when parsing the same string -> param needs to be NULL
        printf("Address invalid\n");
        return STATUS_ERROR;
    }
    if ((hours = strtok(NULL, DELIMITER)) == NULL) {
        printf("Hours invalid\n");
        return STATUS_ERROR;
    }

    if (strlen(name) == 0 || strlen(addr) == 0 || strlen(hours) == 0) {
        printf("Employee fields can't be empty\n");
        return STATUS_ERROR;
    }

    printf("%d\n", dbhdr->count);
    dbhdr->count++; // increment "count" to make space for a new employee

    printf("employeesOut allocated at: %p\n", *&employeesOut);
    struct employee_t *employees = NULL;
    employees = reallocarray(*employeesOut, dbhdr->count, sizeof(struct employee_t));
    if (employees == NULL) {
        perror("reallocarray");
        printf("Unable to reallocate space for new employee");
        return STATUS_ERROR;
    }

    // size_t is an optimized type for representing the size of an object on current target plattform
    size_t idx = dbhdr->count - 1;

    // copy non-null bytes from "name" to "employees" at current array position of "count -1"
    // CAREFULL: strncpy does not automatically append null character to *destination* if *source* >= *destination*
    // how do we guard against missing trailing '\0' ?
    if ((strlen(strncpy(employees[idx].name, name, sizeof(employees[idx].name) - 1))) != strlen(name)) {
        printf("String length mismatch after copy\n");
        return STATUS_ERROR;
    }

    if ((strlen(strncpy(employees[idx].address, addr, sizeof(employees[idx].address) - 1))) != strlen(addr)) {
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

    printf("allocated at: %p\n", &employees);
    printf("employeesOut allocated at: %p\n", *&employeesOut);
    int lenEmployees = sizeof(*employees) / sizeof(employees[0]);
    int lenEmployeesOut = sizeof(employeesOut) / sizeof(employeesOut[0]);

    for (int i = 0; i < dbhdr->count; i++) {
        printf("values in 'employees': \"%s,%s,%d\" length: %d\n", employees[i].name, employees[i].address, employees[i].hours,
               lenEmployees);
        printf("values in 'employeesOut' from parse:\n\t\"%s,%s,%d\" length: %d\n", employeesOut[i]->name, employeesOut[i]->address,
               employeesOut[i]->hours, lenEmployeesOut);
    }

    *employeesOut = employees;

    printf("Added Employee Nr. %d: %s\n", dbhdr->count, name);

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

    // create buffer in memory to fill with data read from disk
    struct employee_t *employees = calloc(count, sizeof(struct employee_t));
    if (employees == NULL) {
        perror("calloc");
        printf("Malloc failed\n");
        return STATUS_ERROR;
    }

    // fill buffer with data from file descriptor (disk)
    if ((read(fd, employees, count * sizeof(struct employee_t)) == -1)) {
        perror("read");
        printf("Couldn't read file");
        return STATUS_ERROR;
    }

    for (int i = 0; i < count; i++) {
        employees[i].hours = ntohl(employees[i].hours);
    }

    *employeesOut = employees;

    for (int e = 0; e < count; e++) {
        printf("values in 'read_employees': \"%s,%s,%d\" length: %lu\n", employees[e].name, employees[e].address, employees[e].hours,
               sizeof(*employees) / sizeof(employees[0]));

        printf("values of 'read_employeesOut': \"%s,%s,%d\" length: %lu\n", employeesOut[e]->name, employeesOut[e]->address,
               employeesOut[e]->hours, sizeof(employeesOut) / sizeof(*employeesOut[0]));
    }
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

    ftruncate(fd, sizeof(struct dbheader_t) + sizeof(struct employee_t) * realcount);

    return STATUS_SUCCESS;
}
