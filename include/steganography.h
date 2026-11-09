#ifndef _STEGANOGRAPHY_H_
#define _STEGANOGRAPHY_H_

#include <stddef.h>
#include <stdint.h>
#include "bitmap.h"

/**
 * @brief Supported steganography algorithms.
 */
typedef enum {
    STEGO_ALG_LSB1 = 0,     // Least Significant Byte - 1 bit
    STEGO_ALG_LSB4,         // Least Significant Byte - 4 bit
    STEGO_ALG_LSBI,         // Least Significant Byte - improved
    STEGO_ALG_COUNT
} stego_alg_t;

/**
 * @brief       Parse a steganography algorithm name string (case-insensitive).
 *
 * @param[in]   str         Algorithm string ("LSB1", "LSB4", or "LSBI").
 * @param[out]  out_alg     Output pointer to receive the resolved enum value.
 *
 * @return      `true` if `str` matches a supported algorithm, `false` otherwise.
 */
bool stego_alg_from_string(const char * str, stego_alg_t * out_alg);

/**
 * @brief       Get the canonical string representation of a steganography algorithm.
 *
 * @param[in]   alg         Algorithm enum value.
 *
 * @return      Constant string ("LSB1", "LSB4", "LSBI"), or "UNKNOWN" if invalid.
 */
const char * stego_alg_to_string(stego_alg_t alg);

/**
 * @brief       Calculate the maximum payload capacity (in bytes) that a carrier
 *              bitmap can hold using the specified steganography algorithm.
 *
 * @param[in]   alg         Steganography algorithm.
 * @param[in]   bmp         Loaded carrier bitmap.
 *
 * @return      Maximum embeddable payload size in bytes, or 0 on invalid arguments.
 */
size_t stego_get_max_capacity(stego_alg_t alg, bitmap bmp);

/**
 * @brief       Embed a raw payload buffer into the carrier bitmap in memory using
 *              the specified steganography algorithm. Rewinds the bitmap cursor
 *              to the beginning before embedding.
 *
 * @param[in]   alg             Steganography algorithm.
 * @param[in]   bmp             Loaded carrier bitmap (modified in-place).
 * @param[in]   payload         Pointer to the payload buffer to embed.
 * @param[in]   payload_size    Number of bytes in `payload`.
 *
 * @return      `true` on success, `false` if capacity is exceeded or parameters are invalid.
 */
bool stego_embed(stego_alg_t alg, bitmap bmp, const uint8_t * payload, size_t payload_size);

/**
 * @brief       Extract `count` payload bytes from the current cursor position of
 *              the carrier bitmap using the specified steganography algorithm.
 *              Does NOT rewind the bitmap, allowing sequential chunked reads
 *              (e.g., 4-byte size header, then N file bytes, then extension).
 *
 * @param[in]   alg         Steganography algorithm.
 * @param[in]   bmp         Loaded carrier bitmap.
 * @param[out]  out_buffer  Destination buffer of at least `count` bytes.
 * @param[in]   count       Number of payload bytes to extract.
 *
 * @return      `true` on success, `false` on EOF or invalid parameters.
 */
bool stego_extract_bytes(stego_alg_t alg, bitmap bmp, uint8_t * out_buffer, size_t count);

#endif // _STEGANOGRAPHY_H_
