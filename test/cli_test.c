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

static void test_flag_options() {
    cli c = cli_create();

    cli_option opt_embed = {
        .option_name_short = "e",
        .option_name_long = "embed",
        .type = CLI_TYPE_FLAG,
        .required = false
    };
    ASSERT_MSG(cli_add_option(c, opt_embed), "Failed to add embed flag option");

    cli_option opt_extract = {
        .option_name_long = "extract",
        .type = CLI_TYPE_FLAG,
        .required = false
    };
    ASSERT_MSG(cli_add_option(c, opt_extract), "Failed to add extract flag option");

    // Test single-dash long flag (-embed) via getopt_long_only
    const char *argv[] = {"program", "-embed"};
    int argc = 2;

    ASSERT_MSG(cli_parse(c, argc, argv), "Failed to parse flag arguments");

    bool embed_val = false;
    ASSERT_MSG(cli_get_flag(c, "embed", &embed_val), "Failed to retrieve flag 'embed'");
    ASSERT_MSG(embed_val == true, "Flag 'embed' should be true when passed");

    bool extract_val = true;
    ASSERT_MSG(cli_get_flag(c, "extract", &extract_val), "Failed to retrieve flag 'extract'");
    ASSERT_MSG(extract_val == false, "Flag 'extract' should be false when omitted");

    cli_destroy(c);
}

static void test_choice_options() {
    cli c = cli_create();

    const char *steg_choices[] = {"LSB1", "LSB4", "LSBI", nullptr};
    cli_option opt_steg = {
        .option_name_short = "s",
        .option_name_long = "steg",
        .type = CLI_TYPE_CHOICE,
        .required = true,
        .choices = steg_choices
    };
    ASSERT_MSG(cli_add_option(c, opt_steg), "Failed to add steg choice option");

    const char *mode_choices[] = {"ecb", "cfb", "ofb", "cbc", nullptr};
    cli_option opt_mode = {
        .option_name_short = "m",
        .option_name_long = "mode",
        .type = CLI_TYPE_CHOICE,
        .required = false,
        .default_value = { .string_value = "cbc" },
        .choices = mode_choices
    };
    ASSERT_MSG(cli_add_option(c, opt_mode), "Failed to add mode choice option");

    const char *argv[] = {"program", "-steg", "LSBI"};
    int argc = 3;

    ASSERT_MSG(cli_parse(c, argc, argv), "Failed to parse valid choice argument");

    const char *val_steg = nullptr;
    ASSERT_MSG(cli_get_string(c, "steg", &val_steg), "Failed to retrieve choice 'steg' via cli_get_string");
    ASSERT_MSG(strcmp(val_steg, "LSBI") == 0, "Choice 'steg' value mismatch");

    const char *val_steg_short = nullptr;
    ASSERT_MSG(cli_get_string(c, "s", &val_steg_short), "Failed to retrieve choice 's' via cli_get_string");
    ASSERT_MSG(strcmp(val_steg_short, "LSBI") == 0, "Choice 's' value mismatch");

    const char *val_mode = nullptr;
    ASSERT_MSG(cli_get_string(c, "m", &val_mode), "Failed to retrieve default choice 'm'");
    ASSERT_MSG(strcmp(val_mode, "cbc") == 0, "Default choice 'm' value mismatch");

    cli_destroy(c);
}

static void test_invalid_choice() {
    cli c = cli_create();

    const char *empty_choices[] = {nullptr};
    cli_option opt_empty_choices = {
        .option_name_short = "e",
        .type = CLI_TYPE_CHOICE,
        .required = true,
        .choices = empty_choices
    };
    ASSERT_MSG(!cli_add_option(c, opt_empty_choices), "Adding choice option with empty choices should fail");

    cli_option opt_null_choices = {
        .option_name_short = "n",
        .type = CLI_TYPE_CHOICE,
        .required = true,
        .choices = nullptr
    };
    ASSERT_MSG(!cli_add_option(c, opt_null_choices), "Adding choice option with NULL choices should fail");

    const char *alg_choices[] = {"aes128", "aes192", "aes256", "3des", nullptr};
    cli_option opt_alg = {
        .option_name_short = "a",
        .option_name_long = "alg",
        .type = CLI_TYPE_CHOICE,
        .required = true,
        .choices = alg_choices
    };
    ASSERT_MSG(cli_add_option(c, opt_alg), "Failed to add alg choice option");

    const char *argv[] = {"program", "-a", "des"};
    int argc = 3;

    ASSERT_MSG(!cli_parse(c, argc, argv), "Parsing should fail when choice is not in allowed choices");

    cli_destroy(c);
}

int main() {
    test_basic_parsing();
    test_missing_required();
    test_float_and_path();
    test_invalid_types();
    test_default_values();
    test_flag_options();
    test_choice_options();
    test_invalid_choice();
    
    return ASSERT_REPORT();
}
