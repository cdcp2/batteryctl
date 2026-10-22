#include "battery.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_usage(const char *program)
{
    fprintf(stderr,
            "Usage:\n"
            "  %s status\n"
            "  %s enable\n"
            "  %s disable\n"
            "  %s performance <comfort|eco|turbo>\n",
            program, program, program, program);
}

int main(int argc, char *argv[])
{
    if (argc == 2) {
        if (strcmp(argv[1], "status") == 0)
            return print_battery_status();

        if (strcmp(argv[1], "enable") == 0)
            return set_charge_limit(80);

        if (strcmp(argv[1], "disable") == 0)
            return set_charge_limit(100);
    }

    if (argc == 3 &&
        strcmp(argv[1], "performance") == 0) {
        return set_performance_mode(argv[2]);
    }

    print_usage(argv[0]);
    return EXIT_FAILURE;
}
