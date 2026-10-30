#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "cli_arguments.h"
#include "string_utils.h"

// PARSER ERROR HANDLING

#define PARSER_ERR_DESC(XX) \
    XX(CLI_PARSE_ERROR_UNKNOWN_OPTION,          "Option unrecognized"                       ) \
    XX(CLI_PARSE_ERROR_MISSING_REQUIRED,        "Missing required parameter"                ) \
    XX(CLI_PARSE_ERROR_MISSING_VALUE,           "Missing value for required parameter"      ) \
    XX(CLI_PARSE_ERROR_NOT_A_CHOICE,            "Entered value is not a valid choice"       ) \
    XX(CLI_PARSE_ERROR_INVALID_VALUE,           "Invalid option value"                      )

static const char * err_desc(cli_error_code code) {
    switch (code) {
        #define XX(_code, _desc) case _code: return _desc;
        PARSER_ERR_DESC(XX)
        #undef XX
        default: return "Memory allocation error";
    }
}

// --------------------------------------------------------------------------------------------------

// MULTI-PARAMETER ERROR HANDLING

static bool embed_extract_required(cli c) {
    bool embed_set = false, extract_set = false;

    if (!cli_is_set(c, CLI_OPT_EMBED_LONG, &embed_set)) return false;
    if (!cli_is_set(c, CLI_OPT_EXTRACT_LONG, &extract_set)) return false;

    // Either embed or extract must be set
    return embed_set || extract_set;    
}

static bool embed_extract_mutual_exclusion(cli c) {
    bool embed_set = false, extract_set = false;

    if (!cli_is_set(c, CLI_OPT_EMBED_LONG, &embed_set)) return false;
    if (!cli_is_set(c, CLI_OPT_EXTRACT_LONG, &extract_set)) return false;

    // embed and extract cannot be set at the same time
    return !(embed_set && extract_set);    
}

static bool embed_in_defined(cli c) {
    bool embed_set = false, in_set = false;

    if (!cli_is_set(c, CLI_OPT_EMBED_LONG, &embed_set)) return false;
    if (!cli_is_set(c, CLI_OPT_IN_LONG, &in_set)) return false;

    return !embed_set || in_set;
}

static bool extract_in_undefined(cli c) {
    bool extract_set = false, in_set = false;

    if (!cli_is_set(c, CLI_OPT_EXTRACT_LONG, &extract_set)) return false;
    if (!cli_is_set(c, CLI_OPT_IN_LONG, &in_set)) return false;

    return !extract_set || !in_set;
}

static bool encryption_decryption_requires_password(cli c) {
    bool cipher_set = false, mode_set = false, password_set = false;

    if (!cli_is_set(c, CLI_OPT_CIPHER_ALG_LONG, &cipher_set)) return false;
    if (!cli_is_set(c, CLI_OPT_CIPHER_MODE_LONG, &mode_set)) return false;
    if (!cli_is_set(c, CLI_OPT_PASS_LONG, &password_set)) return false;

    return (!cipher_set && !mode_set) || password_set;
}

static bool bitmap_file_should_have_bmp_extension(cli c) {
    const char * picture_filename = nullptr;
    char buff[10];

    if (!cli_get_string(c, CLI_OPT_PICTURE_LONG, &picture_filename)) return false;
    if (!get_file_extension(picture_filename, true, buff, 10)) return false;

    return strncmp(buff, PICTURE_FILE_EXTENSION, 10) == 0;
}

typedef struct {
    bool (* fun) (cli c);
    const char * msg;
} multi_argument_validation_t;

static const multi_argument_validation_t MULTI_ARG_VALIDATIONS[] = {
    {
        .fun = embed_extract_required,             
        .msg = "Either '" CLI_OPT_EMBED_LONG "' or '" CLI_OPT_EXTRACT_LONG "' must be selected."              
    },
    {
        .fun = embed_extract_mutual_exclusion,     
        .msg = "Parameters '" CLI_OPT_EMBED_LONG "' and '" CLI_OPT_EXTRACT_LONG "' are mutually exclusive."
    },
    {
        .fun = embed_in_defined,
        .msg = "When using '" CLI_OPT_EMBED_LONG "', '" CLI_OPT_IN_LONG "' must be defined."
    },
    {
        .fun = extract_in_undefined,
        .msg = "When using '" CLI_OPT_EXTRACT_LONG "', '" CLI_OPT_IN_LONG "' must not be defined."
    },
    {
        .fun = encryption_decryption_requires_password,
        .msg = "When using '" CLI_OPT_CIPHER_ALG_SHORT "' or '" CLI_OPT_CIPHER_MODE_SHORT "', '" CLI_OPT_PASS_LONG "' must be defined."
    },
    {
        .fun = bitmap_file_should_have_bmp_extension,
        .msg = "Parameter '" CLI_OPT_PICTURE_LONG "' ('" CLI_OPT_PICTURE_SHORT "') must be a valid filename with extension '" PICTURE_FILE_EXTENSION "'."
    }
};

// --------------------------------------------------------------------------------------------------

static void show_err(cli c, const char * prog_name, const char * restrict fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n\n%s\n", cli_usage(c, prog_name));
    va_end(ap);
}

cli parse_command_line(int argc, const char * argv[]) {
    size_t size = sizeof(COMMAND_LINE_OPTIONS) / sizeof(COMMAND_LINE_OPTIONS[0]);
    bool has_error = false;
    
    // Initialize CLI parser
    cli c = cli_create();
    if (!c) {
        return nullptr;
    }

    // Load options
    for (size_t i = 0; i < size && !has_error; i++) {
        has_error = !cli_add_option(c, COMMAND_LINE_OPTIONS[i]);
    }

    // Parse
    if (!has_error) {
        has_error = !cli_parse(c, argc, argv, true);
    }

    // Show error messages and usage on parser failure
    if (has_error) {
        char * error_option = nullptr;
        cli_error_code error_code = cli_get_error(c, &error_option);
        
        if (error_option){
            show_err(c, argv[0], "Error:\n\t- %s\n\t- On parameter: %s\n", err_desc(error_code), error_option);
        }
        else {
            show_err(c, argv[0], "Error:\n\t- %s\n", err_desc(error_code));
        }

        cli_destroy(c);
        return nullptr;
    }

    // Multi-parameter checks
    size_t count = sizeof(MULTI_ARG_VALIDATIONS) / sizeof(multi_argument_validation_t);
    for (size_t i = 0; i < count; i++) {
        if (! MULTI_ARG_VALIDATIONS[i].fun(c)) {
            show_err(c, argv[0], "Error:\n\t- %s\n", MULTI_ARG_VALIDATIONS[i].msg);
            cli_destroy(c);
            return nullptr;
        }
    }

    return c;
}

void print_config(cli c) {
    if (!c) return;

    size_t size = sizeof(COMMAND_LINE_OPTIONS) / sizeof(COMMAND_LINE_OPTIONS[0]);
    printf("Configuration:\n");

    for (size_t i = 0; i < size; i++) {
        const cli_option * opt = &COMMAND_LINE_OPTIONS[i];
        const char * name = opt->option_name_long[0] != '\0' ? opt->option_name_long : opt->option_name_short;
        bool is_set = false;
        cli_is_set(c, name, &is_set);

        printf("  - %-18s : ", name);
        switch (opt->type) {
            case CLI_TYPE_FLAG: {
                bool val = false;
                if (cli_get_flag(c, name, &val)) {
                    printf("%s", val ? "true" : "false");
                }
                break;
            }
            case CLI_TYPE_INTEGER: {
                long val = 0;
                if (cli_get_integer(c, name, &val)) {
                    printf("%ld", val);
                }
                break;
            }
            case CLI_TYPE_FLOAT: {
                double val = 0.0;
                if (cli_get_float(c, name, &val)) {
                    printf("%f", val);
                }
                break;
            }
            case CLI_TYPE_STRING:
            case CLI_TYPE_PATH:
            case CLI_TYPE_CHOICE: {
                const char * val = nullptr;
                if (cli_get_string(c, name, &val)) {
                    printf("\"%s\"", val ? val : "(null)");
                }
                break;
            }
        }
        printf(" (%s)\n", is_set ? "set" : "default");
    }
}

bool verbose(cli c) {
    bool val = false, res = cli_is_set(c, CLI_OPT_VERBOSE_LONG, &val);
    return val && res;
}
