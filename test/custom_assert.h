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
    return ASSERT_REPORT();
}

*/
/* Output produced to stderr:

ASSERTION FAILED cli_test.c:15:
        - Condition: x == 2
ASSERTION FAILED cli_test.c:16:
        - Condition: fun() == 1
        - fun() should return 1

Results: 2 ran, 0 passed, 2 FAILED

*/

#ifndef _CUSTOM_ASSERT_H_
#define _CUSTOM_ASSERT_H_

#include <stdio.h>

static int _assert_total = 0;
static int _assert_passed = 0;
static int _assert_failed = 0;

#define ASSERT(condition) _DO_ASSERT(condition, __FILE__, __LINE__)

#define _DO_ASSERT(condition, file, line) \
    do { \
        _assert_total++; \
        if (!(condition)) { \
            _assert_failed++; \
            fprintf(stderr, "ASSERTION FAILED %s:%d:\n" \
                "\t- Condition: " #condition "\n", file, line); \
        } else { \
            _assert_passed++; \
        } \
    } while (0)

#define ASSERT_MSG(condition, message) _DO_ASSERT_MSG(condition, message, __FILE__, __LINE__)

#define _DO_ASSERT_MSG(condition, message, file, line) \
    do { \
        _assert_total++; \
        if (!(condition)) { \
            _assert_failed++; \
            fprintf(stderr, "ASSERTION FAILED %s:%d:\n" \
                            "\t- Condition: " #condition "\n" \
                            "\t- %s\n", file, line, message); \
        } else { \
            _assert_passed++; \
        } \
    } while (0)

#define ASSERT_REPORT() _assert_report()

static int _assert_report() {
    fprintf(stderr, "\nResults: %d ran, %d passed, %d failed\n", _assert_total, _assert_passed, _assert_failed);
    return _assert_failed > 0 ? 1 : 0;
}

#endif // _CUSTOM_ASSERT_H_
