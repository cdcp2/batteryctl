#define _POSIX_C_SOURCE 200809L
#include "sysfs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char test_dir[] = "/tmp/batteryctl-test-XXXXXX";
static char attribute[512];
static unsigned int checks;
static unsigned int failures;

#define CHECK(condition) do {                                       \
    checks++;                                                        \
    if (!(condition)) {                                              \
        fprintf(stderr, "FAIL (%s:%d): %s\n", __FILE__, __LINE__, \
                #condition);                                        \
        return 0;                                                    \
    }                                                                \
} while (0)

static int set_contents(const char *value)
{
    FILE *file = fopen(attribute, "w");
    if (file == NULL)
        return 0;
    int ok = fputs(value, file) >= 0;
    if (fclose(file) != 0)
        ok = 0;
    return ok;
}

static int test_read_with_newline(void)
{
    char value[32];
    CHECK(set_contents("80\n"));
    CHECK(read_sysfs_value(attribute, value, sizeof(value)) == IO_OK);
    CHECK(strcmp(value, "80") == 0);
    return 1;
}

static int test_read_without_newline(void)
{
    char value[32];
    CHECK(set_contents("75"));
    CHECK(read_sysfs_value(attribute, value, sizeof(value)) == IO_OK);
    CHECK(strcmp(value, "75") == 0);
    return 1;
}

static int test_write_and_read_back(void)
{
    char value[32];
    CHECK(set_contents("80\n"));
    CHECK(write_sysfs_value(attribute, "60\n") == IO_OK);
    CHECK(read_sysfs_value(attribute, value, sizeof(value)) == IO_OK);
    CHECK(strcmp(value, "60") == 0);
    return 1;
}

static int test_missing_read(void)
{
    char path[512];
    char value[32];
    int n = snprintf(path, sizeof(path), "%s/missing", test_dir);
    CHECK(n > 0 && (size_t)n < sizeof(path));
    CHECK(read_sysfs_value(path, value, sizeof(value)) == IO_UNAVAILABLE);
    return 1;
}

static int test_missing_parent_write(void)
{
    char path[512];
    int n = snprintf(path, sizeof(path), "%s/no-directory/value", test_dir);
    CHECK(n > 0 && (size_t)n < sizeof(path));
    CHECK(write_sysfs_value(path, "80\n") != IO_OK);
    return 1;
}

static int test_oversized_value(void)
{
    char small[5];
    CHECK(set_contents("123456789\n"));
    CHECK(read_sysfs_value(attribute, small, sizeof(small)) == IO_ERROR);
    return 1;
}

static int test_zero_size(void)
{
    char value[1] = { 'X' };
    CHECK(read_sysfs_value(attribute, value, 0) == IO_ERROR);
    CHECK(value[0] == 'X');
    return 1;
}

static int test_null_read_arguments(void)
{
    char value[32];
    CHECK(read_sysfs_value(attribute, NULL, sizeof(value)) == IO_ERROR);
    CHECK(read_sysfs_value(NULL, value, sizeof(value)) == IO_ERROR);
    return 1;
}

static int test_null_write_arguments(void)
{
    CHECK(write_sysfs_value(NULL, "80\n") == IO_ERROR);
    CHECK(write_sysfs_value(attribute, NULL) == IO_ERROR);
    return 1;
}

static int test_directory_not_attribute(void)
{
    char value[32];
    CHECK(read_sysfs_value(test_dir, value, sizeof(value)) != IO_OK);
    CHECK(write_sysfs_value(test_dir, "60\n") != IO_OK);
    return 1;
}

static void run_test(const char *name, int (*test)(void))
{
    if (test())
        printf("PASS %s\n", name);
    else {
        printf("FAIL %s\n", name);
        failures++;
    }
}

int main(void)
{
    if (mkdtemp(test_dir) == NULL) {
        perror("mkdtemp");
        return EXIT_FAILURE;
    }
    int n = snprintf(attribute, sizeof(attribute),
                     "%s/charge_control_end_threshold", test_dir);
    if (n < 0 || (size_t)n >= sizeof(attribute)) {
        fprintf(stderr, "Test path too long\n");
        rmdir(test_dir);
        return EXIT_FAILURE;
    }

    run_test("read with newline", test_read_with_newline);
    run_test("read without newline", test_read_without_newline);
    run_test("write and read back", test_write_and_read_back);
    run_test("missing attribute", test_missing_read);
    run_test("write failure", test_missing_parent_write);
    run_test("buffer too small", test_oversized_value);
    run_test("zero-size buffer", test_zero_size);
    run_test("NULL read arguments", test_null_read_arguments);
    run_test("NULL write arguments", test_null_write_arguments);
    run_test("directory is not an attribute", test_directory_not_attribute);

    if (unlink(attribute) != 0) {
        perror("unlink");
        failures++;
    }
    if (rmdir(test_dir) != 0) {
        perror("rmdir");
        failures++;
    }
    printf("\nChecks: %u | Failed tests: %u\n", checks, failures);
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
