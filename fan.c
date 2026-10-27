#include "fan.h"
#include "output.h"
#include "sysfs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define FAN_MODE_PATH \
    "/sys/devices/platform/msi-ec/fan_mode"

#define AVAILABLE_FAN_MODES_PATH \
    "/sys/devices/platform/msi-ec/available_fan_modes"

#define CPU_FAN_PATH \
    "/sys/devices/platform/msi-ec/cpu/realtime_fan_speed"

#define GPU_FAN_PATH \
    "/sys/devices/platform/msi-ec/gpu/realtime_fan_speed"

#define COOLER_BOOST_PATH \
    "/sys/devices/platform/msi-ec/cooler_boost"

int print_fan_status(void)
{
    int ret = EXIT_SUCCESS;

    print_section("Fan status");

    if (print_sysfs_attribute(
            "Mode", FAN_MODE_PATH, "", 0) != EXIT_SUCCESS)
        ret = EXIT_FAILURE;

    if (print_sysfs_attribute(
            "CPU fan", CPU_FAN_PATH, "%", 1) != EXIT_SUCCESS)
        ret = EXIT_FAILURE;

    if (print_sysfs_attribute(
            "GPU fan", GPU_FAN_PATH, "%", 1) != EXIT_SUCCESS)
        ret = EXIT_FAILURE;

    if (print_sysfs_attribute(
            "Cooler Boost", COOLER_BOOST_PATH, "", 1) != EXIT_SUCCESS)
        ret = EXIT_FAILURE;

    return ret;
}


int print_available_fan_modes(void)
{
    FILE *file = fopen(AVAILABLE_FAN_MODES_PATH, "r");

    if (file == NULL) {
        perror(AVAILABLE_FAN_MODES_PATH);
        return EXIT_FAILURE;
    }

    int ret = EXIT_SUCCESS;
    char buffer[64];

    print_section("Available fan modes");

    while (fgets(buffer, sizeof(buffer), file) != NULL) {

        char *newline = strchr(buffer, '\n');

        if (newline != NULL) {
            *newline = '\0';
        } else {
            int next = fgetc(file);

            if (next != EOF) {
                fprintf(stderr, "Fan mode line too long\n");
                ret = EXIT_FAILURE;
                goto cleanup;
            }

            if (ferror(file)) {
                perror(AVAILABLE_FAN_MODES_PATH);
                ret = EXIT_FAILURE;
                goto cleanup;
            }
        }

        printf("  %s\n", buffer);
    }

    if (ferror(file)) {
        perror(AVAILABLE_FAN_MODES_PATH);
        ret = EXIT_FAILURE;
    }

cleanup:
    if (fclose(file) == EOF) {
        perror("fclose");
        ret = EXIT_FAILURE;
    }

    return ret;
}



static enum IoResult is_fan_mode_available(
    const char *mode,
    bool *available)
{
    if (mode == NULL || available == NULL)
        return IO_ERROR;

    *available = false;

    FILE *file = fopen(AVAILABLE_FAN_MODES_PATH, "r");
    if (file == NULL) {
        perror(AVAILABLE_FAN_MODES_PATH);
        return IO_ERROR;
    }

    enum IoResult ret = IO_OK;
    char buffer[64];

    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        char *newline = strchr(buffer, '\n');

        if (newline != NULL) {
            *newline = '\0';
        } else {
            int next = fgetc(file);

            if (next != EOF || ferror(file)) {
                fprintf(stderr, "Invalid fan mode list\n");
                ret = IO_ERROR;
                break;
            }
        }

        if (strcmp(buffer, mode) == 0) {
            *available = true;
            break;
        }
    }

    if (ferror(file)) {
        perror(AVAILABLE_FAN_MODES_PATH);
        ret = IO_ERROR;
    }

    if (fclose(file) == EOF) {
        perror("fclose");
        ret = IO_ERROR;
    }

    return ret;
}

int set_fan_mode(const char *mode)
{
    if (mode == NULL || mode[0] == '\0') {
        fprintf(stderr, "Invalid fan mode\n");
        return EXIT_FAILURE;
    }

    bool available = false;

    if (is_fan_mode_available(mode, &available) != IO_OK) {
        fprintf(stderr, "Cannot check available fan modes\n");
        return EXIT_FAILURE;
    }

    if (!available) {
        fprintf(stderr, "Unsupported fan mode: %s\n", mode);
        return EXIT_FAILURE;
    }

    char value[64];

    int n = snprintf(value, sizeof(value), "%s\n", mode);

    if (n < 0 || (size_t)n >= sizeof(value)) {
        fprintf(stderr, "Fan mode is too long\n");
        return EXIT_FAILURE;
    }

    if (write_sysfs_value(FAN_MODE_PATH, value) != IO_OK) {
        fprintf(stderr, "Cannot set fan mode\n");
        return EXIT_FAILURE;
    }

    char actual[64];

    if (read_sysfs_value(
            FAN_MODE_PATH, actual, sizeof(actual)) != IO_OK) {
        fprintf(stderr, "Cannot read fan mode\n");
        return EXIT_FAILURE;
    }

    if (strcmp(actual, mode) != 0) {
        fprintf(stderr,
                "Fan mode mismatch: expected %s, got %s\n",
                mode, actual);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int set_cooler_boost(const char *state)
{
    if (state == NULL ||
        (strcmp(state, "on") != 0 && strcmp(state, "off") != 0)) {
        fprintf(stderr, "Unsupported Cooler Boost state: %s\n",
                state != NULL ? state : "(null)");
        return EXIT_FAILURE;
    }

    if (write_sysfs_value(COOLER_BOOST_PATH, state) != IO_OK)
        return EXIT_FAILURE;

    char buffer[32];
    if (read_sysfs_value(COOLER_BOOST_PATH,
                         buffer, sizeof(buffer)) != IO_OK)
        return EXIT_FAILURE;

    if (strcmp(buffer, state) != 0) {
        fprintf(stderr,
                "Cooler Boost mismatch: expected %s, got %s\n",
                state, buffer);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
