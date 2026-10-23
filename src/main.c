#include <stdio.h>
#include "cli_arguments.h"

int main(int argc, const char * argv[]) {
    cli c = parse_command_line(argc, argv);

    // Do something

    if (c) {
        cli_destroy(c);
    }

    return 0;
}
