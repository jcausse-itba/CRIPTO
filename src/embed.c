#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "embed.h"
#include "cli_arguments.h"
#include "bitmap.h"
#include "string_utils.h"
#include "logger.h"

#define MAX_FILE_EXTENSION_BUFF 11

extern Logger logger;

static uint8_t * prepare_stego_payload(
    cli c, 
    const char * const in_filename, 
    FILE * const in_file,
    size_t in_file_size,
    size_t * const out_payload_size
) {
    uint32_t real_file_size = (uint32_t) in_file_size;

    // Get input file extension
    char file_ext_buff[MAX_FILE_EXTENSION_BUFF];
    if (!get_file_extension(in_filename, false, file_ext_buff, MAX_FILE_EXTENSION_BUFF)) {
        LOG_ERROR("Could not get input file extension for path: \"%s\"", in_filename);
        return nullptr;
    }
    size_t file_ext_len_with_term = strlen(file_ext_buff) + 1;

    // Reserve memory for the unencrypted payload
    size_t unencrypted_payload_size =   sizeof(uint32_t) +              // DWORD with real embedded file size
                                        in_file_size +                  // Input file size
                                        file_ext_len_with_term;         // File extension + null terminating byte
    uint8_t * unencrypted_payload = malloc(unencrypted_payload_size);

    // Load unencrypted payload
    memcpy(unencrypted_payload, &real_file_size, sizeof(uint32_t));
    fread(unencrypted_payload + sizeof(uint32_t), sizeof(uint8_t), in_file_size, in_file);
    memcpy(unencrypted_payload + sizeof(uint32_t) + in_file_size, file_ext_buff, file_ext_len_with_term);

    // Generate new encrypted payload if requested
    uint8_t * final_payload = nullptr;
    size_t final_payload_size = 0;


    (void) c;   // TODO: remove
    /* TODO :
    if (CLI argument password is set then requires encryption) {
        final_payload = encrypt(unencrypted_payload, &final_payload_size)
    }
    else {
    */
        final_payload = unencrypted_payload;
        final_payload_size = unencrypted_payload_size;
    /* } */

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
    
    // Get filenames from cli
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

    // Open input file
    if (!(in_file = fopen(in_filename, "rb"))) {
        LOG_ERROR("Could not open input file");
        goto cleanup;
    }

    // Load bitmap
    if (!(bmp = bitmap_create(bmp_filename))) {
        LOG_ERROR("Unknown error loading bitmap: \"%s\"", bmp_filename);
        goto cleanup;
    }
    size_t carrier_capacity = bitmap_get_total_bytes(bmp);
    if (verbose) LOG_INFO("Loaded bitmap (capacity: %lu bytes)", carrier_capacity);

    // Prepare steganography payload (unencrypted)
    size_t payload_size = 0;
    payload = prepare_stego_payload(c, in_filename, in_file, in_file_size, &payload_size);
    if (!payload || payload_size == 0) goto cleanup;    // prepare_stego_payload already logs errors
    if (verbose) LOG_INFO("Prepared payload (total size: %lu)", payload_size);

    // Embed the payload into the image
    // TODO

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
