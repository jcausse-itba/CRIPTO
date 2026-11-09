#ifndef _BITMAP_H_
#define _BITMAP_H_

#include <stdint.h>
#include <stddef.h>

typedef struct bitmap_internal * bitmap;

/**
 * @brief       Load and validate a 24 bpp uncompressed Bitmap V3 file into memory.
 * 
 * @param[in]   filename    Path of the `.bmp` file to load.
 * 
 * @return      A new `bitmap` instance on success, `nullptr` on failure.
 */
bitmap bitmap_create(const char * restrict filename);

/**
 * @brief       Reset the pixel cursor to the beginning of the carrier image.
 * 
 * @param[in]   b           `bitmap` instance.
 */
void bitmap_rewind(bitmap b);

/**
 * @brief       Get total carrier bytes in the image (including row alignment padding).
 * 
 * @param[in]   b           `bitmap` instance.
 * 
 * @return      Total carrier bytes (stride * height).
 */
size_t bitmap_get_total_bytes(bitmap b);

/**
 * @brief       Get a direct pointer to the next carrier byte in memory,
 *              advancing the cursor. Allows in-place reading and modification
 *              during embedding.
 * 
 * @param[in]   b           `bitmap` instance.
 * 
 * @return      Pointer to the byte in memory, or `nullptr` when finished.
 */
uint8_t * bitmap_next_byte(bitmap b);

/**
 * @brief       Read the next carrier byte, advancing the cursor.
 *              This is designed to be used on read-only operations.
 * 
 * @param[in]   b           `bitmap` instance.
 * @param[out]  out_byte    Destination pointer for the read byte.
 * 
 * @return      `true` if a byte was read, `false` at the end of the image.
 */
bool bitmap_get_byte(bitmap b, uint8_t * out_byte);

/**
 * @brief       Write back a modified value to the carrier byte that was
 *              just fetched by `bitmap_get_byte` (caller must fetch a byte 
 *              before calling this function).
 * 
 * @param[in]   b           `bitmap` instance.
 * @param[in]   byte        New byte value.
 * 
 * @return      `true` on success, `false` if no byte has been read yet.
 */
bool bitmap_set_byte(bitmap b, uint8_t byte);

/**
 * @brief       Write the entire in-memory bitmap (with original headers,
 *              modified pixels, and untouched padding) to a new file.
 * 
 * @param[in]   b           `bitmap` instance.
 * @param[in]   filename    Destination file path.
 * 
 * @return      `true` on success, `false` on I/O error.
 */
bool bitmap_save(bitmap b, const char * restrict filename);

/**
 * @brief       Destroy a `bitmap` and free all allocated memory.
 * 
 * @param[in]   b           `bitmap` instance.
 */
void bitmap_destroy(bitmap b);

#endif // _BITMAP_H_
