#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "embed.h"
#include "cli_arguments.h"
#include "bitmap.h"
#include "steganography.h"
#include "cryptography.h"
#include "string_utils.h"
#include "logger.h"

#define MAX_FILE_EXTENSION_BUFF 11

extern Logger logger;

/**
 * @brief       Write a 32-bit little-endian integer into a 4-byte buffer
 *              in big-endian order.
 */
static void write_be32(uint8_t out[4], uint32_t val) {
    out[0] = (uint8_t)((val >> 24) & 0xFFU);
    out[1] = (uint8_t)((val >> 16) & 0xFFU);
    out[2] = (uint8_t)((val >> 8)  & 0xFFU);
    out[3] = (uint8_t)( val        & 0xFFU);
}

static uint8_t * prepare_stego_payload(
    cli c, 
    bool verbose,
    const char * const in_filename, 
    FILE * const in_file,
    size_t in_file_size,
    size_t * const out_payload_size
) {
    uint32_t real_file_size = (uint32_t) in_file_size;

    // Get input file extension (includes leading dot, e.g. ".txt")
    char file_ext_buff[MAX_FILE_EXTENSION_BUFF];
    if (!get_file_extension(in_filename, false, file_ext_buff, MAX_FILE_EXTENSION_BUFF)) {
        LOG_ERROR("Could not get input file extension for path: \"%s\"", in_filename);
        return nullptr;
    }
    const char * full_ext = strrchr(in_filename, '.');
    if (full_ext && strlen(full_ext) >= MAX_FILE_EXTENSION_BUFF) {
        LOG_WARN(
            "Input file extension \"%s\" exceeds maximum length (%d chars) and was truncated to \"%s\"",
            full_ext,
            MAX_FILE_EXTENSION_BUFF - 1,
            file_ext_buff
        );
    }
    size_t file_ext_len_with_term = strlen(file_ext_buff) + 1;

    // Reserve memory for the unencrypted payload
    size_t unencrypted_payload_size =   sizeof(uint32_t) +              // DWORD with real embedded file size (big-endian)
                                        in_file_size +                  // Input file size
                                        file_ext_len_with_term;         // File extension + null terminating byte
    uint8_t * unencrypted_payload = malloc(unencrypted_payload_size);
    if (!unencrypted_payload) {
        LOG_ERROR("Memory allocation failed for steganography payload");
        return nullptr;
    }

    // Load unencrypted payload: 4-byte Big-Endian size || file bytes || extension with '\0'
    write_be32(unencrypted_payload, real_file_size);
    if (fread(unencrypted_payload + sizeof(uint32_t), sizeof(uint8_t), in_file_size, in_file) != in_file_size) {
        LOG_ERROR("Failed to read input file contents from \"%s\"", in_filename);
        free(unencrypted_payload);
        return nullptr;
    }
    memcpy(unencrypted_payload + sizeof(uint32_t) + in_file_size, file_ext_buff, file_ext_len_with_term);

    // Generate new encrypted payload if password is set
    bool is_encrypted = false;
    if (!cli_is_set(c, CLI_OPT_PASS_LONG, &is_encrypted)) {
        LOG_ERROR("Could not check password option from cli");
        free(unencrypted_payload);
        return nullptr;
    }

    uint8_t * final_payload = nullptr;
    size_t final_payload_size = 0;

    if (is_encrypted) {
        const char * alg_str  = nullptr;
        const char * mode_str = nullptr;
        const char * password = nullptr;
        if (    !cli_get_string(c, CLI_OPT_CIPHER_ALG_LONG, &alg_str) 
            ||  !cli_get_string(c, CLI_OPT_CIPHER_MODE_LONG, &mode_str) 
            ||  !cli_get_string(c, CLI_OPT_PASS_LONG, &password)
        ) {
            LOG_ERROR("Could not retrieve encryption parameters from cli");
            free(unencrypted_payload);
            return nullptr;
        }

        crypto_cipher cipher = crypto_resolve_cipher(alg_str, mode_str);
        size_t ciphertext_len = 0;
        uint8_t * ciphertext = crypto_encrypt(
            cipher,
            password,
            unencrypted_payload,
            unencrypted_payload_size,
            &ciphertext_len
        );
        free(unencrypted_payload);

        if (!ciphertext) {
            LOG_ERROR("Failed to encrypt payload using %s-%s", alg_str, mode_str);
            return nullptr;
        }

        // Build final payload: 4-byte Big-Endian ciphertext_len || ciphertext
        final_payload_size = sizeof(uint32_t) + ciphertext_len;
        final_payload = malloc(final_payload_size);
        if (!final_payload) {
            LOG_ERROR("Memory allocation failed for encrypted steganography payload");
            free(ciphertext);
            return nullptr;
        }
        write_be32(final_payload, (uint32_t) ciphertext_len);
        memcpy(final_payload + sizeof(uint32_t), ciphertext, ciphertext_len);
        free(ciphertext);

        if (verbose) {
            LOG_INFO(
                "Prepared encrypted payload using %s-%s (total size: %lu bytes = %luB encrypted size header + %luB ciphertext [from %luB plaintext = %luB size header + %luB file data + %luB extension \"%s\\0\"])",
                alg_str,
                mode_str,
                final_payload_size,
                sizeof(uint32_t),
                ciphertext_len,
                unencrypted_payload_size,
                sizeof(uint32_t),
                in_file_size,
                file_ext_len_with_term,
                file_ext_buff
            );
        }
    } 
    else {
        final_payload = unencrypted_payload;
        final_payload_size = unencrypted_payload_size;

        if (verbose) {
            LOG_INFO(
                "Prepared payload (total size: %lu bytes = %luB size header + %luB file data + %luB extension \"%s\\0\")",
                final_payload_size,
                sizeof(uint32_t),
                in_file_size,
                file_ext_len_with_term,
                file_ext_buff
            );
        }
    }

    * out_payload_size = final_payload_size;
    return final_payload;
}

int embed_main(cli c, bool verbose, size_t in_file_size) {
    int exit_status = EXIT_FAILURE;
    LOG_INFO("Starting embed task");

    // Resources to free
    bitmap      bmp             = nullptr;
    FILE *      in_file         = nullptr;
    uint8_t *   payload         = nullptr;
    
    // Get filenames and steganography algorithm from cli
    const char * in_filename    = nullptr;
    if (!cli_get_string(c, CLI_OPT_IN_LONG, &in_filename)) {
        LOG_ERROR("Could not get input file name from cli");
        goto cleanup;
    }
    if (verbose) LOG_INFO("Input file: \"%s\"", in_filename);
    
    const char * bmp_filename   = nullptr;
    if (!cli_get_string(c, CLI_OPT_PICTURE_LONG, &bmp_filename)) {
        LOG_ERROR("Could not get bitmap filename from cli");
        goto cleanup;
    }
    if (verbose) LOG_INFO("Bitmap file: \"%s\"", bmp_filename);

    const char * out_filename   = nullptr;
    if (!cli_get_string(c, CLI_OPT_OUT_LONG, &out_filename)) {
        LOG_ERROR("Could not get output file name from cli");
        goto cleanup;
    }
    if (verbose) LOG_INFO("Output file: \"%s\"", out_filename);

    const char * steg_alg_str   = nullptr;
    stego_alg_t steg_alg;
    if (!cli_get_string(c, CLI_OPT_STEG_LONG, &steg_alg_str) || !stego_alg_from_string(steg_alg_str, &steg_alg)) {
        LOG_ERROR("Invalid or missing steganography algorithm");
        goto cleanup;
    }
    if (verbose) LOG_INFO("Steganography algorithm: %s", stego_alg_to_string(steg_alg));

    // Open input file
    if (!(in_file = fopen(in_filename, "rb"))) {
        LOG_ERROR("Could not open input file");
        goto cleanup;
    }

    // Load bitmap and calculate the maximum payload capacity (maximum amount of bytes the
    // carrier can have for the selected steganography algorithm)
    if (!(bmp = bitmap_create(bmp_filename))) {
        LOG_ERROR("Unknown error loading bitmap: \"%s\"", bmp_filename);
        goto cleanup;
    }
    if (steg_alg == STEGO_ALG_LSBI && bitmap_has_padding(bmp)) {
        LOG_WARN(
            "Carrier bitmap \"%s\" contains row padding bytes; LSBI assumes images without padding, which may desynchronize channel pattern alignment",
            bmp_filename
        );
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

    // Prepare steganography payload
    size_t payload_size = 0;
    payload = prepare_stego_payload(c, verbose, in_filename, in_file, in_file_size, &payload_size);
    if (!payload || payload_size == 0) goto cleanup;    // prepare_stego_payload already logs errors

    // Check carrier capacity
    if (payload_size > max_payload_capacity) {
        LOG_ERROR(
            "Carrier bitmap cannot hold the payload (required: %lu bytes, maximum carrier capacity for %s: %lu bytes)",
            payload_size,
            stego_alg_to_string(steg_alg),
            max_payload_capacity
        );
        goto cleanup;
    }

    // Embed the payload into the image
    if (!stego_embed(steg_alg, bmp, payload, payload_size)) {
        LOG_ERROR("Failed to embed payload into bitmap using %s", stego_alg_to_string(steg_alg));
        goto cleanup;
    }
    if (verbose) LOG_INFO("Embedded %lu bytes into bitmap using %s", payload_size, stego_alg_to_string(steg_alg));

    // Save the modified bitmap to the output file
    if (!bitmap_save(bmp, out_filename)) {
        LOG_ERROR("Could not save output bitmap to \"%s\"", out_filename);
        goto cleanup;
    }
    LOG_INFO("Saved output bitmap to \"%s\"", out_filename);

    exit_status = EXIT_SUCCESS;

cleanup:
    if (bmp) bitmap_destroy(bmp);
    if (in_file) fclose(in_file);
    free(payload);                          // Null-safe

    LOG_INFO(exit_status == EXIT_SUCCESS 
        ? "Embed task succeeded" 
        : "Embed task failed"
    );
    return exit_status;
}
