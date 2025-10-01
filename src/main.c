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
    return;
}

int main(int argc, char *argv[]) {

    char *filepath = NULL;
    char *addstring = NULL;
    bool newfile = false;
    int c;

    int dbfd = -1;
    struct dbheader_t *dbhdr = NULL;
    struct employee_t *employees = NULL;

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
            printf("Unknown option -%c\n", c);
            break;
        default:
            return STATUS_ERROR;
        }
    }

    if (filepath == NULL) {
        printf("Filepath is required\n");
        print_usage(argv);
    }

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
            free(dbhdr);
            return STATUS_ERROR;
        }
    }

    if (read_employees(dbfd, dbhdr, &employees) == STATUS_ERROR) {
        printf("Failed to read employees\n");
        free(dbhdr);
        free(employees);
        return STATUS_ERROR;
    }

    if (addstring != NULL) {
        dbhdr->count++; // increment "count" to make space for a new employee
        if ((employees = realloc(employees, dbhdr->count * (sizeof(struct employee_t)))) == NULL) {
            perror("realloc");
            printf("Unable to reallocate space for new employee");
            free(dbhdr);
            free(employees);
            return STATUS_ERROR;
        }

        if (add_employee(dbhdr, &employees, addstring) == STATUS_ERROR) {
            printf("Unable to add employee: %s\n", addstring);
            free(dbhdr);
            free(employees);
            return STATUS_ERROR;
        }
    }

    if (output_file(dbfd, dbhdr, employees) == STATUS_ERROR) {
        printf("Unable to create output file\n");
        free(dbhdr);
        free(employees);
        return STATUS_ERROR;
    }

    if (dbfd != -1) {
        close_db_file(&dbfd);
    }

    free(dbhdr);
    free(employees);

    return STATUS_SUCCESS;
}