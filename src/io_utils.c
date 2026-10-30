#include "io_utils.h"
#include <stdio.h>

void show_error(const char * restrict format, ...) {
    va_list ap;
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
}

bool file_exists(const char *filename) {
    if (!filename) return false;
    FILE * file = fopen(filename, "r");
    if (file) fclose(file);
    return file != nullptr;
}
