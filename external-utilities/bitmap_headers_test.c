#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include "bitmap_v3.h"

// gcc -Wall -Wextra -Werror -pedantic -std=c23 -Iinclude external-utilities/bitmap_headers_test.c -o bitmap_headers_test
// ./bitmap_headers_test
// rm bitmap_headers_test

int main() {
    printf("BITMAPFILEHEADER size: %lu\n", sizeof(BITMAPFILEHEADER));
    printf("BITMAPINFOHEADER size: %lu\n", sizeof(BITMAPINFOHEADER));

    assert(sizeof(BITMAPFILEHEADER) == 14 && sizeof(BITMAPINFOHEADER) == 40);

    puts("OK!");
}
