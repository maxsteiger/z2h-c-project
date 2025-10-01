#ifndef PARSE_H
#define PARSE_H

#define HEADER_MAGIC 0x4c4c4144 // used to "mark" a db-file which allows us to determine if that file can be processed.
#define DELIMITER ";"           // choose ";" delimiter because CSV files

struct dbheader_t {
    unsigned int magic;
    unsigned short version;
    unsigned short count;
    unsigned int filesize;
};

struct employee_t {
    char name[256];
    char address[256];
    unsigned int hours;
};

int create_db_header(struct dbheader_t **headerOut);
int validate_db_header(int fd, struct dbheader_t **headerOut);
int read_employees(int fd, struct dbheader_t *, struct employee_t **employeesOut);
int add_employees(struct dbheader_t *, struct employee_t *employees, char *addstring);
int output_file(int fd, struct dbheader_t *, struct employee_t *employees);

#endif