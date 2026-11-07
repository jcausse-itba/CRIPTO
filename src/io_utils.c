#include "io_utils.h"
#include <stdio.h>

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
