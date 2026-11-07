#include <stdio.h>
#include <stdlib.h>
#include "extract.h"
#include "logger.h"

extern Logger logger;

int extract_main(cli c, bool verbose) {
    (void) c;
    (void) verbose;
    LOG_ERROR("Extract mode is not yet implemented!");
    return EXIT_SUCCESS;
}
