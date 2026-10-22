#include "battery.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static void print_usage(const char *program)
{
    fprintf(stderr,
            "Usage:\n"
            "  %s status\n"
            "  %s health\n"
            "  %s limit <60|80|100>\n"
            "  %s performance <comfort|eco|turbo>\n",
            program, program, program, program);
}

static int parse_charge_limit(const char *text, unsigned int *limit)
{
    if (text == NULL || limit == NULL || text[0] == '\0') {
        fprintf(stderr, "Invalid charge limit\n");
        return EXIT_FAILURE;
    }

    if (strspn(text, "0123456789") != strlen(text)) {
        fprintf(stderr, "Charge limit must be a number\n");
        return EXIT_FAILURE;
    }

    errno = 0;
    char *endptr;
    unsigned long parsed = strtoul(text, &endptr, 10);

    if (errno == ERANGE || *endptr != '\0') {
        fprintf(stderr, "Invalid charge limit\n");
        return EXIT_FAILURE;
    }

    if (parsed != 60 && parsed != 80 && parsed != 100) {
        fprintf(stderr, "Supported limits: 60, 80, 100\n");
        return EXIT_FAILURE;
    }

    *limit = (unsigned int)parsed;
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
    if (argc == 2) {
        if (strcmp(argv[1], "status") == 0)
            return print_battery_status();

        if (strcmp(argv[1], "health") == 0)
            return print_battery_health();
    }

    if (argc == 3){
        if (strcmp(argv[1], "performance") == 0)
            return set_performance_mode(argv[2]);
        if (strcmp(argv[1], "limit") == 0){
            unsigned int limit; 
            if (parse_charge_limit(argv[2], &limit)!= EXIT_SUCCESS) 
                return EXIT_FAILURE;

            return set_charge_limit(limit);
        }
    }

    print_usage(argv[0]);
    return EXIT_FAILURE;
}
