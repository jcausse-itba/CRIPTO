/**
 * @file custom_assert.h
 * @brief Custom assertion macros for testing.
 */

/* Usage example:

int fun(){
    puts("Hello, World!");
    return 0;
}

int main(void) {
    int x = 5;
    ASSERT(x == 2);
    ASSERT_MSG(fun() == 1, "fun() should return 1");
    return 0;
}

*/
/* Output produced to stderr:

ASSERTION FAILED cli_test.c:11:
        - Condition: x == 2
ASSERTION FAILED cli_test.c:12:
        - Condition: fun() == 1
        - fun() should return 1

*/

#ifndef _CUSTOM_ASSERT_H_
#define _CUSTOM_ASSERT_H_

#include <stdio.h>

#define ASSERT(condition) _DO_ASSERT(condition, __FILE__, __LINE__)

#define _DO_ASSERT(condition, file, line) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "ASSERTION FAILED %s:%d:\n" \
                "\t- Condition: " #condition "\n", file, line); \
        } \
    } while (0)

#define ASSERT_MSG(condition, message) _DO_ASSERT_MSG(condition, message, __FILE__, __LINE__)

#define _DO_ASSERT_MSG(condition, message, file, line) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "ASSERTION FAILED %s:%d:\n" \
                            "\t- Condition: " #condition "\n" \
                            "\t- %s\n", file, line, message); \
        } \
    } while (0)

#endif // _CUSTOM_ASSERT_H_
