#include <stdio.h>
#include <stdlib.h>
#include "cli_arguments.h"
#include "io_utils.h"
#include "logger.h"
#include "embed.h"
#include "extract.h"

Logger logger = nullptr;              // Global Logger instance is defined here

static LoggerConfig logger_config = {
    .min_log_level = LOGGER_LEVEL_INFO,
    .with_level = true,
    .with_datetime = false,
    .with_color = true,
    .flush_immediately = false,
    .log_prefix = nullptr
};

int main(int argc, const char * argv[]) {
    int exit_status = EXIT_FAILURE;

    cli c = nullptr;

    // Initialize the global Logger instance that will be used across all modules
    if(!(logger = Logger_create_from_stream(logger_config, stdout))) goto cleanup;
    
    // Parse command line arguments and show configuration
    if (!(c = parse_command_line(argc, argv))) goto cleanup;                            // Logs errors
    bool verbose = is_verbose_set(c);
    if (verbose) print_config(c);                                                       // Uses logger

    // Check files existence
    size_t in_file_size = 0;                                                            // Only used for -embed
    bool bitmap_file_ok = bitmap_file_exists(c);                                        // Logs errors
    bool in_file_ok = in_file_exists(c, &in_file_size);                                 // Logs errors
    if (!bitmap_file_ok || !in_file_ok) goto cleanup;

    // Run the selected task
    bool is_embed = false;
    if (!cli_get_flag(c, CLI_OPT_EMBED_LONG, &is_embed)){
        LOG_ERROR("Error getting option: %s", CLI_OPT_EMBED_LONG);
        goto cleanup;
    }
    exit_status = is_embed 
        ? embed_main(c, verbose, in_file_size)                                          // Logs errors
        : extract_main(c, verbose);                                                     // Logs errors

cleanup:
    if (logger) Logger_destroy(logger);
    if (c) cli_destroy(c);

    return exit_status;
}
