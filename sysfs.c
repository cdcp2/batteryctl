#include "sysfs.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>


enum IoResult read_sysfs_value(const char *path, char *buffer,
                                      size_t buffer_size)
{
    if (path == NULL || buffer == NULL || buffer_size < 2 || buffer_size > INT_MAX) {
        fprintf(stderr, "Invalid read arguments\n");
        return IO_ERROR;
    }

    FILE *file = fopen(path, "r");
    if (file == NULL) {
        if (errno == ENOENT || errno == ENODEV)
            return IO_UNAVAILABLE;
        perror(path);
        return IO_ERROR;
    }

    enum IoResult ret = IO_OK;

    if (fgets(buffer, (int)buffer_size, file) == NULL) {
        if (ferror(file))
            perror(path);
        else
            fprintf(stderr, "Empty sysfs attribute: %s\n", path);
        ret = IO_ERROR;
        goto cleanup;
    }

    char *newline = strchr(buffer, '\n');
    if (newline != NULL) {
        *newline = '\0';
    } else {
        int next = fgetc(file);
        if (next != EOF) {
            fprintf(stderr, "Sysfs value too long: %s\n", path);
            ret = IO_ERROR;
        } else if (ferror(file)) {
            perror(path);
            ret = IO_ERROR;
        }
    }

cleanup:
    if (fclose(file) == EOF) {
        perror("fclose");
        ret = IO_ERROR;
    }
    return ret;
}

enum IoResult write_sysfs_value(const char *path, const char *value)
{
    if (path == NULL || value == NULL) {
        fprintf(stderr, "Invalid write arguments\n");
        return IO_ERROR;
    }

    FILE *file = fopen(path, "w");
    if (file == NULL) {
        perror(path);
        return IO_ERROR;
    }

    enum IoResult ret = IO_OK;
    if (fprintf(file, "%s", value) < 0) {
        perror("fprintf");
        ret = IO_ERROR;
    }

    if (fclose(file) == EOF) {
        perror("fclose");
        ret = IO_ERROR;
    }
    return ret;
}
