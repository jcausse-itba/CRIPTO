#ifndef _CRYPTOGRAPHY_H_
#define _CRYPTOGRAPHY_H_

#include <stdint.h>     // uint8_t
#include <stddef.h>     // size_t

// NOTE: Last nullptr is required for the CLI options array. Do not remove.
#define CIPHER_ALGORITHMS(XX)                                   \
    XX(CRYPTO_CIPHER_ALGORITHM_AES_128,             "AES128")   \
    XX(CRYPTO_CIPHER_ALGORITHM_AES_192,             "AES192")   \
    XX(CRYPTO_CIPHER_ALGORITHM_AES_256,             "AES256")   \
    XX(CRYPTO_CIPHER_ALGORITHM_3DES,                "3DES"  )   \
    XX(CRYPTO_CIPHER_ALGORITHM_NONE,                nullptr )

// NOTE: Last nullptr is required for the CLI options array. Do not remove.
#define CIPHER_MODES(XX)                                        \
    XX(CRYPTO_CIPHER_MODE_ECB,                      "ECB"   )   \
    XX(CRYPTO_CIPHER_MODE_CFB,                      "CFB"   )   \
    XX(CRYPTO_CIPHER_MODE_OFB,                      "OFB"   )   \
    XX(CRYPTO_CIPHER_MODE_CBC,                      "CBC"   )   \
    XX(CRYPTO_CIPHER_MODE_NONE,                     nullptr )

/********************************************************************************************/

/**
 * @brief       Identifiers of all supported cipher algorithms
 */
typedef enum {
    #define XX(_identifier, _string) _identifier,
    CIPHER_ALGORITHMS(XX)
    #undef XX
} crypto_cipher_algorithm;

/**
 * @brief       Identifiers of all supported cipher modes
 */
typedef enum {
    #define XX(_identifier, _string) _identifier,
    CIPHER_MODES(XX)
    #undef XX
} crypto_cipher_mode;

/**
 * @brief       Cipher descriptor (algorithm + mode)
 */
typedef struct {
    crypto_cipher_algorithm     algorithm;
    crypto_cipher_mode          mode;
} crypto_cipher;

/********************************************************************************************/

/**
 * @brief       Resolve the cipher algorithm and mode from strings
 * 
 * @param[in]   algorithm           Cipher algorithm to use. Must match one of the
 *                                  CIPHER_ALGORITHMS strings.
 * @param[in]   mode                Cipher mode to use. Must match one of the
 *                                  CIPHER_MODES strings.
 * 
 * @return      A cipher descriptor. If the `algorithm` or `mode` are `nullptr`, the
 *              descriptor is set to:
 *              - algorithm = CRYPTO_CIPHER_ALGORITHM_NONE
 *              - mode      = CRYPTO_CIPHER_MODE_NONE
 */
crypto_cipher crypto_resolve_cipher(const char * algorithm, const char * mode);

/**
 * @brief       Encrypt a plaintext buffer using the specified cipher and password.
 *              Allocates and returns the ciphertext buffer (`*out_ciphertext_len` bytes).
 * @param[in]   cipher              Descriptor of the cipher (algorithm + mode) to use.
 * @param[in]   password            Encryption password.
 * @param[in]   plaintext           Plaintext to encrypt.
 * @param[in]   plaintext_len       Length of the plaintext to encrypt.
 * @param[out]  out_ciphertext_len  Length of the output ciphertext.
 * 
 * @return      Ciphertext on success, `nullptr` on error.
 */
uint8_t * crypto_encrypt(
    crypto_cipher cipher,
    const char * password,
    const uint8_t * plaintext,
    size_t plaintext_len,
    size_t * out_ciphertext_len
);

/**
 * @brief       Decrypt a ciphertext buffer using the specified cipher and password.
 *              Allocates and returns the plaintext buffer (`*out_plaintext_len` bytes).
 * @param[in]   cipher              Descriptor of the cipher (algorithm + mode) to use.
 * @param[in]   password            Decryption password.
 * @param[in]   ciphertext          Ciphertext to decrypt.
 * @param[in]   ciphertext_len      Length of the ciphertext to decrypt.
 * @param[out]  out_plaintext_len   Length of the output plaintext.
 * 
 * @return      Plaintext on success, `nullptr` on error.
 */
uint8_t * crypto_decrypt(
    crypto_cipher cipher,
    const char * password,
    const uint8_t * ciphertext,
    size_t ciphertext_len,
    size_t * out_plaintext_len
);

#endif // _CRYPTOGRAPHY_H_
