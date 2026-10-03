#include "cli.h"
#include "custom_assert.h"
#include <string.h>
#include <stdio.h>

static void test_basic_parsing() {
    cli c = cli_create();
    
    cli_option opt_int = {
        .option_name_short = "i",
        .option_name_long = "integer",
        .type = CLI_TYPE_INTEGER,
        .required = true
    };
    ASSERT_MSG(cli_add_option(c, opt_int), "Failed to add integer option");
    
    cli_option opt_str = {
        .option_name_short = "s",
        .option_name_long = "string",
        .type = CLI_TYPE_STRING,
        .required = false,
        .default_value = { .string_value = "default_str" }
    };
    ASSERT_MSG(cli_add_option(c, opt_str), "Failed to add string option");

    const char *argv[] = {"program", "-i", "42", "--unknown", "ignore_me"};
    int argc = 5;

    ASSERT_MSG(cli_parse(c, argc, argv), "Failed to parse valid basic arguments");

    long val_int = 0;
    ASSERT_MSG(cli_get_integer(c, "i", &val_int), "Failed to retrieve integer 'i'");
    ASSERT_MSG(val_int == 42, "Integer value mismatch");
    
    const char *val_str = NULL;
    // We should be able to query it using the long name too
    ASSERT_MSG(cli_get_string(c, "string", &val_str), "Failed to retrieve string 'string'");
    ASSERT_MSG(strcmp(val_str, "default_str") == 0, "String default value mismatch");

    cli_destroy(c);
}

static void test_missing_required() {
    cli c = cli_create();
    cli_option opt_int = {
        .option_name_short = "i",
        .option_name_long = "integer",
        .type = CLI_TYPE_INTEGER,
        .required = true
    };
    ASSERT_MSG(cli_add_option(c, opt_int), "Failed to add integer option");

    const char *argv[] = {"program"};
    int argc = 1;

    ASSERT_MSG(!cli_parse(c, argc, argv), "Parsing should fail when a required option is missing"); 
    cli_destroy(c);
}

static void test_float_and_path() {
    cli c = cli_create();

    cli_option opt_float = {
        .option_name_short = "f",
        .option_name_long = "float",
        .type = CLI_TYPE_FLOAT,
        .required = true
    };
    ASSERT(cli_add_option(c, opt_float));

    cli_option opt_path = {
        .option_name_short = "p",
        .option_name_long = "path",
        .type = CLI_TYPE_PATH,
        .required = true
    };
    ASSERT(cli_add_option(c, opt_path));

    const char *argv[] = {"program", "-f", "3.14", "--path", "/valid/path.txt"};
    int argc = 5;

    ASSERT_MSG(cli_parse(c, argc, argv), "Failed to parse float and path arguments");

    double val_f = 0.0;
    ASSERT(cli_get_float(c, "float", &val_f));
    ASSERT_MSG(val_f == 3.14, "Float value mismatch");

    const char *val_p = NULL;
    ASSERT(cli_get_string(c, "p", &val_p));
    ASSERT_MSG(strcmp(val_p, "/valid/path.txt") == 0, "Path value mismatch");

    cli_destroy(c);
}

static void test_invalid_types() {
    cli c = cli_create();
    
    cli_option opt_int = {
        .option_name_short = "i",
        .option_name_long = "integer",
        .type = CLI_TYPE_INTEGER,
        .required = true
    };
    ASSERT(cli_add_option(c, opt_int));

    const char *argv[] = {"program", "-i", "not_an_integer"};
    int argc = 3;

    ASSERT_MSG(!cli_parse(c, argc, argv), "Parsing should fail when passing string to integer option");

    cli_destroy(c);
}

static void test_default_values() {
    cli c = cli_create();
    
    cli_option opt_int = {
        .option_name_short = "i",
        .type = CLI_TYPE_INTEGER,
        .required = false,
        .default_value = { .long_value = 123 }
    };
    ASSERT(cli_add_option(c, opt_int));

    const char *argv[] = {"program"};
    int argc = 1;

    ASSERT_MSG(cli_parse(c, argc, argv), "Failed to parse with 0 arguments (using defaults)");

    long val_int = 0;
    ASSERT(cli_get_integer(c, "i", &val_int));
    ASSERT_MSG(val_int == 123, "Default integer value mismatch");

    cli_destroy(c);
}

int main() {
    test_basic_parsing();
    test_missing_required();
    test_float_and_path();
    test_invalid_types();
    test_default_values();
    
    return ASSERT_REPORT();
}
