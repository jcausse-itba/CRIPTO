/**
 * @file        string_utils.h
 * @brief       Various string utilities.
 * @date        Oct, 2026.
*/

#ifndef _STRING_UTILS_H_
#define _STRING_UTILS_H_

#include <stddef.h>
#include <stdbool.h>

/**
 * @brief       Check whether two strings match exactly, specifying case sensitivity.
 * @param[in]   a                   First string.
 * @param[in]   b                   Second string.
 * @param[in]   case_insensitive    When set to `true`, comparison is performed in a
 *                                  case-insensitive way, as `strcasecmp` does.
 * @return      `true` on string equality, `false` otherwise.
 */
bool str_equals(const char * a, const char * b, bool case_insensitive);

/**
 * @brief       Get a file's extension.
 * @details     File extensions are returned with the starting dot. E.g. for a file
 *              named "my_spectacular_file.pdf.txt", ".txt" is returned, as it
 *              corresponds to the real extension (substring after last dot).
 * @param[in]   filename        The filename to process.
 * @param[in]   lowercase       When set to `true`, output will be converted to lowercase.
 * @param[out]  out_extension   A pointer to a large enough memory location to write
 *                              the extension.
 * @param[in]   max_size        Maximum amount of bytes to be written at `out_extension`,
 *                              including the null-terminating byte '\0'.
 */
void get_file_extension(const char * restrict filename, bool lowercase, char * const out_extension, size_t max_size);

#endif // _STRING_UTILS_H_
