#include <stdio.h>
#include <stdlib.h>
#include "cli_arguments.h"
#include "io_utils.h"
#include "logger.h"

Logger logger = nullptr;              // Global Logger instance is defined here

static LoggerConfig logger_config = {
    .min_log_level = LOGGER_LEVEL_INFO,
    .with_level = true,
    .with_datetime = false,
    .flush_immediately = false,
    .log_prefix = nullptr
};

static bool bitmap_file_exists(cli c) {
    const char * picture_filename = nullptr;

    if (!cli_get_string(c, CLI_OPT_PICTURE_LONG, &picture_filename)) return false;

    if (!file_exists(picture_filename)){
        LOG_ERROR("\nError:\n\t- File not found: %s\n", picture_filename);
        return false;
    }
    return true;
}

static bool in_file_exists(cli c) {
    bool embed_set = false;                     // in is only defined when embed is set
    const char * in_filename = nullptr;

    if (!cli_is_set(c, CLI_OPT_EMBED_LONG, &embed_set)) return false;

    // If embed is not set, in should not be set, so return true
    if (!embed_set) return true;

    if (!cli_get_string(c, CLI_OPT_IN_LONG, &in_filename)) return false;

    if (!file_exists(in_filename)) {
        LOG_ERROR("\nError:\n\t- File not found: %s\n", in_filename);
        return false;
    }
    return true;
}

int main(int argc, const char * argv[]) {
    int exit_status = EXIT_FAILURE;

    cli c = nullptr;

    // Initialize the global Logger instance that will be used across all modules
    if(!(logger = Logger_create_from_stream(logger_config, stdout))) goto cleanup;
    
    // Parse command line arguments and show configuration
    if (!(c = parse_command_line(argc, argv))) goto cleanup;
    if (verbose(c)) print_config(c);

    // Check files existence
    bool bitmap_file_ok = bitmap_file_exists(c);
    bool in_file_ok = in_file_exists(c);
    if (!bitmap_file_ok || !in_file_ok) goto cleanup;


    // Do something


    // Set exit status, cleanup and exit gracefully
    exit_status = EXIT_SUCCESS;

cleanup:
    if (logger) Logger_destroy(logger);
    if (c) cli_destroy(c);

    return exit_status;
}
