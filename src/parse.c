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
    if (fd < 0) {
        printf("Bad file descriptor\n");
        return STATUS_ERROR;
    }

    struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));
    if (header == NULL) {
        perror("calloc");
        printf("Malloc failed to create db header\n");
        free(header);
        return STATUS_ERROR;
    }

    if (read(fd, header, sizeof(struct dbheader_t)) != sizeof(struct dbheader_t)) {
        perror("read");
        free(header);
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

int add_employees(struct dbheader_t *dbhdr, struct employee_t *employees, char *addstring) {
    char *returnname, *returnaddr;
    char *name = strtok(addstring, DELIMITER);
    char *addr = strtok(NULL, DELIMITER); // subsequent call when parsing the same string -> param needs to be NULL
    char *hours = strtok(NULL, DELIMITER);

    printf("%s %s %s\n", name, addr, hours);

    // copy non-null bytes from "name" to "employees" at current array position of "count -1"
    // CAREFULL: strncpy does not automatically append null character to *destination* if *source* >= *destination*

    if ((strlen(strncpy(employees[dbhdr->count - 1].name, name, sizeof(employees[dbhdr->count - 1].name)))) != strlen(name)) {
        printf("String length mismatch after copy\n");
        return STATUS_ERROR;
    }

    if ((strlen(strncpy(employees[dbhdr->count - 1].address, addr, sizeof(employees[dbhdr->count - 1].address)))) != strlen(addr)) {
        printf("String length mismatch after copy\n");
        return STATUS_ERROR;
    }

    if ((employees[dbhdr->count - 1].hours = atoi(hours)) == 0 && (employees[dbhdr->count - 1].hours != 0)) {
        perror("atoi");
        printf("Unable to convert input to string\n");
        return STATUS_ERROR;
    }

    return STATUS_SUCCESS;
}

int read_employees(int fd, struct dbheader_t *dbhdr, struct employee_t **employeesOut) {
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
    if (fd < 0) {
        printf("Bad file descriptor\n");
        free(dbhdr);
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
        free(dbhdr);
        return STATUS_ERROR;
    }

    for (int i = 0; i < realcount; i++) {
        employees[i].hours = htonl(employees[i].hours);
        if ((write(fd, &employees[i], sizeof(struct employee_t)) == -1)) {
            perror("write");
            printf("Failed writing employees at position %d\n", i);
            free(dbhdr);
            return STATUS_ERROR;
        }
    }

    return STATUS_SUCCESS;
}
