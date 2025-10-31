#include "cli.h"

#define MAX_INPUT_LENGTH 1024
#define MAX_ARGS 32

CLIState* cli_create(void) {
    CLIState* cli = malloc(sizeof(CLIState));
    if (!cli) return NULL;
    
    cli->storage = memory_storage_create();
    cli->running = true;
    cli->current_database = NULL;
    
    if (!cli->storage) {
        free(cli);
        return NULL;
    }
    
    return cli;
}

void cli_destroy(CLIState* cli) {
    if (!cli) return;
    
    memory_storage_destroy(cli->storage);
    free(cli->current_database);
    free(cli);
}

static void print_prompt() {
    printf("shade> ");
    fflush(stdout);
}

static void print_help(void) {
    printf("Shade DB Commands:\n");
    printf("  CREATE TABLE <name> (<col1 type>, <col2 type>, ...) - Create table\n");
    printf("  INSERT INTO <table> VALUES (<val1>, <val2>, ...)    - Insert data\n");
    printf("  SELECT * FROM <table>                               - Query all data\n");
    printf("  SELECT * FROM <table> WITH GHOSTS                   - Query including ghosts\n");
    printf("  DELETE FROM <table> WHERE id = <id>                 - Delete record (creates ghost)\n");
    printf("  RESURRECT <table> <id>                              - Bring ghost back to life\n");
    printf("  GHOST STATS                                         - Show ghost analytics\n");
    printf("  DECAY GHOSTS <amount>                               - Weaken all ghosts\n");
    printf("  HELP                                                - Show this help\n");
    printf("  EXIT                                                - Exit database\n");
    printf("\n");
    printf("Data types: INT, FLOAT, BOOL, STRING\n");
    printf("Example: CREATE TABLE users (id INT, name STRING, age INT)\n");
}

static ValueType parse_type(const char* type_str) {
    if (string_case_compare(type_str, "INT") == 0) return VALUE_INTEGER;
    if (string_case_compare(type_str, "FLOAT") == 0) return VALUE_FLOAT;
    if (string_case_compare(type_str, "BOOL") == 0) return VALUE_BOOLEAN;
    if (string_case_compare(type_str, "STRING") == 0) return VALUE_STRING;
    return VALUE_NULL;
}

static Value parse_value(const char* value_str, ValueType type) {
    Value value = value_null();
    
    switch (type) {
        case VALUE_INTEGER:
            value = value_integer(atoi(value_str));
            break;
        case VALUE_FLOAT:
            value = value_float(atof(value_str));
            break;
        case VALUE_BOOLEAN:
            if (string_case_compare(value_str, "true") == 0 || strcmp(value_str, "1") == 0) {
                value = value_boolean(true);
            } else {
                value = value_boolean(false);
            }
            break;
        case VALUE_STRING:
            if (value_str[0] == '"' && value_str[strlen(value_str)-1] == '"') {
                char* unquoted = string_duplicate_n(value_str + 1, strlen(value_str) - 2);
                value = value_string(unquoted);
                free(unquoted);
            } else {
                value = value_string(value_str);
            }
            break;
        default:
            break;
    }
    
    return value;
}

static bool handle_create_table(CLIState* cli, char** args, int arg_count) {
    if (arg_count < 4) {
        printf("Usage: CREATE TABLE <name> (<col1 type>, <col2 type>, ...)\n");
        return false;
    }
    
    const char* table_name = args[2];

    char columns_str_buf[512] = {0};
    for (int i = 3; i < arg_count; i++) {
        strcat(columns_str_buf, args[i]);
        strcat(columns_str_buf, " ");
    }

    char* columns_str = columns_str_buf;
    ColumnSchema columns[32];
    int column_count = 0;
    
    char* token = strtok(columns_str, "(), ");
    while (token && column_count < 32) {
        if (column_count % 2 == 0) {
            columns[column_count/2].name = string_duplicate(token);
        } else {
            columns[column_count/2].type = parse_type(token);
        }
        column_count++;
        token = strtok(NULL, "(), ");
    }
    
    column_count /= 2; 
    
    TableSchema* schema = tableschema_create(table_name, columns, column_count);
    if (!schema) {
        printf("Error: Failed to create table schema\n");
        return false;
    }
    
    MemoryTable* table = memory_storage_create_table(cli->storage, table_name, schema);
    if (!table) {
        printf("Error: Table '%s' already exists\n", table_name);
        tableschema_destroy(schema);
        return false;
    }
    
    printf("Table '%s' created with %d columns\n", table_name, column_count);
    
    for (int i = 0; i < column_count; i++) {
        free((char*)columns[i].name);
    }
    
    return true;
}

static bool handle_insert(CLIState* cli, char** args, int arg_count) {
    if (arg_count < 5) {
        printf("Usage: INSERT INTO <table> VALUES (<val1>, <val2>, ...)\n");
        return false;
    }
    
    const char* table_name = args[2];
    MemoryTable* table = memory_storage_get_table(cli->storage, table_name);
    if (!table) {
        printf("Error: Table '%s' not found\n", table_name);
        return false;
    }
    
    char values_str_buf[512] = {0};
    for (int i = 4; i < arg_count; i++) {
        strcat(values_str_buf, args[i]);
        strcat(values_str_buf, " ");
    }

    char* values_str = values_str_buf;
    Value values[32];
    size_t value_count = 0;
    
    char* token = strtok(values_str, "(), ");
    while (token && value_count < 32 && value_count < table->schema->column_count) {
        values[value_count] = parse_value(token, table->schema->columns[value_count].type);
        value_count++;
        token = strtok(NULL, "(), ");
    }
    
    if (value_count != table->schema->column_count) {
        printf("Error: Expected %zu values, got %zu\n", table->schema->column_count, value_count);
        return false;
    }
    
    QueryResult* result = execute_insert_simple(cli->storage, table_name, values, value_count);
    if (result && result->count > 0) {
        printf("Inserted record with id: %lu\n", result->records[0]->id);
        queryresult_destroy(result);
        return true;
    } else {
        printf("Error: Failed to insert record\n");
        if (result) queryresult_destroy(result);
        return false;
    }
}

static bool handle_select(CLIState* cli, char** args, int arg_count) {
    if (arg_count < 4) {
        printf("Usage: SELECT * FROM <table> [WITH GHOSTS]\n");
        return false;
    }
    
    const char* table_name = args[3];
    bool include_ghosts = false;
    
    if (arg_count >= 5 && string_case_compare(args[4], "WITH") == 0 && 
        arg_count >= 6 && string_case_compare(args[5], "GHOSTS") == 0) {
        include_ghosts = true;
    }
    
    Query* query = query_create(QUERY_SELECT, table_name);
    if (!query) return false;
    
    query->include_ghosts = include_ghosts;
    
    QueryResult* result = execute_query(cli->storage, query);
    if (!result) {
        printf("Error: Table '%s' not found\n", table_name);
        query_destroy(query);
        return false;
    }
    
    MemoryTable* table = memory_storage_get_table(cli->storage, table_name);
    
    printf("\n");
    for (size_t i = 0; i < table->schema->column_count; i++) {
        printf("%-12s", table->schema->columns[i].name);
    }
    printf("STATE\n");
    printf("------------");
    for (size_t i = 0; i < table->schema->column_count; i++) {
        printf("------------");
    }
    printf("\n");
    
    for (size_t i = 0; i < result->count; i++) {
        DataRecord* record = result->records[i];
        
        for (size_t j = 0; j < record->value_count; j++) {
            switch (record->values[j].type) {
                case VALUE_INTEGER:
                    printf("%-12ld", record->values[j].data.integer);
                    break;
                case VALUE_FLOAT:
                    printf("%-12.2f", record->values[j].data.float_val);
                    break;
                case VALUE_BOOLEAN:
                    printf("%-12s", record->values[j].data.boolean ? "true" : "false");
                    break;
                case VALUE_STRING:
                    printf("%-12s", record->values[j].data.string);
                    break;
                default:
                    printf("%-12s", "NULL");
                    break;
            }
        }
        
        const char* state = datarecord_state_to_string(record->state);
        if (record->state == DATA_STATE_GHOST) {
            printf("GHOST(%.2f)", record->ghost_strength);
        } else {
            printf("%s", state);
        }
        printf("\n");
    }
    
    printf("\n(%zu rows", result->count);
    if (result->ghost_count > 0) {
        printf(", %zu ghosts", result->ghost_count);
    }
    printf(")\n\n");
    
    queryresult_destroy(result);
    query_destroy(query);
    return true;
}

static bool handle_delete(CLIState* cli, char** args, int arg_count) {
    if (arg_count < 6) {
        printf("Usage: DELETE FROM <table> WHERE id = <id>\n");
        return false;
    }
    
    const char* table_name = args[2];
    uint64_t id = atol(args[arg_count - 1]);
    
    bool success = execute_delete_simple(cli->storage, table_name, id);
    if (success) {
        printf("Deleted record %lu (now a ghost)\n", id);
        return true;
    } else {
        printf("Error: Failed to delete record %lu\n", id);
        return false;
    }
}

static bool handle_resurrect(CLIState* cli, char** args, int arg_count) {
    if (arg_count < 4) {
        printf("Usage: RESURRECT <table> <id>\n");
        return false;
    }
    
    const char* table_name = args[1];
    uint64_t id = atol(args[2]);
    
    bool success = resurrect_ghost(cli->storage, table_name, id);
    if (success) {
        printf("Resurrected ghost %lu in table '%s'\n", id, table_name);
        return true;
    } else {
        printf("Error: Failed to resurrect ghost %lu\n", id);
        return false;
    }
}

static bool handle_ghost_stats(CLIState* cli) {
    DatabaseGhostReport* report = generate_ghost_report(cli->storage);
    if (!report) {
        printf("No data available\n");
        return false;
    }
    
    print_ghost_report(report);
    ghost_report_destroy(report);
    return true;
}

static bool handle_decay_ghosts(CLIState* cli, char** args, int arg_count) {
    if (arg_count < 3) {
        printf("Usage: DECAY GHOSTS <amount>\n");
        return false;
    }
    
    float amount = atof(args[2]);
    decay_all_ghosts(cli->storage, amount);
    
    printf("Decayed all ghosts by %.2f\n", amount);
    return true;
}

static bool handle_exit(CLIState* cli) {
    cli->running = false;
    printf("Exiting\n");
    return true;
}

static bool handle_unknown(char** args) {
    printf("Unknown command: '%s'. Type HELP for available commands.\n", args[0]);
    return false;
}

static bool execute_command(CLIState* cli, char** args, int arg_count) {
    if (arg_count == 0) return true;
    
    char* command = args[0];
    
    if (string_case_compare(command, "HELP") == 0) {
        print_help();
        return true;
    }
    else if (string_case_compare(command, "CREATE") == 0 && arg_count > 1 && 
             string_case_compare(args[1], "TABLE") == 0) {
        return handle_create_table(cli, args, arg_count);
    }
    else if (string_case_compare(command, "INSERT") == 0 && arg_count > 1 && 
             string_case_compare(args[1], "INTO") == 0) {
        return handle_insert(cli, args, arg_count);
    }
    else if (string_case_compare(command, "SELECT") == 0) {
        return handle_select(cli, args, arg_count);
    }
    else if (string_case_compare(command, "DELETE") == 0) {
        return handle_delete(cli, args, arg_count);
    }
    else if (string_case_compare(command, "RESURRECT") == 0) {
        return handle_resurrect(cli, args, arg_count);
    }
    else if (string_case_compare(command, "GHOST") == 0 && arg_count > 1 && 
             string_case_compare(args[1], "STATS") == 0) {
        return handle_ghost_stats(cli);
    }
    else if (string_case_compare(command, "DECAY") == 0 && arg_count > 1 && 
             string_case_compare(args[1], "GHOSTS") == 0) {
        return handle_decay_ghosts(cli, args, arg_count);
    }
    else if (string_case_compare(command, "EXIT") == 0 || 
             string_case_compare(command, "QUIT") == 0) {
        return handle_exit(cli);
    }
    else {
        return handle_unknown(args);
    }
}

static int parse_input(char* input, char** args) {
    int count = 0;
    char* token = strtok(input, " \t\n");
    
    while (token && count < MAX_ARGS - 1) {
        args[count++] = token;
        token = strtok(NULL, " \t\n");
    }
    
    args[count] = NULL;
    return count;
}

void cli_run(CLIState* cli) {
    printf("Shade Database v0.1.0 - Data with a Memory\n");
    printf("Type 'HELP' for commands, 'EXIT' to quit\n\n");
    
    char input[MAX_INPUT_LENGTH];
    char* args[MAX_ARGS];
    
    while (cli->running) {
        print_prompt();
        
        if (!fgets(input, sizeof(input), stdin)) {
            break;
        }
        
        input[strcspn(input, "\n")] = 0;
        
        if (strlen(input) == 0) {
            continue;
        }
        
        int arg_count = parse_input(input, args);
        execute_command(cli, args, arg_count);
    }
}
