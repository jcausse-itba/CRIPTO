#include <stdio.h>
#include <stdlib.h>
#include "embed.h"
#include "logger.h"

extern Logger logger;

int embed_main(cli c, bool verbose) {
    (void) c;
    (void) verbose;
    LOG_ERROR("Embed mode is not yet implemented!");
    return EXIT_SUCCESS;
}
