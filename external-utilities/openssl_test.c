#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "cli.h"
#include "cryptography.h"

/*
Compile and run command:
gcc -Wall -Wextra -Werror -pedantic -std=c23 -Iinclude -Ilib \
    external-utilities/openssl_test.c lib/cli.c lib/hashmap.c lib/string_utils.c \
    src/cryptography.c \
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
    #define XX(_identifier, _string) _string,
    CIPHER_ALGORITHMS(XX)
    #undef XX
};

static const char * MODE_CHOICES[] = {
    #define XX(_identifier, _string) _string,
    CIPHER_MODES(XX)
    #undef XX
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
    crypto_cipher cipher = crypto_resolve_cipher(alg_str, mode_str);

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
