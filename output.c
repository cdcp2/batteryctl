#include "sysfs.h"

#include <stdio.h> 
#include <stdlib.h>

int print_sysfs_attribute(const char *label, const char *path,
                                 const char *suffix, int optional)
{
    char value[64];
    enum IoResult result = read_sysfs_value(path, value, sizeof(value));

    if (result == IO_UNAVAILABLE && optional) {
        printf("  %-20s Unavailable\n", label);
        return EXIT_SUCCESS;
    }
    if (result != IO_OK) {
        if (result == IO_UNAVAILABLE)
            fprintf(stderr, "Required attribute unavailable: %s\n", path);
        return EXIT_FAILURE;
    }

    printf("  %-20s %s%s\n", label, value, suffix);
    return EXIT_SUCCESS;
}


void print_section(const char *title)
{
    printf("\n%s\n------------------------------\n", title);
}

