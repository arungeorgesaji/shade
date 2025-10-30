#ifndef SHADE_QUERY_EXECUTOR_H
#define SHADE_QUERY_EXECUTOR_H

#include "../storage/memory.h"
#include "../types/data.h"
#include <stdbool.h>
#include <stdlib.h>
#include "../util/string_utils.h"
#include <stdio.h>
#include <time.h>

typedef enum {
    QUERY_SELECT,
    QUERY_INSERT,
    QUERY_DELETE,
    QUERY_UPDATE
} QueryType;

typedef struct {
    QueryType type;
    char* table_name;
    
    char** select_columns;
    size_t select_count;
    
    Value* insert_values;
    size_t value_count;
    
    uint64_t target_id;
    
    bool include_ghosts;
    float ghost_threshold; 
} Query;

typedef struct {
    DataRecord** records;
    size_t count;
    size_t ghost_count;
    size_t exorcised_count;
} QueryResult;

Query* query_create(QueryType type, const char* table_name);
void query_destroy(Query* query);

QueryResult* execute_query(MemoryStorage* storage, Query* query);
void queryresult_destroy(QueryResult* result);

QueryResult* execute_select_all(MemoryStorage* storage, const char* table_name);
QueryResult* execute_insert_simple(MemoryStorage* storage, const char* table_name, Value* values, size_t value_count);
bool execute_delete_simple(MemoryStorage* storage, const char* table_name, uint64_t id);

#endif
