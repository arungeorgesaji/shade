#include <stdio.h>
#include "api/cli.h"

int main() {
    printf("Starting Shade Database...\n");
    
    CLIState* cli = cli_create();
    if (!cli) {
        fprintf(stderr, "Failed to initialize database\n");
        return 1;
    }
    
    cli_run(cli);
    cli_destroy(cli);
    
    return 0;
}
