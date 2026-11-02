#ifndef _IO_UTILS_H_
#define _IO_UTILS_H_

/**
 * @brief       Check if a file exists and can be opened.
 * 
 * @param[in]   filename    File name or path of the file.
 * 
 * @returns     `true` if the file exists and can be opened.
 *              `false` otherwise.
 */
bool file_exists(const char *filename);

#endif // _IO_UTILS_H_
