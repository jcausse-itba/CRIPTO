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
    return case_insensitive ? str_case_equals(a, b) : strcmp(a, b) == 0;
}
