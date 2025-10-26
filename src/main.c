#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "common.h"
#include "file.h"
#include "parse.h"

void free_pointers(struct dbheader_t *dbhdr, struct employee_t *employees) {

    if (dbhdr != NULL) {
        printf("Free dbheader\n");
        free(dbhdr);
        dbhdr = NULL;
    }

    if (employees != NULL) {
        printf("Free employees\n");
        free(employees);
        employees = NULL;
    }

    return;
}

void print_usage(char *argv[]) {
    printf("Usage: %s -n -f <database file>\n", argv[0]);
    printf("\t -n  - create new database file\n");
    printf("\t -f  - (required) path to database file\n");
    printf("\t -l  - list the employees\n");
    printf("\t -a  - add new employee in format: \"Name%1$sAddress%1$sWorkHours\"\n", DELIMITER);
    return;
}

int main(int argc, char *argv[]) {

    char *filepath = NULL;
    char *addstring = NULL;
    bool newfile = false;
    bool list = false;
    int c;

    int dbfd = -1;

    while ((c = getopt(argc, argv, "nf:a:l")) != -1) {
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
        case 'l':
            list = true;
            break;
        case '?':
            printf("Unknown option -%c\n", c);
            break;
        default:
            return STATUS_ERROR;
        }
    }

    if (filepath == NULL) {
        printf("Filepath is required\n");
        print_usage(argv);

        return STATUS_SUCCESS;
    }

    struct dbheader_t *dbhdr = NULL;

    if (newfile) {
        dbfd = create_db_file(filepath);
        if (dbfd == STATUS_ERROR) {
            printf("Unable to create database file\n");
            return STATUS_ERROR;
        }

        if (create_db_header(&dbhdr) == STATUS_ERROR) {
            printf("Failed to create database header\n");
            return STATUS_ERROR;
        }

    } else {
        dbfd = open_db_file(filepath);
        if (dbfd == STATUS_ERROR) {
            printf("Unable to open database file\n");
            return STATUS_ERROR;
        }

        if (validate_db_header(dbfd, &dbhdr) == STATUS_ERROR) {
            printf("Failed to validate database header\n");
            return STATUS_ERROR;
        }
    }

    struct employee_t *employees = NULL;

    if (read_employees(dbfd, dbhdr, &employees) != STATUS_SUCCESS) {
        printf("Failed to read employees\n");
        free_pointers(dbhdr, employees);
        return STATUS_ERROR;
    }

    if (addstring) {
        if (add_employee(dbhdr, &employees, addstring) == STATUS_ERROR) {
            printf("Unable to add employee\n");
            free_pointers(dbhdr, employees);
            return STATUS_ERROR;
        }

        /* printf("%d\n", dbhdr->count);
        printf("(main): %s %s %d\n", employees[dbhdr->count - 1].name, employees[dbhdr->count - 1].address,
               employees[dbhdr->count - 1].hours); */
    }

    if (list) {
        list_employees(dbhdr, employees);
    }

    if (output_file(dbfd, dbhdr, employees) == STATUS_ERROR) {
        printf("Unable to create output file\n");
        return STATUS_ERROR;
    }

    if (dbfd != -1) {
        close_db_file(&dbfd);
    }

    free_pointers(dbhdr, employees);

    return STATUS_SUCCESS;
}