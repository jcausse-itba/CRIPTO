#include <stdio.h>
#include "cli.h"

static const cli_option COMMAND_LINE_OPTIONS[] = {
    {
        .option_name_short = "v",
        .option_name_long = "verbose",
        .type = CLI_TYPE_FLAG,
    },
    {
        .option_name_short = "n",
        .option_name_long = "name",
        .type = CLI_TYPE_STRING,
        .required = true
    }
};

static cli parse_command_line(int argc, const char * argv[]);
static char * get_error_description(cli_error_code code);

int main(int argc, const char *argv[]) {
    cli c = parse_command_line(argc, argv);

    // Do something

    if (c) {
        cli_destroy(c);
    }

    return 0;
}

static cli parse_command_line(int argc, const char * argv[]) {
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

    // Show error messages and usage on failure
    if (has_error) {
        char * error_option = nullptr;
        cli_error_code error_code = cli_get_error(c, &error_option);
        
        fprintf(
            stderr, 
            "Error:\n\t- %s\n\t- On parameter: %s\n",
            get_error_description(error_code),
            error_option
        );
        putchar('\n');
        fprintf(stderr, "%s\n", cli_usage(c, argv[0]));

        cli_destroy(c);
        return nullptr;
    }

    return c;
}

static char * get_error_description(cli_error_code code) {
    switch (code) {
        case CLI_PARSE_ERROR_UNKNOWN_OPTION:
            return "Option unrecognized";
        case CLI_PARSE_ERROR_MISSING_REQUIRED:
            return "Missing required parameter";
        case CLI_PARSE_ERROR_MISSING_VALUE:
            return "Missing value for required parameter";
        case CLI_PARSE_ERROR_NOT_A_CHOICE:
            return "Entered value is not a valid choice";
        case CLI_PARSE_ERROR_INVALID_VALUE:
            return "Invalid option value";
        default:
            return "Memory allocation error";
    }
}
