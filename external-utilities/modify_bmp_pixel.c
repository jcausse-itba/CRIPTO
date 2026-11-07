/*
gcc -Wall -Wextra -Werror -pedantic -std=c23 -Iinclude external-utilities/modify_bmp_pixel.c -o modify_bmp_pixel &&
./modify_bmp_pixel &&
rm modify_bmp_pixel
*/

// C version of modify_bmp_pixel.py

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#ifndef FILENAME
#define FILENAME "images/cat_v3.bmp" // 700 x 700 BMP V3
#endif

#ifndef HEIGHT
#define HEIGHT 700
#endif

#ifndef WIDTH
#define WIDTH 700
#endif

typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} Pixel;

static inline Pixel pixel_negative_of(Pixel p) {
    return (Pixel){
        .red = (uint8_t)(255 - p.red),
        .green = (uint8_t)(255 - p.green),
        .blue = (uint8_t)(255 - p.blue),
    };
}

Pixel read_bmp_pixel(FILE *file, int i, int j, int width) {
    long offset = 3L * (-(long)(i + 1) * width + j);
    if (fseek(file, offset, SEEK_END) != 0) {
        perror("fseek failed in read_bmp_pixel");
        return (Pixel){0, 0, 0};
    }

    uint8_t pixel_data[3];
    if (fread(pixel_data, sizeof(uint8_t), 3, file) != 3) {
        perror("fread failed in read_bmp_pixel");
        return (Pixel){0, 0, 0};
    }

    return (Pixel){
        .red = pixel_data[2],
        .green = pixel_data[1],
        .blue = pixel_data[0]
    };
}

void modify_bmp_pixel(FILE *file, int i, int j, int width, Pixel p) {
    long offset = 3L * (-(long)(i + 1) * width + j);
    if (fseek(file, offset, SEEK_END) != 0) {
        perror("fseek failed in modify_bmp_pixel");
        return;
    }

    uint8_t pixel_data[3] = { p.blue, p.green, p.red };
    if (fwrite(pixel_data, sizeof(uint8_t), 3, file) != 3) {
        perror("fwrite failed in modify_bmp_pixel");
        return;
    }
}

int main(void) { // Test program to turn an image to its negative
    FILE *file = fopen(FILENAME, "r+b");
    if (!file) {
        perror("Failed to open file " FILENAME);
        return 1;
    }

    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            Pixel p = read_bmp_pixel(file, i, j, WIDTH);
            modify_bmp_pixel(file, i, j, WIDTH, pixel_negative_of(p));
        }
    }

    fclose(file);
    return 0;
}
