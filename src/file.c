#include <stdio.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"
#include "file.h"

int fd = -1;

int create_db_file(char *filename) {

    if ((fd = open(filename, O_RDONLY)) != -1) {
        close(fd);
        printf("File already exists\n");
        return STATUS_ERROR;
    }

    if ((fd = open(filename, O_RDWR | O_CREAT, 0644)) == -1) {
        perror("open");
        return STATUS_ERROR;
    }

    return fd;
}

int open_db_file(char *filename) {

    if ((fd = open(filename, O_RDWR)) == -1) {
        perror("open");
        return STATUS_ERROR;
    }

    return fd;
}

int close_db_file(int file) {

    if (close(file) == -1) {
        perror("close");
        return STATUS_ERROR;
    }

    return STATUS_SUCCESS;
}