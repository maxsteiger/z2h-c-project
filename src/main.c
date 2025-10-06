#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "common.h"
#include "file.h"
#include "parse.h"

void print_usage(char *argv[]) {
    printf("Usage: %s -n -f <database file>\n", argv[0]);
    printf("\t -n  - create new database file\n");
    printf("\t -f  - (required) path to database file\n");
    printf("\t -a  - add new employee <name,adress,workhours>\n");
    return;
}

int main(int argc, char *argv[]) {

    char *filepath = NULL;
    char *addstring = NULL;
    bool newfile = false;
    int c;

    int dbfd = -1;

    while ((c = getopt(argc, argv, "nf:a:")) != -1) {
        switch (c) {
        case 'n':
            newfile = true;
            break;
        case 'f':
            filepath = optarg;
            break;
        case 'a':
            addstring = optarg;
            break;
        case '?':
            print_usage(argv);
            return STATUS_ERROR;
        default:
            return STATUS_ERROR;
        }
    }

    if (filepath == NULL) {
        printf("Filepath is required\n");
        print_usage(argv);

        return STATUS_ERROR;
    }

    struct dbheader_t *dbheader = NULL;

    if (newfile) {
        dbfd = create_db_file(filepath);
        if (dbfd == STATUS_ERROR) {
            printf("Unable to create database file\n");
            return STATUS_ERROR;
        }

        if (create_db_header(&dbheader) == STATUS_ERROR) {
            printf("Failed to create database header\n");
            close_db_file(dbfd);
            return STATUS_ERROR;
        }

    } else {
        dbfd = open_db_file(filepath);
        if (dbfd == STATUS_ERROR) {
            printf("Unable to open database file\n");
            return STATUS_ERROR;
        }

        if (validate_db_header(dbfd, &dbheader) == STATUS_ERROR) {
            printf("Failed to validate database header\n");
            close_db_file(dbfd);
            // free(dbheader);
            return STATUS_ERROR;
        }
    }

    struct employee_t *employees = NULL; // this will be overwritten inside 'read_employees', with pointer from calloc

    if (addstring != NULL) {

        if (read_employees(dbfd, dbheader, &employees) == STATUS_ERROR) {
            printf("Failed to read employees\n");
            close_db_file(dbfd);
            free(dbheader);
            free(employees);
            return STATUS_ERROR;
        }

        for (int e = 0; e < dbheader->count; e++) {
            printf("employees after READ in main:\"%s,%s,%d\" length: %lu\n", employees[e].name, employees[e].address, employees[e].hours,
                   sizeof(employees));
        }

        printf("dbheader count before: %d\n", dbheader->count);
        printf("(main) employees allocated at: %p - length: %lu\n", employees, sizeof *employees);

        if (add_employee(dbheader, &employees, addstring) == STATUS_ERROR) {
            printf("Unable to add employee: %s\n", addstring);
            free(dbheader);
            free(employees);
            return STATUS_ERROR;
        }
        printf("dbheader count %s after: %d\n" CRESET, RED, dbheader->count);
        printf("(main) employees allocated length AFTER: %lu\n", sizeof *employees);

        for (int e = 0; e < dbheader->count; e++) {
            printf("values of 'employees' AFTER add_employee:\n\t\"%s,%s,%d\" length: %lu\n", employees[e].name, employees[e].address,
                   employees[e].hours, sizeof(employees));
        }
    }

    if (output_file(dbfd, dbheader, employees) == STATUS_ERROR) {
        printf("Unable to create output file\n");
        // free(dbheader);
        // free(employees);
        return STATUS_ERROR;
    }

    if (dbfd != -1) {
        close_db_file(dbfd);
    }

    free(dbheader);
    free(employees);
    employees = NULL;
    printf("\n");

    return STATUS_SUCCESS;
}