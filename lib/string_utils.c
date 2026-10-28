#include "string_utils.h"
#include <string.h>
#include <ctype.h>

/**
 * @brief       Compare two strings case-insensitively.
 * @details     This is needed because strcasecmp is POSIX, not standard ISO C.
 */
static bool str_case_equals(const char * a, const char * b) {
    while (*a && *b) {
        if (tolower((unsigned char) *a) != tolower((unsigned char) *b)) {
            return false;
        }
        a++;
        b++;
    }
    return *a == *b;
}

bool str_equals(const char * a, const char * b, bool case_insensitive) {
    if (a == b) {
        return true;
    }
    if (a == nullptr || b == nullptr) {
        return false;
    }
    return case_insensitive ? str_case_equals(a, b) : strcmp(a, b) == 0;
}

void get_file_extension(const char * restrict filename, bool lowercase, char * const out_extension, size_t max_size) {
    if (out_extension == nullptr || max_size == 0) {
        return;
    }

    out_extension[0] = '\0';

    if (filename == nullptr) {
        return;
    }

    // Find last directory separator. Can be / for UNIX-like systems, or \ for Windows systems.
    const char *slash = strrchr(filename, '/');
    const char *backslash = strrchr(filename, '\\');
    const char *sep = nullptr;

    if (slash != nullptr && backslash != nullptr) {
        sep = (slash > backslash) ? slash : backslash;
    } 
    else if (slash != nullptr) {
        sep = slash;
    } 
    else {
        sep = backslash;
    }

    // Find file's basename that starts after the last directory separator
    const char *basename = (sep != nullptr) ? sep + 1 : filename;

    if (strcmp(basename, ".") == 0 || strcmp(basename, "..") == 0) {
        return;
    }

    // Find file extension
    const char *dot = strrchr(basename, '.');
    if (dot == nullptr) {
        return;
    }

    // Copy file extension to output buffer
    size_t i = 0;
    while (dot[i] != '\0' && i + 1 < max_size) {
        out_extension[i] = lowercase ? (char)tolower((unsigned char)dot[i]) : dot[i];
        i++;
    }
    out_extension[i] = '\0';
}
