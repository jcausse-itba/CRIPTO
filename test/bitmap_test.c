#include "bitmap.h"
#include "bitmap_v3.h"
#include "custom_assert.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char * find_cat_bmp(void) {
    FILE * f = fopen("images/cat_v3.bmp", "rb");
    if (f) { fclose(f); return "images/cat_v3.bmp"; }
    f = fopen("../images/cat_v3.bmp", "rb");
    if (f) { fclose(f); return "../images/cat_v3.bmp"; }
    return "images/cat_v3.bmp";
}

static void test_open_valid_bitmap() {
    const char * cat_path = find_cat_bmp();
    bitmap b = bitmap_create(cat_path);
    ASSERT_MSG(b != nullptr, "bitmap_create should succeed for images/cat_v3.bmp");
    if (!b) return;

    ASSERT(bitmap_get_total_bytes(b) == 700 * 700 * 3);
    ASSERT(bitmap_get_cursor(b) == 0);

    // Read first few bytes
    uint8_t first_bytes[6];
    for (int i = 0; i < 6; i++) {
        bool ok = bitmap_get_byte(b, &first_bytes[i]);
        ASSERT(ok == true);
    }
    ASSERT(bitmap_get_cursor(b) == 6);

    // Rewind and verify first bytes match
    bitmap_rewind(b);
    ASSERT(bitmap_get_cursor(b) == 0);
    for (int i = 0; i < 6; i++) {
        uint8_t byte = 0;
        bool ok = bitmap_get_byte(b, &byte);
        ASSERT(ok == true && byte == first_bytes[i]);
    }

    bitmap_destroy(b);
}

static void test_invalid_files() {
    ASSERT(bitmap_create(nullptr) == nullptr);
    ASSERT(bitmap_create("non_existent_file.bmp") == nullptr);

    const char * dummy_path = "/tmp/test_invalid.bmp";
    FILE * f = fopen(dummy_path, "wb");
    if (f) {
        uint8_t garbage[54] = {0};
        fwrite(garbage, 1, 54, f);
        fclose(f);

        bitmap b = bitmap_create(dummy_path);
        ASSERT_MSG(b == nullptr, "Should reject file with invalid magic signature");
        remove(dummy_path);
    }
}

static void test_padding_included_and_modification() {
    // 5x2 24bpp BMP: width=5 -> 15 pixel bytes/row, stride 16 (1 padding byte/row)
    const char * path = "/tmp/test_padding.bmp";
    const char * out_path = "/tmp/test_padding_out.bmp";

    FILE * f = fopen(path, "wb");
    ASSERT(f != nullptr);
    if (!f) return;

    BITMAPFILEHEADER fh = {
        .bfType = 0x4D42,
        .bfSize = 54 + 16 * 2,
        .bfReserved1 = 0,
        .bfReserved2 = 0,
        .bfOffBits = 54
    };

    BITMAPINFOHEADER ih = {
        .biSize = 40,
        .biWidth = 5,
        .biHeight = 2,
        .biPlanes = 1,
        .biBitCount = 24,
        .biCompression = 0,
        .biSizeImage = 16 * 2,
        .biXPelsPerMeter = 0,
        .biYPelsPerMeter = 0,
        .biClrUsed = 0,
        .biClrImportant = 0
    };

    fwrite(&fh, sizeof(fh), 1, f);
    fwrite(&ih, sizeof(ih), 1, f);

    // Row 0: 15 bytes + 1 padding byte (0xEE)
    uint8_t row0[16];
    for (int i = 0; i < 15; i++) row0[i] = (uint8_t)(0x10 + i);
    row0[15] = 0xEE; // padding
    fwrite(row0, 1, 16, f);

    // Row 1: 15 bytes + 1 padding byte (0xEE)
    uint8_t row1[16];
    for (int i = 0; i < 15; i++) row1[i] = (uint8_t)(0x20 + i);
    row1[15] = 0xEE; // padding
    fwrite(row1, 1, 16, f);

    fclose(f);

    bitmap b = bitmap_create(path);
    ASSERT(b != nullptr);
    if (b) {
        // Total bytes includes padding (16 * 2 = 32) per Enunciado.pdf Section 5.4
        ASSERT(bitmap_get_total_bytes(b) == 32);

        // Read all 32 bytes sequentially and check values
        uint8_t bytes[32];
        size_t count = 0;
        uint8_t byte = 0;
        while (bitmap_get_byte(b, &byte)) {
            if (count < 32) bytes[count] = byte;
            count++;
        }
        ASSERT(count == 32);

        for (int i = 0; i < 15; i++) ASSERT(bytes[i] == (uint8_t)(0x10 + i));
        ASSERT(bytes[15] == 0xEE);
        for (int i = 0; i < 15; i++) ASSERT(bytes[16 + i] == (uint8_t)(0x20 + i));
        ASSERT(bytes[31] == 0xEE);

        // Rewind and modify the first byte via next_byte
        bitmap_rewind(b);
        uint8_t * p0 = bitmap_next_byte(b);
        ASSERT(p0 != nullptr && *p0 == 0x10);
        *p0 = 0xAA;

        // Advance to row 0 padding byte (index 15) and modify it via get_byte + set_byte
        for (int i = 1; i < 15; i++) {
            ASSERT(bitmap_get_byte(b, &byte));
        }
        ASSERT(bitmap_get_byte(b, &byte) && byte == 0xEE);
        ASSERT(bitmap_set_byte(b, 0xCC));

        // Next byte is row 1 pixel 0 (index 16)
        ASSERT(bitmap_get_byte(b, &byte) && byte == 0x20);
        ASSERT(bitmap_set_byte(b, 0xBB));

        // Save modified bitmap
        ASSERT(bitmap_save(b, out_path) == true);

        bitmap_destroy(b);
    }

    // Verify saved file directly:
    FILE * f_out = fopen(out_path, "rb");
    ASSERT(f_out != nullptr);
    if (f_out) {
        uint8_t buf[54 + 16 * 2];
        size_t read_bytes = fread(buf, 1, sizeof(buf), f_out);
        ASSERT(read_bytes == sizeof(buf));

        // Pixel 0 modified
        ASSERT(buf[54 + 0] == 0xAA);
        // Padding byte of row 0 modified
        ASSERT(buf[54 + 15] == 0xCC);
        // Pixel 0 of row 1 modified
        ASSERT(buf[54 + 16] == 0xBB);
        // Padding byte of row 1 still 0xEE
        ASSERT(buf[54 + 31] == 0xEE);

        fclose(f_out);
    }

    remove(path);
    remove(out_path);
}

int main(void) {
    test_open_valid_bitmap();
    test_invalid_files();
    test_padding_included_and_modification();

    return ASSERT_REPORT();
}
