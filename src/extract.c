#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "extract.h"
#include "cli_arguments.h"
#include "bitmap.h"
#include "steganography.h"
#include "logger.h"

#define MAX_FILE_EXTENSION_BUFF 11

extern Logger logger;

/**
 * @brief       Read a 32-bit integer from a 4-byte big-endian buffer.
 */
static uint32_t read_be32(const uint8_t in[4]) {
    return  ((uint32_t) in[0] << 24) |
            ((uint32_t) in[1] << 16) |
            ((uint32_t) in[2] << 8)  |
            ((uint32_t) in[3]);
}

static bool parse_after_unencryption(
    const uint8_t * const decrypted_payload,
    size_t decrypted_size,
    uint8_t ** const out_file_data,
    size_t * const out_file_size,
    char out_file_ext[static MAX_FILE_EXTENSION_BUFF]
) {
    // Minimum valid decrypted buffer: 4 bytes (size) + 0 file bytes + 2 bytes (".\0")
    if (!decrypted_payload || decrypted_size < sizeof(uint32_t) + 2) {
        LOG_ERROR("Decrypted payload is too small to contain valid header and extension");
        return false;
    }

    // 1. Parse 4-byte Big-Endian real file size
    uint32_t real_file_size = read_be32(decrypted_payload);
    size_t ext_offset = sizeof(uint32_t) + (size_t) real_file_size;

    if ((size_t) real_file_size > decrypted_size || ext_offset + 2 > decrypted_size) {
        LOG_ERROR("Invalid real file size (%u bytes) in decrypted payload", real_file_size);
        return false;
    }

    // 2. Parse null-terminated file extension after the file bytes
    const char * ext_start = (const char *)(decrypted_payload + ext_offset);
    size_t remaining = decrypted_size - ext_offset;
    size_t max_ext_scan = remaining < MAX_FILE_EXTENSION_BUFF ? remaining : MAX_FILE_EXTENSION_BUFF;

    size_t ext_len = 0;
    bool found_null = false;
    while (!found_null && ext_len < max_ext_scan) {
        out_file_ext[ext_len] = ext_start[ext_len];
        found_null = ext_start[ext_len] == '\0';
        ext_len++;
    }

    if (!found_null || out_file_ext[0] != '.') {
        LOG_ERROR("Decrypted file extension is invalid or missing null terminator");
        return false;
    }

    // 3. Copy real file bytes into newly allocated buffer
    uint8_t * file_data = malloc(real_file_size > 0 ? (size_t) real_file_size : 1);
    if (!file_data) {
        LOG_ERROR("Memory allocation failed for decrypted file (%u bytes)", real_file_size);
        return false;
    }
    if (real_file_size > 0) {
        memcpy(file_data, decrypted_payload + sizeof(uint32_t), (size_t) real_file_size);
    }

    *out_file_data = file_data;
    *out_file_size = (size_t) real_file_size;
    return true;
}

static bool extract_stego_payload(
    cli c,
    bitmap bmp,
    stego_alg_t steg_alg,
    size_t max_payload_capacity,
    uint8_t ** const out_file_data,
    size_t * const out_file_size,
    char out_file_ext[static MAX_FILE_EXTENSION_BUFF]
) {
    bitmap_rewind(bmp);

    bool is_encrypted = false;
    if (!cli_is_set(c, CLI_OPT_PASS_LONG, &is_encrypted)) {
        LOG_ERROR("Could not check password option from cli");
        return false;
    }

    // 1. Extract 4-byte Big-Endian payload size (real file size if unencrypted, ciphertext size if encrypted)
    uint8_t size_be[sizeof(uint32_t)] = {0};
    if (!stego_extract_bytes(steg_alg, bmp, size_be, sizeof(uint32_t))) {
        LOG_ERROR("Failed to extract payload size header from bitmap");
        return false;
    }
    uint32_t payload_size = read_be32(size_be);

    // Validate that the extracted payload size fits within the carrier's maximum capacity.
    // When encrypted, the file extension is already included inside the encrypted block (0 extra bytes).
    // When unencrypted, the extension follows the file bytes and requires at least 2 bytes ('.' + '\0').
    size_t min_extra_bytes = is_encrypted ? 0 : 2;
    if (sizeof(uint32_t) + (size_t) payload_size + min_extra_bytes > max_payload_capacity) {    // Payload can be encrypted or unencrypted
        LOG_ERROR(
            "Invalid extracted payload size (%u bytes) exceeds maximum carrier capacity for %s (%lu bytes)",
            payload_size,
            stego_alg_to_string(steg_alg),
            max_payload_capacity
        );
        return false;
    }

    // 2. Allocate memory and extract the actual payload (encrypted or unencrypted)
    uint8_t * payload = malloc(payload_size > 0 ? (size_t) payload_size : 1);
    if (!payload) {
        LOG_ERROR("Memory allocation failed for extracted payload (%u bytes)", payload_size);
        return false;
    }
    if (payload_size > 0 && !stego_extract_bytes(steg_alg, bmp, payload, (size_t) payload_size)) {
        LOG_ERROR("Failed to extract payload content (%u bytes)", payload_size);
        free(payload);
        return false;
    }

    // 3.1. If payload is encrypted: decrypt and parse (size || real file bytes || real file extension)
    if (is_encrypted) {
        uint8_t * decrypted_payload = nullptr;
        size_t decrypted_size = 0;

        /* TODO:
        decrypted_payload = decrypt(c, payload, (size_t) payload_size, &decrypted_size);
        */
        LOG_ERROR("Decryption is not yet implemented");
        free(payload);

        if (!decrypted_payload) {
            return false;
        }

        bool ok = parse_after_unencryption(
            decrypted_payload,
            decrypted_size,
            out_file_data,
            out_file_size,
            out_file_ext
        );
        free(decrypted_payload);
        return ok;
    }

    // 3.2. If payload is not encrypted: extract the null-terminated file extension (e.g. ".png\0")
    size_t ext_idx = 0;
    bool found_null = false;
    while (!found_null && ext_idx < MAX_FILE_EXTENSION_BUFF) {
        uint8_t ch = 0;
        if (!stego_extract_bytes(steg_alg, bmp, &ch, 1)) {
            LOG_ERROR("Unexpected end of carrier bitmap while extracting file extension");
            free(payload);
            return false;
        }
        out_file_ext[ext_idx++] = (char) ch;
        found_null = ch == '\0';
    }

    if (!found_null || out_file_ext[0] != '.') {
        LOG_ERROR("Extracted file extension is invalid or missing null terminator");
        free(payload);
        return false;
    }

    *out_file_data = payload;
    *out_file_size = (size_t) payload_size;

    return true;
}

int extract_main(cli c, bool verbose) {
    int exit_status = EXIT_FAILURE;
    LOG_INFO("Starting extract task");

    // Resources to free
    bitmap      bmp                 = nullptr;
    FILE *      out_file            = nullptr;
    uint8_t *   file_data           = nullptr;
    char *      full_out_filename   = nullptr;

    // Get filenames and steganography algorithm from cli
    const char * bmp_filename       = nullptr;
    if (!cli_get_string(c, CLI_OPT_PICTURE_LONG, &bmp_filename)) {
        LOG_ERROR("Could not get bitmap filename from cli");
        goto cleanup;
    }
    if (verbose) LOG_INFO("Bitmap file: \"%s\"", bmp_filename);

    const char * out_filename       = nullptr;
    if (!cli_get_string(c, CLI_OPT_OUT_LONG, &out_filename)) {
        LOG_ERROR("Could not get output file name from cli");
        goto cleanup;
    }
    if (verbose) LOG_INFO("Output base file: \"%s\"", out_filename);

    const char * steg_alg_str       = nullptr;
    stego_alg_t steg_alg;
    if (!cli_get_string(c, CLI_OPT_STEG_LONG, &steg_alg_str) || !stego_alg_from_string(steg_alg_str, &steg_alg)) {
        LOG_ERROR("Invalid or missing steganography algorithm");
        goto cleanup;
    }
    if (verbose) LOG_INFO("Steganography algorithm: %s", stego_alg_to_string(steg_alg));

    // Load bitmap and calculate maximum payload capacity
    if (!(bmp = bitmap_create(bmp_filename))) {
        LOG_ERROR("Unknown error loading bitmap: \"%s\"", bmp_filename);
        goto cleanup;
    }
    size_t carrier_capacity = bitmap_get_total_bytes(bmp);
    size_t max_payload_capacity = stego_get_max_capacity(steg_alg, bmp);
    if (verbose) {
        LOG_INFO(
            "Loaded bitmap (carrier bytes: %lu, max %s payload capacity: %lu bytes)",
            carrier_capacity,
            stego_alg_to_string(steg_alg),
            max_payload_capacity
        );
    }

    // Extract payload (file size, file bytes, and extension) from the bitmap
    size_t file_size = 0;
    char file_ext_buff[MAX_FILE_EXTENSION_BUFF] = {0};
    if (!extract_stego_payload(c, bmp, steg_alg, max_payload_capacity, &file_data, &file_size, file_ext_buff)) {
        goto cleanup;   // extract_stego_payload already logs errors
    }
    if (verbose) {
        size_t ext_len_with_term = strlen(file_ext_buff) + 1;
        size_t total_payload_size = sizeof(uint32_t) + file_size + ext_len_with_term;
        LOG_INFO(
            "Extracted payload (total size: %lu bytes = %luB size header + %luB file data + %luB extension \"%s\\0\")",
            total_payload_size,
            sizeof(uint32_t),
            file_size,
            ext_len_with_term,
            file_ext_buff
        );
    }

    // Build final output filename (<out_filename><extension>)
    size_t full_out_len = strlen(out_filename) + strlen(file_ext_buff) + 1;
    full_out_filename = malloc(full_out_len * sizeof(char));
    if (!full_out_filename) {
        LOG_ERROR("Memory allocation failed for output filename");
        goto cleanup;
    }
    snprintf(full_out_filename, full_out_len, "%s%s", out_filename, file_ext_buff);

    // Save extracted file to disk
    if (!(out_file = fopen(full_out_filename, "wb"))) {
        LOG_ERROR("Could not open output file for writing: \"%s\"", full_out_filename);
        goto cleanup;
    }
    if (file_size > 0 && fwrite(file_data, sizeof(uint8_t), file_size, out_file) != file_size) {
        LOG_ERROR("Failed to write extracted data to \"%s\"", full_out_filename);
        goto cleanup;
    }
    LOG_INFO("Saved extracted file to \"%s\"", full_out_filename);

    exit_status = EXIT_SUCCESS;

cleanup:
    if (bmp) bitmap_destroy(bmp);
    if (out_file) fclose(out_file);
    free(file_data);                        // Null-safe
    free(full_out_filename);                // Null-safe

    LOG_INFO(exit_status == EXIT_SUCCESS
        ? "Extract task succeeded"
        : "Extract task failed"
    );
    return exit_status;
}
