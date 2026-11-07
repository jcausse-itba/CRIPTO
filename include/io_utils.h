#ifndef _IO_UTILS_H_
#define _IO_UTILS_H_

#include <stddef.h>

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

#endif // _IO_UTILS_H_
