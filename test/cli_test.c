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

    ASSERT_MSG(cli_parse(c, argc, argv, false), "Failed to parse valid basic arguments");

    char *err_opt = nullptr;
    ASSERT_MSG(cli_get_error(c, &err_opt) == CLI_PARSE_ERROR_NONE, "Error code should be NONE");
    ASSERT_MSG(err_opt == nullptr, "Error option should be NULL on success");

    long val_int = 0;
    ASSERT_MSG(cli_get_integer(c, "i", &val_int), "Failed to retrieve integer 'i'");
    ASSERT_MSG(val_int == 42, "Integer value mismatch");

    // Also test lookup with leading dashes
    val_int = 0;
    ASSERT_MSG(cli_get_integer(c, "-i", &val_int), "Failed to retrieve integer '-i'");
    ASSERT_MSG(val_int == 42, "Integer value mismatch via '-i'");

    bool int_is_set = false;
    ASSERT_MSG(cli_is_set(c, "--integer", &int_is_set), "Failed to check cli_is_set for '--integer'");
    ASSERT_MSG(int_is_set == true, "'--integer' should be marked as explicitly set");
    
    const char *val_str = NULL;
    // We should be able to query it using the long name too
    ASSERT_MSG(cli_get_string(c, "string", &val_str), "Failed to retrieve string 'string'");
    ASSERT_MSG(strcmp(val_str, "default_str") == 0, "String default value mismatch");

    bool str_is_set = true;
    ASSERT_MSG(cli_is_set(c, "string", &str_is_set), "Failed to check cli_is_set for 'string'");
    ASSERT_MSG(str_is_set == false, "'string' should NOT be marked as explicitly set when defaulted");

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

    ASSERT_MSG(!cli_parse(c, argc, argv, false), "Parsing should fail when a required option is missing");

    char *err_opt = nullptr;
    ASSERT_MSG(cli_get_error(c, &err_opt) == CLI_PARSE_ERROR_MISSING_REQUIRED, "Expected MISSING_REQUIRED error code");
    ASSERT_MSG(err_opt != nullptr && strcmp(err_opt, "integer") == 0, "Expected offending option to be 'integer'");

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

    ASSERT_MSG(cli_parse(c, argc, argv, true), "Failed to parse float and path arguments");

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

    char *err_opt = nullptr;

    const char *argv_str[] = {"program", "-i", "not_an_integer"};
    ASSERT_MSG(!cli_parse(c, 3, argv_str, false), "Parsing should fail when passing string to integer option");
    ASSERT_MSG(cli_get_error(c, &err_opt) == CLI_PARSE_ERROR_INVALID_VALUE, "Expected INVALID_VALUE error code");
    ASSERT_MSG(err_opt != nullptr && strcmp(err_opt, "integer") == 0, "Expected offending option 'integer'");

    const char *argv_empty[] = {"program", "-i", ""};
    ASSERT_MSG(!cli_parse(c, 3, argv_empty, false), "Parsing should fail when passing empty string to integer option");
    ASSERT_MSG(cli_get_error(c, &err_opt) == CLI_PARSE_ERROR_INVALID_VALUE, "Expected INVALID_VALUE error code on empty string");

    const char *argv_overflow[] = {"program", "-i", "9999999999999999999999999999999999999999"};
    ASSERT_MSG(!cli_parse(c, 3, argv_overflow, false), "Parsing should fail on integer overflow");
    ASSERT_MSG(cli_get_error(c, &err_opt) == CLI_PARSE_ERROR_INVALID_VALUE, "Expected INVALID_VALUE error code on overflow");

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

    ASSERT_MSG(cli_parse(c, argc, argv, true), "Failed to parse with 0 arguments (using defaults)");

    long val_int = 0;
    ASSERT(cli_get_integer(c, "i", &val_int));
    ASSERT_MSG(val_int == 123, "Default integer value mismatch");

    bool is_set = true;
    ASSERT(cli_is_set(c, "i", &is_set));
    ASSERT_MSG(is_set == false, "Defaulted option should have is_set == false");

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

    ASSERT_MSG(cli_parse(c, argc, argv, true), "Failed to parse flag arguments");

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

    ASSERT_MSG(cli_parse(c, argc, argv, true), "Failed to parse valid choice argument");

    const char *val_steg = nullptr;
    ASSERT_MSG(cli_get_string(c, "steg", &val_steg), "Failed to retrieve choice 'steg' via cli_get_string");
    ASSERT_MSG(strcmp(val_steg, "LSBI") == 0, "Choice 'steg' value mismatch");

    const char *val_steg_short = nullptr;
    ASSERT_MSG(cli_get_string(c, "s", &val_steg_short), "Failed to retrieve choice 's' via cli_get_string");
    ASSERT_MSG(strcmp(val_steg_short, "LSBI") == 0, "Choice 's' value mismatch");

    const char *val_mode = nullptr;
    ASSERT_MSG(cli_get_string(c, "m", &val_mode), "Failed to retrieve default choice 'm'");
    ASSERT_MSG(strcmp(val_mode, "cbc") == 0, "Default choice 'm' value mismatch");

    // Case-sensitive by default: lowercase "lsbi" must be rejected
    const char *argv_wrong_case[] = {"program", "-steg", "lsbi"};
    ASSERT_MSG(!cli_parse(c, 3, argv_wrong_case, true), "Default case-sensitive choice should reject wrong case");
    char *err_opt = nullptr;
    ASSERT_MSG(cli_get_error(c, &err_opt) == CLI_PARSE_ERROR_NOT_A_CHOICE, "Expected NOT_A_CHOICE on case mismatch");

    // Case-insensitive choice option: should match any casing and store canonical choice string
    const char *cipher_choices[] = {"AES128", "AES192", "AES256", "3DES", nullptr};
    cli_option opt_cipher = {
        .option_name_short = "a",
        .option_name_long = "alg",
        .type = CLI_TYPE_CHOICE,
        .required = false,
        .default_value = { .string_value = "AES128" },
        .choices = cipher_choices,
        .case_insensitive = true
    };
    ASSERT_MSG(cli_add_option(c, opt_cipher), "Failed to add case-insensitive cipher choice option");

    const char *argv_ci[] = {"program", "-steg", "LSB1", "-a", "aes256"};
    ASSERT_MSG(cli_parse(c, 5, argv_ci, true), "Case-insensitive choice should accept 'aes256' for 'AES256'");

    const char *val_cipher = nullptr;
    ASSERT_MSG(cli_get_string(c, "alg", &val_cipher), "Failed to retrieve choice 'alg'");
    ASSERT_MSG(strcmp(val_cipher, "AES256") == 0, "Expected canonical choice string 'AES256' to be stored");

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

    ASSERT_MSG(!cli_parse(c, argc, argv, true), "Parsing should fail when choice is not in allowed choices");

    char *err_opt = nullptr;
    ASSERT_MSG(cli_get_error(c, &err_opt) == CLI_PARSE_ERROR_NOT_A_CHOICE, "Expected NOT_A_CHOICE error code");
    ASSERT_MSG(err_opt != nullptr && strcmp(err_opt, "alg") == 0, "Expected offending option to be 'alg'");

    cli_destroy(c);
}

static void test_duplicate_and_invalid_option_names() {
    cli c = cli_create();

    cli_option opt_too_long_short = {
        .option_name_short = "ab",
        .type = CLI_TYPE_FLAG
    };
    ASSERT_MSG(!cli_add_option(c, opt_too_long_short), "Short option name > 1 char should fail");

    cli_option opt_first = {
        .option_name_short = "a",
        .option_name_long = "alpha",
        .type = CLI_TYPE_FLAG
    };
    ASSERT_MSG(cli_add_option(c, opt_first), "First valid option should succeed");

    cli_option opt_dup_short = {
        .option_name_short = "-a",
        .option_name_long = "another",
        .type = CLI_TYPE_FLAG
    };
    ASSERT_MSG(!cli_add_option(c, opt_dup_short), "Duplicate short option name should fail");

    cli_option opt_dup_long = {
        .option_name_short = "b",
        .option_name_long = "--alpha",
        .type = CLI_TYPE_FLAG
    };
    ASSERT_MSG(!cli_add_option(c, opt_dup_long), "Duplicate long option name should fail");

    cli_destroy(c);
}

static void test_error_reporting() {
    cli c = cli_create();

    cli_option opt_in = {
        .option_name_long = "in",
        .type = CLI_TYPE_PATH,
        .required = true
    };
    ASSERT_MSG(cli_add_option(c, opt_in), "Failed to add 'in' option");

    char *err_opt = nullptr;

    // 1. Unknown option when fail_on_unknown is true
    const char *argv_unknown[] = {"program", "--bogus", "-in", "file.txt"};
    ASSERT_MSG(!cli_parse(c, 4, argv_unknown, true), "Should fail on unknown option when fail_on_unknown is true");
    ASSERT_MSG(cli_get_error(c, &err_opt) == CLI_PARSE_ERROR_UNKNOWN_OPTION, "Expected CLI_PARSE_ERROR_UNKNOWN_OPTION");
    ASSERT_MSG(err_opt != nullptr && strcmp(err_opt, "bogus") == 0, "Expected offending option 'bogus'");

    // 2. Missing value for an option requiring an argument
    const char *argv_missing_val[] = {"program", "-in"};
    ASSERT_MSG(!cli_parse(c, 2, argv_missing_val, true), "Should fail when required option argument is missing");
    ASSERT_MSG(cli_get_error(c, &err_opt) == CLI_PARSE_ERROR_MISSING_VALUE, "Expected CLI_PARSE_ERROR_MISSING_VALUE");
    ASSERT_MSG(err_opt != nullptr && strcmp(err_opt, "in") == 0, "Expected offending option 'in'");

    cli_destroy(c);
}

static void test_usage_generation() {
    cli c = cli_create();

    cli_option opt_embed = {
        .option_name_short = "e",
        .option_name_long = "embed",
        .type = CLI_TYPE_FLAG,
        .required = false
    };
    ASSERT(cli_add_option(c, opt_embed));

    cli_option opt_in = {
        .option_name_long = "in",
        .type = CLI_TYPE_PATH,
        .required = true
    };
    ASSERT(cli_add_option(c, opt_in));

    const char *steg_choices[] = {"LSB1", "LSB4", "LSBI", nullptr};
    cli_option opt_steg = {
        .option_name_short = "s",
        .option_name_long = "steg",
        .type = CLI_TYPE_CHOICE,
        .required = true,
        .choices = steg_choices
    };
    ASSERT(cli_add_option(c, opt_steg));

    const char *usage = cli_usage(c, "stegobmp");
    ASSERT_MSG(usage != nullptr, "cli_usage should return a non-NULL string");
    ASSERT_MSG(
        strcmp(usage, "Usage: stegobmp [-e|--embed] --in <path> -s|--steg <LSB1|LSB4|LSBI>\n") == 0,
        "Generated usage string did not match expected format"
    );

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
    test_duplicate_and_invalid_option_names();
    test_error_reporting();
    test_usage_generation();
    
    return ASSERT_REPORT();
}
