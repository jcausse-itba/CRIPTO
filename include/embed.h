#ifndef _EMBED_H_
#define _EMBED_H_

#include "cli.h"

/**
 * @brief       Run the `embed` task.
 * 
 * @param[in]   c               Parsed command-line arguments.
 * @param[in]   verbose         When `true`, verbose logging is enabled (otherwise disabled).
 * @param[in]   in_file_size    Size of the `-in` file used to calculate whether the content
 *                              can be embeded into the image or not.
 * 
 * @return      `EXIT_SUCCESS` on success,  `EXIT_FAILURE` otherwise.
 */
int embed_main(cli c, bool verbose, size_t in_file_size);

#endif // _EMBED_H_
