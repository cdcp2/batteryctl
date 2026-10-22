#include "battery.h"
#include "sysfs.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define POWER_SUPPLY_DIR "/sys/class/power_supply"
#define SHIFT_MODE_PATH "/sys/devices/platform/msi-ec/shift_mode"

#define BATTERY_NAME_SIZE 256
#define SYSFS_PATH_SIZE 512

/* Encuentra una unica bateria leyendo el atributo "type" de cada dispositivo. */
static enum IoResult find_battery(char *name, size_t size)
{
    if (name == NULL || size == 0) {
        fprintf(stderr, "Invalid battery name buffer\n");
        return IO_ERROR;
    }

    DIR *directory = opendir(POWER_SUPPLY_DIR);
    if (directory == NULL) {
        perror(POWER_SUPPLY_DIR);
        return IO_ERROR;
    }

    enum IoResult ret = IO_UNAVAILABLE;
    struct dirent *entry;
    char path[SYSFS_PATH_SIZE];
    char type[32];

    for (;;) {
        errno = 0;
        entry = readdir(directory);
        if (entry == NULL) {
            if (errno != 0) {
                perror("readdir");
                ret = IO_ERROR;
            }
            break;
        }

        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0)
            continue;

        int n = snprintf(path, sizeof(path), "%s/%s/type",
                         POWER_SUPPLY_DIR, entry->d_name);
        if (n < 0 || (size_t)n >= sizeof(path)) {
            fprintf(stderr, "Power supply path too long\n");
            ret = IO_ERROR;
            break;
        }

        enum IoResult result = read_sysfs_value(path, type, sizeof(type));
        if (result == IO_UNAVAILABLE)
            continue;
        if (result != IO_OK) {
            ret = IO_ERROR;
            break;
        }
        if (strcmp(type, "Battery") != 0)
            continue;

        if (ret == IO_OK) {
            fprintf(stderr, "Multiple batteries found; selection is ambiguous\n");
            ret = IO_ERROR;
            break;
        }

        n = snprintf(name, size, "%s", entry->d_name);
        if (n < 0 || (size_t)n >= size) {
            fprintf(stderr, "Battery name too long\n");
            ret = IO_ERROR;
            break;
        }
        ret = IO_OK;
    }

    if (closedir(directory) == -1) {
        perror("closedir");
        ret = IO_ERROR;
    }
    return ret;
}

/* Construye una ruta a un atributo de la bateria descubierta. */
static enum IoResult make_battery_path(const char *battery,
                                       const char *attribute,
                                       char *path, size_t size)
{
    if (battery == NULL || attribute == NULL || path == NULL || size == 0)
        return IO_ERROR;

    int n = snprintf(path, size, "%s/%s/%s",
                     POWER_SUPPLY_DIR, battery, attribute);
    if (n < 0 || (size_t)n >= size) {
        fprintf(stderr, "Battery attribute path too long\n");
        return IO_ERROR;
    }
    return IO_OK;
}

static int print_sysfs_attribute(const char *label, const char *path,
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

static int print_battery_attribute(const char *battery, const char *label,
                                   const char *attribute, const char *suffix,
                                   int optional)
{
    char path[SYSFS_PATH_SIZE];
    if (make_battery_path(battery, attribute, path, sizeof(path)) != IO_OK)
        return EXIT_FAILURE;
    return print_sysfs_attribute(label, path, suffix, optional);
}

static void print_section(const char *title)
{
    printf("\n%s\n------------------------------\n", title);
}

int print_battery_status(void)
{
    char battery[BATTERY_NAME_SIZE];
    enum IoResult found = find_battery(battery, sizeof(battery));
    if (found != IO_OK) {
        if (found == IO_UNAVAILABLE)
            fprintf(stderr, "No battery found\n");
        return EXIT_FAILURE;
    }

    print_section("Battery");
    printf("  %-20s %s\n", "Device", battery);

    if (print_battery_attribute(battery, "Capacity", "capacity", "%", 0) != EXIT_SUCCESS ||
        print_battery_attribute(battery, "Status", "status", "", 0) != EXIT_SUCCESS)
        return EXIT_FAILURE;

    print_section("Charge protection");
    if (print_battery_attribute(battery, "Start threshold",
                                "charge_control_start_threshold", "%", 1) != EXIT_SUCCESS ||
        print_battery_attribute(battery, "End threshold",
                                "charge_control_end_threshold", "%", 1) != EXIT_SUCCESS)
        return EXIT_FAILURE;

    print_section("Performance");
    return print_sysfs_attribute("Current mode", SHIFT_MODE_PATH, "", 1);
}

int set_charge_limit(unsigned int percentage)
{
    if (percentage < 10 || percentage > 100) {
        fprintf(stderr, "Invalid charge limit: %u\n", percentage);
        return EXIT_FAILURE;
    }

    char battery[BATTERY_NAME_SIZE];
    if (find_battery(battery, sizeof(battery)) != IO_OK) {
        fprintf(stderr, "Cannot select a battery\n");
        return EXIT_FAILURE;
    }

    char path[SYSFS_PATH_SIZE];
    if (make_battery_path(battery, "charge_control_end_threshold",
                          path, sizeof(path)) != IO_OK)
        return EXIT_FAILURE;

    char value[16];
    int n = snprintf(value, sizeof(value), "%u\n", percentage);
    if (n < 0 || (size_t)n >= sizeof(value)) {
        fprintf(stderr, "Failed to format charge limit\n");
        return EXIT_FAILURE;
    }

    if (write_sysfs_value(path, value) != IO_OK)
        return EXIT_FAILURE;

    char actual_text[32];
    if (read_sysfs_value(path, actual_text, sizeof(actual_text)) != IO_OK) {
        fprintf(stderr, "Failed to read back charge limit\n");
        return EXIT_FAILURE;
    }

    /* strtoul acepta signos/espacios: para sysfs exigimos solo digitos. */
    if (actual_text[0] == '\0' ||
        strspn(actual_text, "0123456789") != strlen(actual_text)) {
        fprintf(stderr, "Invalid charge limit returned by driver: %s\n", actual_text);
        return EXIT_FAILURE;
    }

    errno = 0;
    char *endptr;
    unsigned long actual = strtoul(actual_text, &endptr, 10);
    if (errno == ERANGE || endptr == actual_text || *endptr != '\0') {
        fprintf(stderr, "Invalid charge limit returned by driver\n");
        return EXIT_FAILURE;
    }

    if (actual != percentage) {
        fprintf(stderr, "Charge limit mismatch: expected %u, got %lu\n",
                percentage, actual);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int set_performance_mode(const char *mode)
{
    if (mode == NULL ||
        (strcmp(mode, "comfort") != 0 &&
         strcmp(mode, "eco") != 0 &&
         strcmp(mode, "turbo") != 0)) {
        fprintf(stderr, "Invalid performance mode\n");
        return EXIT_FAILURE;
    }

    char value[32];
    int n = snprintf(value, sizeof(value), "%s\n", mode);
    if (n < 0 || (size_t)n >= sizeof(value)) {
        fprintf(stderr, "Failed to format performance mode\n");
        return EXIT_FAILURE;
    }

    if (write_sysfs_value(SHIFT_MODE_PATH, value) != IO_OK)
        return EXIT_FAILURE;

    char actual[32];
    if (read_sysfs_value(SHIFT_MODE_PATH, actual, sizeof(actual)) != IO_OK)
        return EXIT_FAILURE;

    if (strcmp(actual, mode) != 0) {
        fprintf(stderr, "Performance mode mismatch: expected %s, got %s\n",
                mode, actual);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
