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
        // LSBI: rem = 48 - 4 = 44; skipped_red = (44 + 1) / 3 = 15; usable_bits = 29; 29 / 8 = 3 bytes
        ASSERT(bitmap_get_total_bytes(bmp) == 48);
        ASSERT(stego_get_max_capacity(STEGO_ALG_LSB1, bmp) == 6);
        ASSERT(stego_get_max_capacity(STEGO_ALG_LSB4, bmp) == 24);
        ASSERT(stego_get_max_capacity(STEGO_ALG_LSBI, bmp) == 3);
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

static void test_lsbi_inversion_and_roundtrip(void) {
    const char * path = "/tmp/test_stego_lsbi.bmp";
    ASSERT(create_dummy_bmp(path, 0x00));

    bitmap bmp = bitmap_create(path);
    ASSERT(bmp != nullptr);
    if (bmp) {
        // Prepare custom carrier bytes to reproduce the Enunciado.pdf page 5 example:
        // Patterns '00' and '10' have changed > unchanged -> header LSBs must be 1, 0, 1, 0
        // and all Red channel bytes after the 4 header bytes (indices 5, 8, 11, 14) must be untouched (0xFF).
        const uint8_t custom_carrier[16] = {
            0xA8, // [0] B0 (header bit 0 for pattern '00')
            0xA9, // [1] G0 (header bit 1 for pattern '01')
            0xAA, // [2] R0 (header bit 2 for pattern '10' - uses Red per Enunciado!)
            0xAB, // [3] B1 (header bit 3 for pattern '11')
            0x04, // [4] G1: pattern '10', orig LSB 0 -> payload bit 7 (1) -> changed
            0xFF, // [5] R1: Red channel -> MUST BE SKIPPED
            0x05, // [6] B2: pattern '10', orig LSB 1 -> payload bit 6 (0) -> changed
            0x05, // [7] G2: pattern '10', orig LSB 1 -> payload bit 5 (1) -> unchanged
            0xFF, // [8] R2: Red channel -> MUST BE SKIPPED
            0x03, // [9] B3: pattern '01', orig LSB 1 -> payload bit 4 (1) -> unchanged
            0x00, // [10] G3: pattern '00', orig LSB 0 -> payload bit 3 (1) -> changed
            0xFF, // [11] R3: Red channel -> MUST BE SKIPPED
            0x00, // [12] B4: pattern '00', orig LSB 0 -> payload bit 2 (1) -> changed
            0x00, // [13] G4: pattern '00', orig LSB 0 -> payload bit 1 (0) -> unchanged
            0xFF, // [14] R4: Red channel -> MUST BE SKIPPED
            0x06  // [15] B5: pattern '11', orig LSB 0 -> payload bit 0 (0) -> unchanged
        };

        bitmap_rewind(bmp);
        for (int i = 0; i < 16; i++) {
            uint8_t * p = bitmap_next_byte(bmp);
            ASSERT(p != nullptr);
            *p = custom_carrier[i];
        }

        // Embed 3 bytes: first byte 0xBC (10111100), followed by 0x5A, 0x3C
        // Note: bytes 1 and 2 fall on remaining 0x00 carrier bytes (pattern '00').
        // For 0x5A (01011010, four 1s and four 0s) and 0x3C (00111100, four 1s and four 0s),
        // pattern '00' gets +8 changed and +8 unchanged, so total for '00' is 10 changed > 9 unchanged!
        const uint8_t payload[3] = { 0xBC, 0x5A, 0x3C };
        ASSERT(stego_embed(STEGO_ALG_LSBI, bmp, payload, sizeof(payload)));

        // Verify the first 16 carrier bytes after LSBI embedding:
        // - Header bytes [0..3] must have LSBs 1, 0, 1, 0 (patterns '00' and '10' inverted)
        // - Red bytes [5, 8, 11, 14] must remain 0xFF
        // - Payload bytes with patterns '00' and '10' have their LSBs inverted
        const uint8_t expected_first_16[16] = {
            0xA9, // [0] B0: LSB = 1 ('00' inverted)
            0xA8, // [1] G0: LSB = 0 ('01' not inverted)
            0xAB, // [2] R0: LSB = 1 ('10' inverted)
            0xAA, // [3] B1: LSB = 0 ('11' not inverted)
            0x04, // [4] G1: pattern '10', bit 1 inverted -> 0
            0xFF, // [5] R1: untouched
            0x05, // [6] B2: pattern '10', bit 0 inverted -> 1
            0x04, // [7] G2: pattern '10', bit 1 inverted -> 0
            0xFF, // [8] R2: untouched
            0x03, // [9] B3: pattern '01', bit 1 not inverted -> 1
            0x00, // [10] G3: pattern '00', bit 1 inverted -> 0
            0xFF, // [11] R3: untouched
            0x00, // [12] B4: pattern '00', bit 1 inverted -> 0
            0x01, // [13] G4: pattern '00', bit 0 inverted -> 1
            0xFF, // [14] R4: untouched
            0x06  // [15] B5: pattern '11', bit 0 not inverted -> 0
        };

        bitmap_rewind(bmp);
        for (int i = 0; i < 16; i++) {
            uint8_t c = 0;
            ASSERT(bitmap_get_byte(bmp, &c));
            ASSERT(c == expected_first_16[i]);
        }

        // Rewind and extract incrementally (1 byte, then 2 bytes) to verify stateful cursor handling
        bitmap_rewind(bmp);
        uint8_t extracted[3] = {0};
        ASSERT(stego_extract_bytes(STEGO_ALG_LSBI, bmp, &extracted[0], 1));
        ASSERT(stego_extract_bytes(STEGO_ALG_LSBI, bmp, &extracted[1], 2));
        ASSERT(memcmp(extracted, payload, sizeof(payload)) == 0);

        // Exceeding capacity (max 3 bytes) must fail
        uint8_t too_large[4] = {0};
        ASSERT(!stego_embed(STEGO_ALG_LSBI, bmp, too_large, sizeof(too_large)));

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
    test_lsbi_inversion_and_roundtrip();
    test_full_payload_structure_roundtrip();

    return ASSERT_REPORT();
}
