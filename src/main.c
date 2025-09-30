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
    bool newfile = false;
    int c;

    int dbfd = STATUS_ERROR;
    struct dbheader_t *dbhdr = NULL;
    struct employee_t *employees = NULL;

    while ((c = getopt(argc, argv, "nf:")) != -1) {
        switch (c) {
        case 'n':
            newfile = true;
            break;
        case 'f':
            filepath = optarg;
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
            return STATUS_ERROR;
        }
    }

    output_file(dbfd, dbhdr, employees);

    free(dbhdr);
    if (dbfd != STATUS_ERROR) {
        close(dbfd);
    }
    return 0;
}