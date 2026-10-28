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
 *              named `"my_spectacular_file.pdf.txt"`, `".txt"` is returned, as it
 *              corresponds to the real extension (substring after last dot).
 *              If `filename` is a relative or absolute path to the file, only the part
 *              after the last directory separator (`/` for UNIX, `\` for Windows) is taken
 *              into consideration.
 *              On failure, this function writes a null-terminating byte to the output buffer.
 * @param[in]   filename        The filename to process. Must be not null.
 * @param[in]   lowercase       When set to `true`, output will be converted to lowercase.
 * @param[out]  out_extension   A pointer to a large enough memory location to write
 *                              the extension. Must be not null.
 * @param[in]   max_size        Maximum amount of bytes to be written at `out_extension`,
 *                              including the null-terminating byte `'\0'`. Must be > 0.
 * @return      `true` on success retrieving the extension. `false` if no extension was found,
 *              or on parameter errors.
 */
bool get_file_extension(const char * restrict filename, bool lowercase, char * const out_extension, size_t max_size);

#endif // _STRING_UTILS_H_
