/**
 * @file        string_utils.h
 * @brief       Various string utilities.
 * @date        Oct, 2026.
*/

#ifndef _STRING_UTILS_H_
#define _STRING_UTILS_H_

/**
 * @brief       Check whether two strings match exactly, specifying case sensitivity.
 * @param[in]   a                   First string.
 * @param[in]   b                   Second string.
 * @param[in]   case_insensitive    When set to `true`, comparison is performed in a
 *                                  case-insensitive way, as `strcasecmp` does.
 * @return      `true` on string equality, `false` otherwise.
 */
bool str_equals(const char * a, const char * b, bool case_insensitive);

#endif // _STRING_UTILS_H_
