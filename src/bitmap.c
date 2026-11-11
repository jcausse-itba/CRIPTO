#include "bitmap.h"
#include "bitmap_v3.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct bitmap_internal {
    uint8_t * data;             // Complete file dump (including headers and image data)
    size_t size;                // Total size of the file
    size_t data_offset;         // Bitmap bytes start index in the data array
    size_t total_bytes;         // Total bitmap carrier capacity (pixels + inter-row padding)
    size_t cursor;              // Reading cursor within the image data block
    bool has_padding;           // True if rows contain alignment padding bytes
};

/**
 * NOTE: BMP Row Stride & Padding:
 * In 24 bpp BMP, each pixel has 3 bytes (BGR), so each row contains (width * 3)
 * pixel bytes ('row_bytes'). However, the BMP format requires every row in the
 * file to be aligned to a multiple of 4 bytes. When row_bytes is not a multiple
 * of 4, 1 to 3 padding bytes are added at the end of the row.
 *
 * 'stride' is the total row size in the file including this padding:
 *     stride = ((row_bytes + 3) / 4) * 4
 *
 * Although strict BMP manipulation skips padding bytes (using 'row_bytes * height'
 * and mapping cursor -> row * stride + col), the assignment specification
 * (Section 5.4 of Enunciado.pdf) explicitly requires using the entire contiguous
 * block of pixels + padding bytes ('stride * height') for steganography so all
 * implementations produce bit-identical results.
 */

bitmap bitmap_create(const char * restrict filename) {
    if (!filename) return nullptr;

    // Open bitmap file
    FILE * f = fopen(filename, "rb");
    if (!f) return nullptr;

    // Calculate bitmap file size and validate it is greater than the size of both headers
    if (fseek(f, 0, SEEK_END) != 0) { 
        fclose(f); 
        return nullptr; 
    }
    long size = ftell(f);
    size_t header_total_size = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    if (size < (long) header_total_size) {
        fclose(f);
        return nullptr;
    }
    rewind(f);

    // Reserve space to store the entire bitmap file in memory, to eliminate I/O overhead
    // Then, read the file into memory
    uint8_t * data = malloc((size_t) size);
    if (!data || fread(data, sizeof(uint8_t), (size_t) size, f) != (size_t) size) {
        free(data);             // free is null-safe
        fclose(f);
        return nullptr;
    }

    // Finished reading file, close it
    fclose(f);

    // Extract the headers from the read data in order to perform validations
    // The file header is extracted first, then the info header which comes after it
    BITMAPFILEHEADER fh;
    BITMAPINFOHEADER ih;
    memcpy(&fh, data, sizeof(BITMAPFILEHEADER));
    memcpy(&ih, data + sizeof(BITMAPFILEHEADER), sizeof(BITMAPINFOHEADER));

    // Validate bitmap headers
    if (   fh.bfType != 0x4D42                  // File type must always be 0x4D42 for V3 bitmaps
        || ih.biSize != 40                      // BITMAPINFOHEADER must be 40 bytes long
        || ih.biPlanes != 1                     // The number of planes must be set to 1
        || ih.biBitCount != 24                  // The bitmap should be 24 bits per pixel (bpp)
        || ih.biCompression != 0                // The type of compression must be 0, meaning no compression
        || ih.biWidth <= 0                      // Width must be greater than zero
        || ih.biHeight == 0                     // Height must not be zero (> 0 is bottom-up, < 0 is top-down)
        || fh.bfOffBits < header_total_size     // Offset to bitmap bits must exceed the size of both headers
    ) {
        free(data);
        return nullptr;
    }

    // Load relevant bitmap information
    uint32_t width = (uint32_t) ih.biWidth;             // Image width
    uint32_t height = (uint32_t) labs(ih.biHeight);     // Image height
    size_t row_bytes = (size_t) width * 3;              // Total pixel bytes per image row (3 bytes per pixel * width)
    size_t stride = ((row_bytes + 3) / 4) * 4;          // Total bytes per image row (pixel bytes + alignment padding bytes)
    bool has_padding = (stride > row_bytes);            // True if image rows require alignment padding bytes
    size_t data_offset = (size_t) fh.bfOffBits;         // Offset from the start of BITMAPFILEHEADER to the bitmap bits
    size_t total_bytes = stride * height;               // Total carrier capacity (includes inter-row padding per specification)

    // Check that the file is actually large enough to contain headers + all bytes in the image matrix (considering padding)
    if ((size_t) size < data_offset + total_bytes) {
        free(data);
        return nullptr;
    }

    // Reserve memory for a bitmap_internal structure
    bitmap b = malloc(sizeof(struct bitmap_internal));
    if (!b) { 
        free(data); 
        return nullptr; 
    }

    // Load all data (relevant information from headers + data) into the internal abstract data type structure
    *b = (struct bitmap_internal) {
        .data = data,
        .size = (size_t) size,
        .data_offset = data_offset,
        .total_bytes = total_bytes,
        .cursor = 0,
        .has_padding = has_padding
    };

    return b;
}

void bitmap_rewind(bitmap b) {
    if (b) b->cursor = 0;       // Move logical cursor to start (0)
}

size_t bitmap_get_cursor(bitmap b) {
    return b ? b->cursor : 0;
}

size_t bitmap_get_total_bytes(bitmap b) {
    return b ? b->total_bytes : 0;
}

uint8_t * bitmap_next_byte(bitmap b) {
    if (!b || b->cursor >= b->total_bytes) return nullptr;
    return &(b->data[b->data_offset + b->cursor++]);
}

bool bitmap_get_byte(bitmap b, uint8_t * out_byte) {
    uint8_t * p = bitmap_next_byte(b);
    if (!p) return false;
    if (out_byte) *out_byte = *p;
    return true;
}

bool bitmap_set_byte(bitmap b, uint8_t byte) {
    if (!b || b->cursor == 0 || b->cursor > b->total_bytes) return false;
    b->data[b->data_offset + b->cursor - 1] = byte;
    return true;
}

bool bitmap_has_padding(bitmap b) {
    return b ? b->has_padding : false;
}

bool bitmap_get_byte_at(bitmap b, size_t index, uint8_t * out_byte) {
    if (!b || index >= b->total_bytes || !out_byte) return false;
    *out_byte = b->data[b->data_offset + index];
    return true;
}

bool bitmap_save(bitmap b, const char * restrict filename) {
    if (!b || !filename) return false;
    FILE * f = fopen(filename, "wb");
    if (!f) return false;
    size_t written = fwrite(b->data, sizeof(uint8_t), b->size, f);
    fclose(f);
    return written == b->size;
}

void bitmap_destroy(bitmap b) {
    if (!b) return;
    free(b->data);
    free(b);
}
