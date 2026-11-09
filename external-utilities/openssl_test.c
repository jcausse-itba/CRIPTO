#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <openssl/evp.h>    // OpenSSL "EVP" (Envelope): high-level API for ciphers, hashes, and PBKDF2
#include <openssl/crypto.h> // Provides OPENSSL_cleanse() for securely wiping keys from memory
#include "cli.h"
#include "string_utils.h"

/*
Compile and run command:
gcc -Wall -Wextra -Werror -pedantic -std=c23 -Iinclude -Ilib \
    external-utilities/openssl_test.c lib/cli.c lib/hashmap.c lib/string_utils.c \
    -o openssl_test -lssl -lcrypto &&
./openssl_test -a aes128 -m cbc -pass "oculto" &&
rm openssl_test
*/

// ============================================================================
// CLI Definitions (copied from include/cli_arguments.h, all required for PoC)
// ============================================================================

#define CLI_OPT_CIPHER_ALG_SHORT        "a"
#define CLI_OPT_CIPHER_ALG_LONG         "cipher-algorithm"
#define CLI_OPT_CIPHER_MODE_SHORT       "m"
#define CLI_OPT_CIPHER_MODE_LONG        "cipher-mode"
#define CLI_OPT_PASS_LONG               "pass"

static const char * CIPHER_CHOICES[] = {
    "AES128",
    "AES192",
    "AES256",
    "3DES",
    nullptr
};

static const char * MODE_CHOICES[] = {
    "ECB",
    "CFB",
    "OFB",
    "CBC",
    nullptr
};

static const cli_option CRYPTO_CLI_OPTIONS[] = {
    {
        .option_name_short = CLI_OPT_CIPHER_ALG_SHORT,
        .option_name_long  = CLI_OPT_CIPHER_ALG_LONG,
        .type              = CLI_TYPE_CHOICE,
        .required          = true,
        .choices           = CIPHER_CHOICES,
        .case_insensitive  = true,
    },
    {
        .option_name_short = CLI_OPT_CIPHER_MODE_SHORT,
        .option_name_long  = CLI_OPT_CIPHER_MODE_LONG,
        .type              = CLI_TYPE_CHOICE,
        .required          = true,
        .choices           = MODE_CHOICES,
        .case_insensitive  = true,
    },
    {
        .option_name_long  = CLI_OPT_PASS_LONG,
        .type              = CLI_TYPE_STRING,
        .required          = true,
    }
};

// ============================================================================
// Cryptography Primitives (PBKDF2-HMAC-SHA256 + OpenSSL EVP)
//
// How OpenSSL's EVP API works in a nutshell:
// 1. `const EVP_CIPHER *`:
//    An immutable descriptor (like a Strategy object inside OpenSSL) that
//    represents a specific (Algorithm + Mode) combination, e.g. AES-128-CBC.
//    You query it with helper functions to know how many key bytes, IV bytes,
//    and block bytes it requires:
//      - `EVP_CIPHER_key_length(cipher)` -> e.g., 16 for AES128, 24 for 3DES, 32 for AES256
//      - `EVP_CIPHER_iv_length(cipher)`  -> e.g., 16 for AES CBC/CFB/OFB, 8 for 3DES, 0 for ECB
//      - `EVP_CIPHER_block_size(cipher)` -> e.g., 16 for AES ECB/CBC, 8 for 3DES ECB/CBC, 1 for stream modes (CFB/OFB)
//
// 2. `EVP_CIPHER_CTX *`:
//    An opaque state machine (context) allocated on the heap for a single
//    encryption or decryption operation. Every operation follows 3 steps:
//      a) `EVP_EncryptInit_ex` / `EVP_DecryptInit_ex`: Load the cipher, Key, and IV.
//      b) `EVP_EncryptUpdate`  / `EVP_DecryptUpdate`:  Process the input buffer.
//      c) `EVP_EncryptFinal_ex`/ `EVP_DecryptFinal_ex`: Handle the last block & PKCS#5/PKCS#7 padding.
// ============================================================================

#define PBKDF2_SALT_LEN     8
#define PBKDF2_ITERATIONS   10000

// Fixed 8-byte zero salt (0x0000000000000000) required by Enunciado.pdf Section 5.4
static const uint8_t PBKDF2_FIXED_SALT[PBKDF2_SALT_LEN] = {0};

/**
 * @brief       Select the OpenSSL `EVP_CIPHER` descriptor for a given algorithm and mode.
 *
 * @details     Per Enunciado.pdf Section 5.4:
 *              - CFB mode uses 8-bit feedback (`*_cfb8()` in OpenSSL).
 *              - OFB mode uses full-block feedback (`*_ofb()` in OpenSSL: 128-bit for AES, 64-bit for 3DES).
 *              - 3DES uses 3-key Triple DES (`EVP_des_ede3_*()` in OpenSSL).
 */
static const EVP_CIPHER * get_openssl_cipher(const char * alg, const char * mode) {
    if (str_equals(alg, "AES128", true)) {
        if (str_equals(mode, "ECB", true)) return EVP_aes_128_ecb();
        if (str_equals(mode, "CFB", true)) return EVP_aes_128_cfb8();
        if (str_equals(mode, "OFB", true)) return EVP_aes_128_ofb();
        if (str_equals(mode, "CBC", true)) return EVP_aes_128_cbc();
    } else if (str_equals(alg, "AES192", true)) {
        if (str_equals(mode, "ECB", true)) return EVP_aes_192_ecb();
        if (str_equals(mode, "CFB", true)) return EVP_aes_192_cfb8();
        if (str_equals(mode, "OFB", true)) return EVP_aes_192_ofb();
        if (str_equals(mode, "CBC", true)) return EVP_aes_192_cbc();
    } else if (str_equals(alg, "AES256", true)) {
        if (str_equals(mode, "ECB", true)) return EVP_aes_256_ecb();
        if (str_equals(mode, "CFB", true)) return EVP_aes_256_cfb8();
        if (str_equals(mode, "OFB", true)) return EVP_aes_256_ofb();
        if (str_equals(mode, "CBC", true)) return EVP_aes_256_cbc();
    } else if (str_equals(alg, "3DES", true)) {
        if (str_equals(mode, "ECB", true)) return EVP_des_ede3_ecb();
        if (str_equals(mode, "CFB", true)) return EVP_des_ede3_cfb8();
        if (str_equals(mode, "OFB", true)) return EVP_des_ede3_ofb();
        if (str_equals(mode, "CBC", true)) return EVP_des_ede3_cbc();
    }
    return nullptr;
}

/**
 * @brief       Derive the symmetric Key and Initialization Vector (IV) from a user password
 *              using PBKDF2-HMAC-SHA256 (8-byte zero salt, 10000 iterations).
 *
 * @details     A human password (e.g., "oculto") cannot be used directly as a cipher key
 *              because ciphers require exact byte lengths (16, 24, or 32 bytes) and high entropy.
 *              PBKDF2 stretches the password into `key_len + iv_len` pseudorandom bytes:
 *                  derived = [ <--- key_len bytes ---> | <--- iv_len bytes ---> ]
 *              We then split `derived` into `out_key` and `out_iv`.
 */
static bool derive_key_and_iv(
    const EVP_CIPHER * cipher,
    const char * password,
    uint8_t out_key[static EVP_MAX_KEY_LENGTH],
    uint8_t out_iv[static EVP_MAX_IV_LENGTH]
) {
    if (!cipher || !password) return false;

    // Ask the OpenSSL cipher descriptor how many bytes of Key and IV it needs.
    // Note: For ECB mode, `iv_len` will be 0 because ECB does not use an IV.
    int key_len   = EVP_CIPHER_key_length(cipher);
    int iv_len    = EVP_CIPHER_iv_length(cipher);
    int total_len = key_len + iv_len;

    // Temporary buffer large enough to hold the largest possible Key + IV in OpenSSL
    uint8_t derived[EVP_MAX_KEY_LENGTH + EVP_MAX_IV_LENGTH] = {0};

    // Run PBKDF2 using HMAC-SHA256 (`EVP_sha256()`).
    // OpenSSL functions return 1 on success and 0 on failure.
    int ok = PKCS5_PBKDF2_HMAC(
        password,                   // Input password string
        (int) strlen(password),     // Password length in bytes
        PBKDF2_FIXED_SALT,          // Fixed 8-byte zero salt
        PBKDF2_SALT_LEN,            // Salt length (8 bytes)
        PBKDF2_ITERATIONS,          // Iteration count (10000)
        EVP_sha256(),               // Hash function for HMAC (SHA-256)
        total_len,                  // How many total bytes to generate (key_len + iv_len)
        derived                     // Output buffer receiving the generated bytes
    );

    if (ok != 1) {
        // `OPENSSL_cleanse` is like `memset(..., 0, ...)`, but guaranteed not to be
        // optimized away by the compiler when wiping secret data from stack memory.
        OPENSSL_cleanse(derived, sizeof(derived));
        return false;
    }

    // Split the generated byte stream:
    // First `key_len` bytes -> symmetric key
    // Next  `iv_len`  bytes -> initialization vector (IV)
    memcpy(out_key, derived, (size_t) key_len);
    if (iv_len > 0) {
        memcpy(out_iv, derived + key_len, (size_t) iv_len);
    }

    OPENSSL_cleanse(derived, sizeof(derived));
    return true;
}

/**
 * @brief       Encrypt a plaintext buffer using the specified OpenSSL `EVP_CIPHER` and password.
 *              Allocates and returns the ciphertext buffer (`*out_ciphertext_len` bytes).
 */
static uint8_t * crypto_encrypt(
    const EVP_CIPHER * cipher,
    const char * password,
    const uint8_t * plaintext,
    size_t plaintext_len,
    size_t * out_ciphertext_len
) {
    if (!cipher || !password || !plaintext || !out_ciphertext_len) return nullptr;

    // 1. Derive Key and IV from the password
    uint8_t key[EVP_MAX_KEY_LENGTH] = {0};
    uint8_t iv[EVP_MAX_IV_LENGTH]   = {0};
    if (!derive_key_and_iv(cipher, password, key, iv)) {
        return nullptr;
    }

    uint8_t * ciphertext = nullptr;

    // 2. Create and initialize a new OpenSSL cipher context on the heap
    EVP_CIPHER_CTX * ctx = EVP_CIPHER_CTX_new();
    if (!ctx) goto cleanup;

    // 3. Allocate output buffer for the ciphertext.
    //    Why `plaintext_len + block_size`?
    //    Block modes (ECB, CBC) use PKCS#5/PKCS#7 padding by default in OpenSSL,
    //    which always adds between 1 and `block_size` extra bytes at the end.
    int block_size = EVP_CIPHER_block_size(cipher);
    ciphertext = malloc(plaintext_len + (size_t) block_size);
    if (!ciphertext) goto cleanup;

    // OpenSSL uses `int` out-parameters to report how many bytes each step wrote:
    // - `update_len`: bytes written by `EVP_EncryptUpdate`
    // - `final_len`:  bytes written by `EVP_EncryptFinal_ex` (the padded last block)
    int update_len = 0;
    int final_len  = 0;

    // Pass `nullptr` for IV if the mode does not use an IV (i.e. ECB mode)
    const uint8_t * iv_ptr = EVP_CIPHER_iv_length(cipher) > 0 ? iv : nullptr;

    // 4. Run the 3-step OpenSSL encryption pipeline:
    //    - Step A (`EVP_EncryptInit_ex`): Configure context with cipher, key, and IV.
    //      (The 3rd parameter `ENGINE *impl` is `nullptr` to use OpenSSL's default implementation).
    //      Note: PKCS#5/PKCS#7 padding is automatically enabled by default.
    //    - Step B (`EVP_EncryptUpdate`): Encrypt input data blocks into `ciphertext`.
    //      Sets `update_len` to the number of ciphertext bytes written so far.
    //    - Step C (`EVP_EncryptFinal_ex`): Pad and encrypt any remaining partial block,
    //      writing it at `ciphertext + update_len` (right after Step B's output),
    //      and setting `final_len` to the number of final bytes written.
    if (    EVP_EncryptInit_ex(ctx, cipher, nullptr, key, iv_ptr)                               != 1 
        ||  EVP_EncryptUpdate(ctx, ciphertext, &update_len, plaintext, (int) plaintext_len)     != 1 
        ||  EVP_EncryptFinal_ex(ctx, ciphertext + update_len, &final_len)                       != 1
    ) {
        free(ciphertext);
        ciphertext = nullptr;
        goto cleanup;
    }

    // Total encrypted size is the sum of bytes produced by Update + Final
    *out_ciphertext_len = (size_t)(update_len + final_len);

cleanup:
    // 5. Free the OpenSSL context and wipe sensitive key/IV from stack memory
    if (ctx) EVP_CIPHER_CTX_free(ctx);
    OPENSSL_cleanse(key, sizeof(key));
    OPENSSL_cleanse(iv, sizeof(iv));

    return ciphertext;
}

/**
 * @brief       Decrypt a ciphertext buffer using the specified OpenSSL `EVP_CIPHER` and password.
 *              Allocates and returns the plaintext buffer (`*out_plaintext_len` bytes).
 */
static uint8_t * crypto_decrypt(
    const EVP_CIPHER * cipher,
    const char * password,
    const uint8_t * ciphertext,
    size_t ciphertext_len,
    size_t * out_plaintext_len
) {
    if (!cipher || !password || !ciphertext || !out_plaintext_len) return nullptr;

    // 1. Derive the exact same Key and IV from the password
    uint8_t key[EVP_MAX_KEY_LENGTH] = {0};
    uint8_t iv[EVP_MAX_IV_LENGTH]   = {0};
    if (!derive_key_and_iv(cipher, password, key, iv)) {
        return nullptr;
    }

    uint8_t * plaintext = nullptr;

    // 2. Create a new OpenSSL cipher context
    EVP_CIPHER_CTX * ctx = EVP_CIPHER_CTX_new();
    if (!ctx) goto cleanup;

    // 3. Allocate output buffer for the decrypted plaintext
    int block_size = EVP_CIPHER_block_size(cipher);
    plaintext = malloc(ciphertext_len + (size_t) block_size);
    if (!plaintext) goto cleanup;

    int update_len = 0;
    int final_len  = 0;
    const uint8_t * iv_ptr = EVP_CIPHER_iv_length(cipher) > 0 ? iv : nullptr;

    // 4. Run the 3-step OpenSSL decryption pipeline:
    //    - Step A (`EVP_DecryptInit_ex`): Configure context with cipher, key, and IV.
    //    - Step B (`EVP_DecryptUpdate`): Decrypt ciphertext blocks into `plaintext`.
    //    - Step C (`EVP_DecryptFinal_ex`): Decrypt the final block, verify and strip
    //      the PKCS#5/PKCS#7 padding, and write any remaining plaintext bytes at
    //      `plaintext + update_len`. (If the password/key is wrong, padding verification
    //      fails here and `EVP_DecryptFinal_ex` returns 0!).
    if (    EVP_DecryptInit_ex(ctx, cipher, nullptr, key, iv_ptr)                               != 1 
        ||  EVP_DecryptUpdate(ctx, plaintext, &update_len, ciphertext, (int) ciphertext_len)    != 1 
        ||  EVP_DecryptFinal_ex(ctx, plaintext + update_len, &final_len)                        != 1
    ) {
        free(plaintext);
        plaintext = nullptr;
        goto cleanup;
    }

    // Total decrypted plaintext size (with padding already stripped by OpenSSL)
    *out_plaintext_len = (size_t)(update_len + final_len);

cleanup:
    // 5. Free the OpenSSL context and wipe sensitive key/IV from stack memory
    if (ctx) EVP_CIPHER_CTX_free(ctx);
    OPENSSL_cleanse(key, sizeof(key));
    OPENSSL_cleanse(iv, sizeof(iv));

    return plaintext;
}

// ============================================================================
// Proof-of-Concept Main
// ============================================================================

static void print_hex(const char * label, const uint8_t * data, size_t len) {
    printf("%s (%lu bytes): ", label, len);
    for (size_t i = 0; i < len; i++) {
        printf("%02x", data[i]);
    }
    printf("\n");
}

int main(int argc, const char * argv[]) {
    // Parse CLI arguments (most return values are unchecked since this is just a proof-of-concept program to later migrate
    // the generated crypto functions and implementations to stegobmp)
    cli c = cli_create();
    for (size_t i = 0; i < sizeof(CRYPTO_CLI_OPTIONS) / sizeof(CRYPTO_CLI_OPTIONS[0]); i++) {
        cli_add_option(c, CRYPTO_CLI_OPTIONS[i]);
    }

    if (!cli_parse(c, argc, argv, true)) {
        fprintf(stderr, "%s\n", cli_usage(c, argv[0]));
        cli_destroy(c);
        return EXIT_FAILURE;
    }

    const char * alg_str  = nullptr;
    const char * mode_str = nullptr;
    const char * password = nullptr;
    cli_get_string(c, CLI_OPT_CIPHER_ALG_LONG, &alg_str);
    cli_get_string(c, CLI_OPT_CIPHER_MODE_LONG, &mode_str);
    cli_get_string(c, CLI_OPT_PASS_LONG, &password);

    // STEP 1: Get cypher
    const EVP_CIPHER * cipher = get_openssl_cipher(alg_str, mode_str);

    printf("=== OpenSSL Crypto Proof of Concept ===\n");
    printf("Algorithm : %s (key=%d bytes)\n", alg_str, EVP_CIPHER_key_length(cipher));
    printf("Mode      : %s (iv=%d bytes, block=%d bytes)\n", mode_str, EVP_CIPHER_iv_length(cipher), EVP_CIPHER_block_size(cipher));
    printf("Password  : \"%s\"\n\n", password);

    const char * sample_msg = "Secret message for StegoBMP cryptography test!";
    size_t plaintext_len = strlen(sample_msg) + 1; // Include null terminator

    printf("Plaintext (%lu bytes): \"%s\"\n", plaintext_len, sample_msg);
    print_hex("Plaintext hex ", (const uint8_t *) sample_msg, plaintext_len);

    // STEP 2: Encrypt
    size_t ciphertext_len = 0;
    uint8_t * ciphertext = crypto_encrypt(cipher, password, (const uint8_t *) sample_msg, plaintext_len, &ciphertext_len);
    if (!ciphertext) {
        fprintf(stderr, "Encryption failed!\n");
        cli_destroy(c);
        return EXIT_FAILURE;
    }
    print_hex("Ciphertext hex", ciphertext, ciphertext_len);

    // STEP 3: Decrypt
    size_t decrypted_len = 0;
    uint8_t * decrypted = crypto_decrypt(cipher, password, ciphertext, ciphertext_len, &decrypted_len);
    if (!decrypted) {
        fprintf(stderr, "Decryption failed!\n");
        free(ciphertext);
        cli_destroy(c);
        return EXIT_FAILURE;
    }
    print_hex("Decrypted hex ", decrypted, decrypted_len);
    printf("Decrypted (%lu bytes): \"%s\"\n\n", decrypted_len, (const char *) decrypted);

    bool match = (decrypted_len == plaintext_len) && (memcmp(sample_msg, decrypted, plaintext_len) == 0);
    printf("Roundtrip verification: %s\n", match ? "SUCCESS" : "FAILURE");

    free(ciphertext);
    free(decrypted);
    cli_destroy(c);
    return match ? EXIT_SUCCESS : EXIT_FAILURE;
}
