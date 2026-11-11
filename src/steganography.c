#include "steganography.h"
#include "string_utils.h"

/**
 * @brief Strategy interface for a steganography algorithm.
 */
typedef struct {
    const char * name;
    size_t (*max_capacity)(size_t carrier_bytes);
    bool (*embed)(bitmap bmp, const uint8_t * payload, size_t payload_size);
    bool (*extract_bytes)(bitmap bmp, uint8_t * out_buffer, size_t count);
} stego_strategy_t;

// ============================================================================
// LSB1 Strategy Implementation (1 bit per carrier byte, MSB to LSB)
// ============================================================================

static size_t lsb1_max_capacity(size_t carrier_bytes) {
    return carrier_bytes / 8;
}

static bool lsb1_embed(bitmap bmp, const uint8_t * payload, size_t payload_size) {
    for (size_t i = 0; i < payload_size; i++) {
        uint8_t byte = payload[i];
        for (int bit = 7; bit >= 0; bit--) {
            uint8_t * carrier = bitmap_next_byte(bmp);
            if (!carrier) return false;
            uint8_t bit_val = (uint8_t)((byte >> bit) & 0x01U);
            *carrier = (uint8_t)((*carrier & 0xFEU) | bit_val);
        }
    }
    return true;
}

static bool lsb1_extract_bytes(bitmap bmp, uint8_t * out_buffer, size_t count) {
    for (size_t i = 0; i < count; i++) {
        uint8_t extracted = 0;
        for (int bit = 7; bit >= 0; bit--) {
            uint8_t carrier = 0;
            if (!bitmap_get_byte(bmp, &carrier)) return false;
            extracted = (uint8_t)((extracted << 1) | (carrier & 0x01U));
        }
        out_buffer[i] = extracted;
    }
    return true;
}

// ============================================================================
// LSB4 Strategy Implementation (4 bits per carrier byte, high nibble first)
// ============================================================================

static size_t lsb4_max_capacity(size_t carrier_bytes) {
    return carrier_bytes / 2;
}

static bool lsb4_embed(bitmap bmp, const uint8_t * payload, size_t payload_size) {
    for (size_t i = 0; i < payload_size; i++) {
        uint8_t byte = payload[i];
        uint8_t * hi_carrier = bitmap_next_byte(bmp);
        uint8_t * lo_carrier = bitmap_next_byte(bmp);
        if (!hi_carrier || !lo_carrier) return false;

        uint8_t hi_nibble = (uint8_t)((byte >> 4) & 0x0FU);
        uint8_t lo_nibble = (uint8_t)(byte & 0x0FU);

        *hi_carrier = (uint8_t)((*hi_carrier & 0xF0U) | hi_nibble);
        *lo_carrier = (uint8_t)((*lo_carrier & 0xF0U) | lo_nibble);
    }
    return true;
}

static bool lsb4_extract_bytes(bitmap bmp, uint8_t * out_buffer, size_t count) {
    for (size_t i = 0; i < count; i++) {
        uint8_t hi_carrier = 0;
        uint8_t lo_carrier = 0;
        if (!bitmap_get_byte(bmp, &hi_carrier) || !bitmap_get_byte(bmp, &lo_carrier)) {
            return false;
        }
        out_buffer[i] = (uint8_t)(((hi_carrier & 0x0FU) << 4) | (lo_carrier & 0x0FU));
    }
    return true;
}

// ============================================================================
// LSBI Strategy Implementation (LSB Improved - Majeed & Sulaiman)
//
// Specification (Enunciado.pdf Section 5.3.3, LSBI_Paper.pdf, and
// LSBI_Implementation_Notes.md):
// 1. The first 4 carrier bytes (indices 0..3: B0, G0, R0, B1) store the 4
//    inversion-pattern control bits using standard LSB1 (all 3 channels used,
//    never inverted), corresponding to patterns '00' (0), '01' (1), '10' (2),
//    and '11' (3).
// 2. Starting at carrier index 4 (G1), payload bits are embedded/extracted 1 bit
//    per carrier byte (MSB to LSB) using only Blue and Green channels; every
//    Red channel byte (where carrier_idx % 3 == 2) is skipped.
// 3. Each usable carrier byte belongs to one of 4 patterns defined by its 2nd
//    and 3rd least significant bits: pattern = (byte >> 1) & 0x03.
// 4. For each pattern p in 0..3, if the number of carrier bytes whose LSB
//    changed during standard LSB1 embedding is strictly greater than those
//    left unchanged (changed[p] > unchanged[p]), the LSB of all payload
//    carrier bytes with pattern p is inverted, and pattern bit p is set to 1
//    (otherwise 0). If no pattern needs inversion, pass 2 is omitted.
// 5. Extraction is single-pass and stateless: the 4 control bits are read
//    from the first 4 carrier bytes into a uint8_t[4] array, and each bit
//    is re-inverted via branchless XOR: bit_val = (carrier & 1) ^ invert[pattern].
// ============================================================================

#define LSBI_PATTERN_COUNT 4

static size_t lsbi_max_capacity(size_t carrier_bytes) {
    if (carrier_bytes < LSBI_PATTERN_COUNT) return 0;
    size_t rem = carrier_bytes - LSBI_PATTERN_COUNT;
    // Starting at index 4 (G1), indices 5, 8, 11, ... (where (idx - 4) % 3 == 1)
    // correspond to the Red channel and are skipped.
    size_t skipped_red = (rem + 1) / 3;
    size_t usable_bits = rem - skipped_red;
    return usable_bits / 8;
}

static uint8_t * lsbi_next_carrier_byte(bitmap bmp) {
    uint8_t * carrier = nullptr;
    do {
        carrier = bitmap_next_byte(bmp);
        if (!carrier) return nullptr;
    } while (((bitmap_get_cursor(bmp) - 1) % 3) == 2); // Skip Red channel bytes
    return carrier;
}

static bool lsbi_get_carrier_byte(bitmap bmp, uint8_t * out_byte) {
    uint8_t b = 0;
    do {
        if (!bitmap_get_byte(bmp, &b)) return false;
    } while (((bitmap_get_cursor(bmp) - 1) % 3) == 2); // Skip Red channel bytes
    if (out_byte) *out_byte = b;
    return true;
}

static bool lsbi_embed(bitmap bmp, const uint8_t * payload, size_t payload_size) {
    // 1. Reserve pointers to the first 4 carrier bytes (B0, G0, R0, B1) for the pattern map
    uint8_t * pattern_carriers[LSBI_PATTERN_COUNT];
    for (int p = 0; p < LSBI_PATTERN_COUNT; p++) {
        pattern_carriers[p] = bitmap_next_byte(bmp);
        if (!pattern_carriers[p]) return false;
    }

    // 2. Pass 1: Apply standard LSB1 on B and G channels (skipping R where byte offset % 3 == 2)
    //    and count how many carrier bytes changed vs remained unchanged per 2-bit pattern.
    size_t changed[LSBI_PATTERN_COUNT]   = {0};
    size_t unchanged[LSBI_PATTERN_COUNT] = {0};

    for (size_t i = 0; i < payload_size; i++) {
        uint8_t byte = payload[i];
        for (int bit = 7; bit >= 0; bit--) {
            uint8_t * carrier = lsbi_next_carrier_byte(bmp);
            if (!carrier) return false;

            uint8_t pattern    = (uint8_t)((*carrier >> 1) & 0x03U);
            uint8_t orig_lsb   = (uint8_t)(*carrier & 0x01U);
            uint8_t secret_bit = (uint8_t)((byte >> bit) & 0x01U);

            if (orig_lsb != secret_bit) {
                changed[pattern]++;
            } else {
                unchanged[pattern]++;
            }

            *carrier = (uint8_t)((*carrier & 0xFEU) | secret_bit);
        }
    }

    // 3. Decide which patterns require bit inversion and write the 4 control bits
    //    into the first 4 carrier bytes using normal LSB1.
    uint8_t inv_mask[LSBI_PATTERN_COUNT] = {0};
    bool any_inverted = false;
    for (int p = 0; p < LSBI_PATTERN_COUNT; p++) {
        if (changed[p] > unchanged[p]) {
            inv_mask[p] = 1U;
            any_inverted = true;
        }
        *pattern_carriers[p] = (uint8_t)((*pattern_carriers[p] & 0xFEU) | inv_mask[p]);
    }

    // If no pattern requires inversion, pass 2 is omitted per implementation notes
    if (!any_inverted) {
        return true;
    }

    // 4. Pass 2: Rewind, skip the 4 pattern bytes, and invert the LSB of all used
    //    payload carrier bytes whose pattern was marked for inversion (branchless XOR).
    bitmap_rewind(bmp);
    for (int p = 0; p < LSBI_PATTERN_COUNT; p++) {
        if (!bitmap_next_byte(bmp)) return false;
    }

    size_t total_payload_bits = payload_size * 8;
    for (size_t b = 0; b < total_payload_bits; b++) {
        uint8_t * carrier = lsbi_next_carrier_byte(bmp);
        if (!carrier) return false;

        uint8_t pattern = (uint8_t)((*carrier >> 1) & 0x03U);
        *carrier ^= inv_mask[pattern];
    }

    return true;
}

static bool lsbi_extract_bytes(bitmap bmp, uint8_t * out_buffer, size_t count) {
    size_t cursor = bitmap_get_cursor(bmp);
    if (cursor > 0 && cursor < LSBI_PATTERN_COUNT) {
        return false; // Invalid cursor state mid-header
    }

    // If at the start of the bitmap, advance cursor past the 4 header bytes
    if (cursor == 0) {
        for (int p = 0; p < LSBI_PATTERN_COUNT; p++) {
            uint8_t dummy = 0;
            if (!bitmap_get_byte(bmp, &dummy)) return false;
        }
    }

    // Read the 4 inversion map bits from the first 4 bytes of the carrier (B0, G0, R0, B1)
    // using bitmap_get_byte_at without moving the cursor.
    // Using a uint8_t[4] array allows applying bit inversion via branchless XOR per implementation notes.
    uint8_t invert[LSBI_PATTERN_COUNT];
    for (int p = 0; p < LSBI_PATTERN_COUNT; p++) {
        uint8_t hdr_byte = 0;
        if (!bitmap_get_byte_at(bmp, (size_t) p, &hdr_byte)) return false;
        invert[p] = (uint8_t)(hdr_byte & 0x01U);
    }

    // Extract `count` payload bytes from B and G channels (skipping Red channel where byte_idx % 3 == 2)
    for (size_t i = 0; i < count; i++) {
        uint8_t extracted = 0;
        for (int bit = 7; bit >= 0; bit--) {
            uint8_t carrier = 0;
            if (!lsbi_get_carrier_byte(bmp, &carrier)) return false;

            uint8_t pattern = (uint8_t)((carrier >> 1) & 0x03U);
            uint8_t bit_val = (uint8_t)((carrier & 0x01U) ^ invert[pattern]);
            extracted = (uint8_t)((extracted << 1) | bit_val);
        }
        out_buffer[i] = extracted;
    }

    return true;
}

// ============================================================================
// Strategy Dispatch Table & Public API
// ============================================================================

static const stego_strategy_t STRATEGIES[STEGO_ALG_COUNT] = {
    [STEGO_ALG_LSB1] = {
        .name          = "LSB1",
        .max_capacity  = lsb1_max_capacity,
        .embed         = lsb1_embed,
        .extract_bytes = lsb1_extract_bytes
    },
    [STEGO_ALG_LSB4] = {
        .name          = "LSB4",
        .max_capacity  = lsb4_max_capacity,
        .embed         = lsb4_embed,
        .extract_bytes = lsb4_extract_bytes
    },
    [STEGO_ALG_LSBI] = {
        .name          = "LSBI",
        .max_capacity  = lsbi_max_capacity,
        .embed         = lsbi_embed,
        .extract_bytes = lsbi_extract_bytes
    }
};

static bool is_valid_alg(stego_alg_t alg) {
    return alg >= STEGO_ALG_LSB1 && alg < STEGO_ALG_COUNT;
}

bool stego_alg_from_string(const char * str, stego_alg_t * out_alg) {
    if (!str || !out_alg) return false;
    for (int i = 0; i < STEGO_ALG_COUNT; i++) {
        if (str_equals(str, STRATEGIES[i].name, true)) {
            *out_alg = (stego_alg_t) i;
            return true;
        }
    }
    return false;
}

const char * stego_alg_to_string(stego_alg_t alg) {
    if (!is_valid_alg(alg)) return "UNKNOWN";
    return STRATEGIES[alg].name;
}

size_t stego_get_max_capacity(stego_alg_t alg, bitmap bmp) {
    if (!is_valid_alg(alg) || !bmp) return 0;
    return STRATEGIES[alg].max_capacity(bitmap_get_total_bytes(bmp));
}

bool stego_embed(stego_alg_t alg, bitmap bmp, const uint8_t * payload, size_t payload_size) {
    if (!is_valid_alg(alg) || !bmp || (!payload && payload_size > 0)) return false;
    if (payload_size > stego_get_max_capacity(alg, bmp)) return false;

    bitmap_rewind(bmp);
    return STRATEGIES[alg].embed(bmp, payload, payload_size);
}

bool stego_extract_bytes(stego_alg_t alg, bitmap bmp, uint8_t * out_buffer, size_t count) {
    if (!is_valid_alg(alg) || !bmp || (!out_buffer && count > 0)) return false;
    if (count == 0) return true;

    return STRATEGIES[alg].extract_bytes(bmp, out_buffer, count);
}
