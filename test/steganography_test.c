#include "steganography.h"
#include "bitmap.h"
#include "bitmap_v3.h"
#include "custom_assert.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Helper to create a synthetic 24bpp uncompressed V3 BMP file with known carrier bytes.
 * Width = 4 -> 12 bytes/row (already a multiple of 4, so stride = 12).
 * Height = 4 -> 48 total carrier bytes.
 */
static bool create_dummy_bmp(const char * path, uint8_t fill_byte) {
    FILE * f = fopen(path, "wb");
    if (!f) return false;

    const uint32_t width = 4;
    const uint32_t height = 4;
    const uint32_t data_size = width * 3 * height; // 48 bytes

    BITMAPFILEHEADER fh = {
        .bfType = 0x4D42,
        .bfSize = 54 + data_size,
        .bfReserved1 = 0,
        .bfReserved2 = 0,
        .bfOffBits = 54
    };

    BITMAPINFOHEADER ih = {
        .biSize = 40,
        .biWidth = (int32_t) width,
        .biHeight = (int32_t) height,
        .biPlanes = 1,
        .biBitCount = 24,
        .biCompression = 0,
        .biSizeImage = data_size,
        .biXPelsPerMeter = 0,
        .biYPelsPerMeter = 0,
        .biClrUsed = 0,
        .biClrImportant = 0
    };

    fwrite(&fh, sizeof(fh), 1, f);
    fwrite(&ih, sizeof(ih), 1, f);

    for (uint32_t i = 0; i < data_size; i++) {
        fwrite(&fill_byte, 1, 1, f);
    }

    fclose(f);
    return true;
}

static void test_alg_string_conversion(void) {
    stego_alg_t alg;

    ASSERT(stego_alg_from_string("LSB1", &alg) && alg == STEGO_ALG_LSB1);
    ASSERT(stego_alg_from_string("lsb1", &alg) && alg == STEGO_ALG_LSB1);
    ASSERT(stego_alg_from_string("LSB4", &alg) && alg == STEGO_ALG_LSB4);
    ASSERT(stego_alg_from_string("lsb4", &alg) && alg == STEGO_ALG_LSB4);
    ASSERT(stego_alg_from_string("LSBI", &alg) && alg == STEGO_ALG_LSBI);
    ASSERT(stego_alg_from_string("lsbi", &alg) && alg == STEGO_ALG_LSBI);

    ASSERT(!stego_alg_from_string("LSB2", &alg));
    ASSERT(!stego_alg_from_string(nullptr, &alg));
    ASSERT(!stego_alg_from_string("LSB1", nullptr));

    ASSERT(strcmp(stego_alg_to_string(STEGO_ALG_LSB1), "LSB1") == 0);
    ASSERT(strcmp(stego_alg_to_string(STEGO_ALG_LSB4), "LSB4") == 0);
    ASSERT(strcmp(stego_alg_to_string(STEGO_ALG_LSBI), "LSBI") == 0);
}

static void test_max_capacity(void) {
    const char * path = "/tmp/test_stego_cap.bmp";
    ASSERT(create_dummy_bmp(path, 0xAA));

    bitmap bmp = bitmap_create(path);
    ASSERT(bmp != nullptr);
    if (bmp) {
        // 48 carrier bytes:
        // LSB1: 48 / 8 = 6 bytes
        // LSB4: 48 / 2 = 24 bytes
        // LSBI: (48 - 4) / 8 = 5 bytes
        ASSERT(bitmap_get_total_bytes(bmp) == 48);
        ASSERT(stego_get_max_capacity(STEGO_ALG_LSB1, bmp) == 6);
        ASSERT(stego_get_max_capacity(STEGO_ALG_LSB4, bmp) == 24);
        ASSERT(stego_get_max_capacity(STEGO_ALG_LSBI, bmp) == 5);
        bitmap_destroy(bmp);
    }

    remove(path);
}

static void test_lsb1_bit_order_and_roundtrip(void) {
    const char * path = "/tmp/test_stego_lsb1.bmp";
    // Fill carrier with 0xAA (10101010 in binary)
    ASSERT(create_dummy_bmp(path, 0xAA));

    bitmap bmp = bitmap_create(path);
    ASSERT(bmp != nullptr);
    if (bmp) {
        // Embed 2 bytes: 0xA5 (10100101) and 0x3C (00111100)
        const uint8_t payload[2] = { 0xA5, 0x3C };
        ASSERT(stego_embed(STEGO_ALG_LSB1, bmp, payload, sizeof(payload)));

        // Verify exact bit order in the first 8 carrier bytes for 0xA5 (1,0,1,0,0,1,0,1)
        // Upper 7 bits of 0xAA (1010101x) must remain intact:
        // bit=1 -> 0xAB, bit=0 -> 0xAA
        bitmap_rewind(bmp);
        const uint8_t expected_carrier_0xA5[8] = {
            0xAB, 0xAA, 0xAB, 0xAA, 0xAA, 0xAB, 0xAA, 0xAB
        };
        for (int i = 0; i < 8; i++) {
            uint8_t c = 0;
            ASSERT(bitmap_get_byte(bmp, &c));
            ASSERT(c == expected_carrier_0xA5[i]);
        }

        // Rewind and extract the 2 payload bytes
        bitmap_rewind(bmp);
        uint8_t extracted[2] = {0};
        ASSERT(stego_extract_bytes(STEGO_ALG_LSB1, bmp, extracted, sizeof(extracted)));
        ASSERT(extracted[0] == 0xA5);
        ASSERT(extracted[1] == 0x3C);

        // Verify exceeding capacity fails (max is 6 bytes)
        uint8_t too_large[7] = {0};
        ASSERT(!stego_embed(STEGO_ALG_LSB1, bmp, too_large, sizeof(too_large)));

        bitmap_destroy(bmp);
    }

    remove(path);
}

static void test_lsb4_nibble_order_and_roundtrip(void) {
    const char * path = "/tmp/test_stego_lsb4.bmp";
    // Fill carrier with 0xC0 (11000000 in binary)
    ASSERT(create_dummy_bmp(path, 0xC0));

    bitmap bmp = bitmap_create(path);
    ASSERT(bmp != nullptr);
    if (bmp) {
        // Embed 3 bytes: 0x5A, 0xF0, 0x09
        const uint8_t payload[3] = { 0x5A, 0xF0, 0x09 };
        ASSERT(stego_embed(STEGO_ALG_LSB4, bmp, payload, sizeof(payload)));

        // Verify exact nibble order in the first 6 carrier bytes:
        // Upper 4 bits (0xC0) must remain intact; high nibble first, low nibble second
        bitmap_rewind(bmp);
        const uint8_t expected_carrier[6] = {
            0xC5, 0xCA, // 0x5A
            0xCF, 0xC0, // 0xF0
            0xC0, 0xC9  // 0x09
        };
        for (int i = 0; i < 6; i++) {
            uint8_t c = 0;
            ASSERT(bitmap_get_byte(bmp, &c));
            ASSERT(c == expected_carrier[i]);
        }

        // Rewind and extract incrementally (1 byte, then 2 bytes)
        bitmap_rewind(bmp);
        uint8_t extracted[3] = {0};
        ASSERT(stego_extract_bytes(STEGO_ALG_LSB4, bmp, &extracted[0], 1));
        ASSERT(stego_extract_bytes(STEGO_ALG_LSB4, bmp, &extracted[1], 2));
        ASSERT(memcmp(extracted, payload, sizeof(payload)) == 0);

        // Verify exceeding capacity fails (max is 24 bytes)
        uint8_t too_large[25] = {0};
        ASSERT(!stego_embed(STEGO_ALG_LSB4, bmp, too_large, sizeof(too_large)));

        bitmap_destroy(bmp);
    }

    remove(path);
}

static void test_full_payload_structure_roundtrip(void) {
    // Test the complete assignment payload structure:
    // 4-byte Big-Endian size (5) || "HELLO" || ".txt\0" (total 14 bytes)
    // Using LSB4 on our 48-byte carrier (capacity 24 bytes)
    const char * path = "/tmp/test_stego_full.bmp";
    const char * out_path = "/tmp/test_stego_full_out.bmp";
    ASSERT(create_dummy_bmp(path, 0x88));

    const uint8_t full_payload[14] = {
        0x00, 0x00, 0x00, 0x05,         // Big-Endian uint32: 5
        'H',  'E',  'L',  'L',  'O',    // 5 file bytes
        '.',  't',  'x',  't',  '\0'    // null-terminated extension
    };

    bitmap bmp = bitmap_create(path);
    ASSERT(bmp != nullptr);
    if (bmp) {
        ASSERT(stego_embed(STEGO_ALG_LSB4, bmp, full_payload, sizeof(full_payload)));
        ASSERT(bitmap_save(bmp, out_path));
        bitmap_destroy(bmp);
    }

    // Reload saved bitmap and extract incrementally just like extract_main will do
    bitmap reloaded = bitmap_create(out_path);
    ASSERT(reloaded != nullptr);
    if (reloaded) {
        uint8_t size_be[4] = {0};
        ASSERT(stego_extract_bytes(STEGO_ALG_LSB4, reloaded, size_be, 4));
        uint32_t file_size = ((uint32_t)size_be[0] << 24) |
                             ((uint32_t)size_be[1] << 16) |
                             ((uint32_t)size_be[2] << 8)  |
                             ((uint32_t)size_be[3]);
        ASSERT(file_size == 5);

        uint8_t content[5] = {0};
        ASSERT(stego_extract_bytes(STEGO_ALG_LSB4, reloaded, content, file_size));
        ASSERT(memcmp(content, "HELLO", 5) == 0);

        char ext[16] = {0};
        size_t ext_idx = 0;
        do {
            ASSERT(stego_extract_bytes(STEGO_ALG_LSB4, reloaded, (uint8_t *)&ext[ext_idx], 1));
        } while (ext[ext_idx++] != '\0' && ext_idx < sizeof(ext));
        ASSERT(strcmp(ext, ".txt") == 0);

        bitmap_destroy(reloaded);
    }

    remove(path);
    remove(out_path);
}

int main(void) {
    test_alg_string_conversion();
    test_max_capacity();
    test_lsb1_bit_order_and_roundtrip();
    test_lsb4_nibble_order_and_roundtrip();
    test_full_payload_structure_roundtrip();

    return ASSERT_REPORT();
}
