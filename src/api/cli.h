#ifndef SHADE_CLI_H
#define SHADE_CLI_H

#include "../storage/memory.h"
#include "../ghost/analytics.h"
#include "../types/schema.h"
#include "../types/value.h"
#include "../query/executor.h"
#include "../ghost/lifecycle.h"
#include "../ghost/analytics.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

typedef struct {
    MemoryStorage* storage;
    bool running;
    char* current_database;
} CLIState;

CLIState* cli_create(void);
void cli_destroy(CLIState* cli);

void cli_run(CLIState* cli);

typedef bool (*CommandHandler)(CLIState* cli, char** args, int arg_count);

void cli_register_commands(void);

#endif
