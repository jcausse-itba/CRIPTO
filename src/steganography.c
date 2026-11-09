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
// LSBI Strategy Placeholder (To be implemented in a future iteration)
// ============================================================================

static size_t lsbi_max_capacity(size_t carrier_bytes) {
    return carrier_bytes >= 4 ? (carrier_bytes - 4) / 8 : 0;
}

static bool lsbi_embed(bitmap bmp, const uint8_t * payload, size_t payload_size) {
    // TODO: Implement
    (void) bmp;
    (void) payload;
    (void) payload_size;
    return false;
}

static bool lsbi_extract_bytes(bitmap bmp, uint8_t * out_buffer, size_t count) {
    // TODO: Implement
    (void) bmp;
    (void) out_buffer;
    (void) count;
    return false;
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
