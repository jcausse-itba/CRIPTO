#ifndef _EXTRACT_H_
#define _EXTRACT_H_

#include "cli.h"

/**
 * @brief       Run the `extract` task.
 * 
 * @param[in]   c           Parsed command-line arguments.
 * @param[in]   verbose     When `true`, verbose logging is enabled (otherwise disabled).
 * 
 * @return      `EXIT_SUCCESS` on success,  `EXIT_FAILURE` otherwise.
 */
int extract_main(cli c, bool verbose);

#endif // _EXTRACT_H_
