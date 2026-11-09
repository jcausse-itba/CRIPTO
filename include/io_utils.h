#ifndef _IO_UTILS_H_
#define _IO_UTILS_H_

#include <stddef.h>
#include "cli.h"

/**
 * @brief       Check if a file exists and can be opened. If so, get its size in bytes.
 *              Passing `nullptr` to `out_size` parameter skips file size calculation.
 * 
 * @param[in]   filename    File name or path of the file.
 * @param[out]  out_size    A pointer to a location where to store the file's size in bytes
 *                          (or `nullptr` to skip file size calculation).
 * 
 * @returns     `true` if the file exists and can be opened.
 *              `false` otherwise.
 */
bool get_file_size(const char * restrict filename, size_t * out_size);

/**
 * @brief       Check if the carrier bitmap file specified in the CLI (`-p` / `--picture`)
 *              exists and can be opened. Logs an error message if the file is not found.
 * 
 * @param[in]   c           Parsed CLI instance.
 * 
 * @returns     `true` if the bitmap file exists and can be opened, `false` otherwise.
 */
bool bitmap_file_exists(cli c);

/**
 * @brief       Check if the input file specified in the CLI (`-in`) exists and can be
 *              opened when `-embed` mode is active, and retrieve its size in bytes.
 *              When `-embed` is not set (e.g., in `-extract` mode), returns `true` immediately.
 *              Logs an error message if `-embed` is set and the file is not found.
 * 
 * @param[in]   c           Parsed CLI instance.
 * @param[out]  out_size    Pointer to store the input file's size in bytes (or `nullptr` to skip).
 * 
 * @returns     `true` if `-embed` is not set, or if the input file exists and can be opened;
 *              `false` otherwise.
 */
bool in_file_exists(cli c, size_t * out_size);

#endif // _IO_UTILS_H_
