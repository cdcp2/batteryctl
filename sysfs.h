#ifndef SYSFS_H
#define SYSFS_H

#include <stddef.h>

enum IoResult {
    IO_OK,
    IO_UNAVAILABLE,
    IO_ERROR
};

enum IoResult read_sysfs_value(
    const char *path,
    char *buffer,
    size_t buffer_size
);

enum IoResult write_sysfs_value(
    const char *path,
    const char *value
);

#endif
