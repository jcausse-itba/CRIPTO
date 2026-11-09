#include "cryptography.h"
#include "custom_assert.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

static void test_resolve_cipher_valid(void) {
    crypto_cipher c;

    // Exact canonical names
    c = crypto_resolve_cipher("AES128", "ECB");
    ASSERT(c.algorithm == CRYPTO_CIPHER_ALGORITHM_AES_128 && c.mode == CRYPTO_CIPHER_MODE_ECB);

    c = crypto_resolve_cipher("AES192", "CFB");
    ASSERT(c.algorithm == CRYPTO_CIPHER_ALGORITHM_AES_192 && c.mode == CRYPTO_CIPHER_MODE_CFB);

    c = crypto_resolve_cipher("AES256", "OFB");
    ASSERT(c.algorithm == CRYPTO_CIPHER_ALGORITHM_AES_256 && c.mode == CRYPTO_CIPHER_MODE_OFB);

    c = crypto_resolve_cipher("3DES", "CBC");
    ASSERT(c.algorithm == CRYPTO_CIPHER_ALGORITHM_3DES && c.mode == CRYPTO_CIPHER_MODE_CBC);

    // Case-insensitive resolution
    c = crypto_resolve_cipher("aes128", "cbc");
    ASSERT(c.algorithm == CRYPTO_CIPHER_ALGORITHM_AES_128 && c.mode == CRYPTO_CIPHER_MODE_CBC);

    c = crypto_resolve_cipher("3des", "cfb");
    ASSERT(c.algorithm == CRYPTO_CIPHER_ALGORITHM_3DES && c.mode == CRYPTO_CIPHER_MODE_CFB);

    c = crypto_resolve_cipher("AeS256", "OfB");
    ASSERT(c.algorithm == CRYPTO_CIPHER_ALGORITHM_AES_256 && c.mode == CRYPTO_CIPHER_MODE_OFB);
}

static void test_resolve_cipher_invalid_and_null(void) {
    crypto_cipher c;

    // Both nullptr
    c = crypto_resolve_cipher(nullptr, nullptr);
    ASSERT(c.algorithm == CRYPTO_CIPHER_ALGORITHM_NONE && c.mode == CRYPTO_CIPHER_MODE_NONE);

    // Only algorithm is nullptr / invalid (resolved independently per header spec)
    c = crypto_resolve_cipher(nullptr, "CBC");
    ASSERT(c.algorithm == CRYPTO_CIPHER_ALGORITHM_NONE && c.mode == CRYPTO_CIPHER_MODE_CBC);

    c = crypto_resolve_cipher("DES", "CBC");
    ASSERT(c.algorithm == CRYPTO_CIPHER_ALGORITHM_NONE && c.mode == CRYPTO_CIPHER_MODE_CBC);

    // Only mode is nullptr / invalid (resolved independently per header spec)
    c = crypto_resolve_cipher("AES128", nullptr);
    ASSERT(c.algorithm == CRYPTO_CIPHER_ALGORITHM_AES_128 && c.mode == CRYPTO_CIPHER_MODE_NONE);

    c = crypto_resolve_cipher("AES128", "GCM");
    ASSERT(c.algorithm == CRYPTO_CIPHER_ALGORITHM_AES_128 && c.mode == CRYPTO_CIPHER_MODE_NONE);

    // Empty strings
    c = crypto_resolve_cipher("", "");
    ASSERT(c.algorithm == CRYPTO_CIPHER_ALGORITHM_NONE && c.mode == CRYPTO_CIPHER_MODE_NONE);
}

static void test_all_algorithms_and_modes_roundtrip(void) {
    const crypto_cipher_algorithm algorithms[] = {
        CRYPTO_CIPHER_ALGORITHM_AES_128,
        CRYPTO_CIPHER_ALGORITHM_AES_192,
        CRYPTO_CIPHER_ALGORITHM_AES_256,
        CRYPTO_CIPHER_ALGORITHM_3DES
    };

    const crypto_cipher_mode modes[] = {
        CRYPTO_CIPHER_MODE_ECB,
        CRYPTO_CIPHER_MODE_CFB,
        CRYPTO_CIPHER_MODE_OFB,
        CRYPTO_CIPHER_MODE_CBC
    };

    // Binary payload with embedded null bytes (simulating: 4B size || file data || ".txt\0")
    const uint8_t sample_payload[] = {
        0x00, 0x00, 0x00, 0x0B,
        'H', 'e', 'l', 'l', 'o', 0x00, 'W', 'o', 'r', 'l', 'd',
        '.', 't', 'x', 't', '\0'
    };
    const size_t sample_len = sizeof(sample_payload);
    const char * password = "super_secret_password_2026";

    for (size_t a = 0; a < sizeof(algorithms) / sizeof(algorithms[0]); a++) {
        for (size_t m = 0; m < sizeof(modes) / sizeof(modes[0]); m++) {
            crypto_cipher cipher = {
                .algorithm = algorithms[a],
                .mode      = modes[m]
            };

            size_t ciphertext_len = 0;
            uint8_t * ciphertext = crypto_encrypt(cipher, password, sample_payload, sample_len, &ciphertext_len);
            ASSERT(ciphertext != nullptr);
            ASSERT(ciphertext_len >= sample_len);

            if (ciphertext) {
                // Ciphertext should not be identical to plaintext
                ASSERT(memcmp(ciphertext, sample_payload, sample_len) != 0);

                size_t decrypted_len = 0;
                uint8_t * decrypted = crypto_decrypt(cipher, password, ciphertext, ciphertext_len, &decrypted_len);
                ASSERT(decrypted != nullptr);
                ASSERT(decrypted_len == sample_len);

                if (decrypted) {
                    ASSERT(memcmp(decrypted, sample_payload, sample_len) == 0);
                    free(decrypted);
                }
                free(ciphertext);
            }
        }
    }
}

static void test_various_payload_sizes(void) {
    // Test block boundaries (1, 7, 8, 15, 16, 17, 31, 32, 257 bytes)
    const size_t test_sizes[] = { 1, 7, 8, 15, 16, 17, 31, 32, 257 };
    const crypto_cipher cipher_aes_cbc  = crypto_resolve_cipher("AES256", "CBC");
    const crypto_cipher cipher_3des_cbc = crypto_resolve_cipher("3DES", "CBC");
    const crypto_cipher cipher_aes_cfb  = crypto_resolve_cipher("AES128", "CFB");

    const crypto_cipher ciphers[] = { cipher_aes_cbc, cipher_3des_cbc, cipher_aes_cfb };

    for (size_t c_idx = 0; c_idx < sizeof(ciphers) / sizeof(ciphers[0]); c_idx++) {
        for (size_t s_idx = 0; s_idx < sizeof(test_sizes) / sizeof(test_sizes[0]); s_idx++) {
            size_t len = test_sizes[s_idx];
            uint8_t * input = malloc(len);
            ASSERT(input != nullptr);
            if (!input) continue;

            for (size_t i = 0; i < len; i++) {
                input[i] = (uint8_t)((i * 37 + 13) & 0xFFU);
            }

            size_t ct_len = 0;
            uint8_t * ct = crypto_encrypt(ciphers[c_idx], "pass123", input, len, &ct_len);
            ASSERT(ct != nullptr && ct_len >= len);

            if (ct) {
                size_t pt_len = 0;
                uint8_t * pt = crypto_decrypt(ciphers[c_idx], "pass123", ct, ct_len, &pt_len);
                ASSERT(pt != nullptr && pt_len == len);
                if (pt) {
                    ASSERT(memcmp(pt, input, len) == 0);
                    free(pt);
                }
                free(ct);
            }

            free(input);
        }
    }
}

static void test_determinism_and_password_sensitivity(void) {
    const uint8_t msg[] = "Deterministic key derivation check";
    const crypto_cipher cipher = crypto_resolve_cipher("AES128", "CBC");

    size_t ct1_len = 0, ct2_len = 0, ct_diff_len = 0;
    uint8_t * ct1     = crypto_encrypt(cipher, "same_pass", msg, sizeof(msg), &ct1_len);
    uint8_t * ct2     = crypto_encrypt(cipher, "same_pass", msg, sizeof(msg), &ct2_len);
    uint8_t * ct_diff = crypto_encrypt(cipher, "other_pass", msg, sizeof(msg), &ct_diff_len);

    ASSERT(ct1 != nullptr && ct2 != nullptr && ct_diff != nullptr);
    if (ct1 && ct2 && ct_diff) {
        // Same password and cipher must produce identical ciphertext
        ASSERT(ct1_len == ct2_len);
        ASSERT(memcmp(ct1, ct2, ct1_len) == 0);

        // Different password must produce different ciphertext
        ASSERT(memcmp(ct1, ct_diff, ct1_len) != 0);

        // Decrypting CBC ciphertext with wrong password must either return nullptr
        // (padding check failure) or not match the original plaintext
        size_t wrong_pt_len = 0;
        uint8_t * wrong_pt = crypto_decrypt(cipher, "wrong_password", ct1, ct1_len, &wrong_pt_len);
        if (wrong_pt) {
            ASSERT(wrong_pt_len != sizeof(msg) || memcmp(wrong_pt, msg, sizeof(msg)) != 0);
            free(wrong_pt);
        }
    }

    free(ct1);
    free(ct2);
    free(ct_diff);
}

static void test_invalid_parameters(void) {
    const uint8_t data[] = "0123456789abcdef";
    size_t out_len = 0;

    const crypto_cipher valid_cipher = crypto_resolve_cipher("AES128", "CBC");
    const crypto_cipher none_alg     = { CRYPTO_CIPHER_ALGORITHM_NONE, CRYPTO_CIPHER_MODE_CBC };
    const crypto_cipher none_mode    = { CRYPTO_CIPHER_ALGORITHM_AES_128, CRYPTO_CIPHER_MODE_NONE };
    const crypto_cipher both_none    = { CRYPTO_CIPHER_ALGORITHM_NONE, CRYPTO_CIPHER_MODE_NONE };

    // Invalid cipher descriptors
    ASSERT(crypto_encrypt(none_alg,  "pass", data, sizeof(data), &out_len) == nullptr);
    ASSERT(crypto_encrypt(none_mode, "pass", data, sizeof(data), &out_len) == nullptr);
    ASSERT(crypto_encrypt(both_none, "pass", data, sizeof(data), &out_len) == nullptr);

    ASSERT(crypto_decrypt(none_alg,  "pass", data, sizeof(data), &out_len) == nullptr);
    ASSERT(crypto_decrypt(none_mode, "pass", data, sizeof(data), &out_len) == nullptr);
    ASSERT(crypto_decrypt(both_none, "pass", data, sizeof(data), &out_len) == nullptr);

    // Null or empty password
    ASSERT(crypto_encrypt(valid_cipher, nullptr, data, sizeof(data), &out_len) == nullptr);
    ASSERT(crypto_encrypt(valid_cipher, "",      data, sizeof(data), &out_len) == nullptr);
    ASSERT(crypto_decrypt(valid_cipher, nullptr, data, sizeof(data), &out_len) == nullptr);
    ASSERT(crypto_decrypt(valid_cipher, "",      data, sizeof(data), &out_len) == nullptr);

    // Null input buffer
    ASSERT(crypto_encrypt(valid_cipher, "pass", nullptr, sizeof(data), &out_len) == nullptr);
    ASSERT(crypto_decrypt(valid_cipher, "pass", nullptr, sizeof(data), &out_len) == nullptr);

    // Null output length pointer
    ASSERT(crypto_encrypt(valid_cipher, "pass", data, sizeof(data), nullptr) == nullptr);
    ASSERT(crypto_decrypt(valid_cipher, "pass", data, sizeof(data), nullptr) == nullptr);
}

int main(void) {
    test_resolve_cipher_valid();
    test_resolve_cipher_invalid_and_null();
    test_all_algorithms_and_modes_roundtrip();
    test_various_payload_sizes();
    test_determinism_and_password_sensitivity();
    test_invalid_parameters();

    return ASSERT_REPORT();
}
