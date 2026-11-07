#ifndef _EMBED_H_
#define _EMBED_H_

#include "cli.h"

/**
 * @brief       Run the `embed` task.
 * 
 * @param[in]   c           Parsed command-line arguments.
 * @param[in]   verbose     When `true`, verbose logging is enabled (otherwise disabled).
 * 
 * @return      `EXIT_SUCCESS` on success,  `EXIT_FAILURE` otherwise.
 */
int embed_main(cli c, bool verbose);

#endif // _EMBED_H_
