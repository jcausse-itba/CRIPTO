#include <stdio.h>
#include "io_utils.h"
#include "cli_arguments.h"
#include "logger.h"

extern Logger logger;

bool get_file_size(const char * restrict filename, size_t * out_size) {
    if (!filename) return false;

    // Open file
    FILE * file = fopen(filename, "rb");
    if (!file) return false;

    // Calculate the file's size (passing nullptr skips this step)
    if (out_size) {
        // Move the file's pointer to the end
        if (fseek(file, 0, SEEK_END) != 0) {
            fclose(file);
            return false;
        }
        // Calculate the size using the current pointer of the stream
        *out_size = ftell(file);
    }

    fclose(file);
    return true;
}

bool bitmap_file_exists(cli c) {
    const char * picture_filename = nullptr;

    if (!cli_get_string(c, CLI_OPT_PICTURE_LONG, &picture_filename)) return false;

    if (!get_file_size(picture_filename, nullptr)) {
        LOG_ERROR("\nError:\n\t- File not found: %s\n", picture_filename);
        return false;
    }
    return true;
}

bool in_file_exists(cli c, size_t * out_size) {
    bool embed_set = false;                     // in is only defined when embed is set
    const char * in_filename = nullptr;

    if (!cli_is_set(c, CLI_OPT_EMBED_LONG, &embed_set)) return false;

    // If embed is not set, in should not be set, so return true
    if (!embed_set) return true;

    if (!cli_get_string(c, CLI_OPT_IN_LONG, &in_filename)) return false;

    if (!get_file_size(in_filename, out_size)) {
        LOG_ERROR("\nError:\n\t- File not found: %s\n", in_filename);
        return false;
    }
    return true;
}
