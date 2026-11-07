#include <stdio.h>
#include <stdlib.h>
#include "embed.h"
#include "logger.h"

extern Logger logger;

int embed_main(cli c, bool verbose, size_t in_file_size) {
    (void) c;
    (void) verbose;
    (void) in_file_size;
    LOG_ERROR("Embed mode is not yet implemented!");
    return EXIT_SUCCESS;
}
