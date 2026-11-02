#include "io_utils.h"
#include <stdio.h>

bool file_exists(const char *filename) {
    if (!filename) return false;
    FILE * file = fopen(filename, "r");
    if (file) fclose(file);
    return file != nullptr;
}
