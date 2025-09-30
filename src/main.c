#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>

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
            return -1;
        }
    }

    if (filepath == NULL) {
        printf("Filepath is required\n");
        print_usage(argv);
    }

    printf("Newfile: %d\n", newfile);
    printf("Filepath: %s\n", filepath);

    return 0;
}