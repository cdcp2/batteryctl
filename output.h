#ifndef OUTPUT_H
#define OUTPUT_H

void print_section(const char *title);

int print_sysfs_attribute(
    const char *label,
    const char *path,
    const char *suffix,
    int optional
);

#endif
