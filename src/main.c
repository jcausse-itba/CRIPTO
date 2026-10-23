#include <stdio.h>
#include <stdlib.h>
#include "cli_arguments.h"

int main(int argc, const char * argv[]) {
    
    // Parse command line arguments
    cli c;
    if (!(c = parse_command_line(argc, argv))) {
        return EXIT_FAILURE;
    }
    if (verbose(c)) {
        print_config(c);
    }


    // Do something


    // Cleanup and exit
    cli_destroy(c);

    return EXIT_SUCCESS;
}
