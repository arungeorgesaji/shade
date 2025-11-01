#include "executor.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

Query* query_create(QueryType type, const char* table_name) {
    Query* query = malloc(sizeof(Query));
    if (!query) return NULL;
    
    query->type = type;
    query->table_name = table_name ? string_duplicate(table_name) : NULL;
    query->select_columns = NULL;
    query->select_count = 0;
    query->insert_values = NULL;
    query->value_count = 0;
    query->target_id = 0;
    query->include_ghosts = false;
    query->ghost_threshold = 0.0f;
    
    return query;
}

void query_destroy(Query* query) {
    if (!query) return;
    
    free(query->table_name);
    
    if (query->select_columns) {
        for (size_t i = 0; i < query->select_count; i++) {
            free(query->select_columns[i]);
        }
        free(query->select_columns);
    }
    
    if (query->insert_values) {
        for (size_t i = 0; i < query->value_count; i++) {
            value_destroy(&query->insert_values[i]);
        }
        free(query->insert_values);
    }
    
    free(query);
}

static QueryResult* execute_select(Query* query, QueryResult* result, MemoryTable* table) {
    size_t record_count = 0;
    DataRecord** all_records = memory_table_scan(table, &record_count);
    
    if (!all_records) {
        result->count = 0;
        return result;
    }
    
    size_t match_count = 0;
    for (size_t i = 0; i < record_count; i++) {
        DataRecord* record = all_records[i];
        
        if (record->state == DATA_STATE_GHOST) {
            result->ghost_count++;
            if (!query->include_ghosts || record->ghost_strength < query->ghost_threshold) {
                continue;
            }
        } else if (record->state == DATA_STATE_EXORCISED) {
            result->exorcised_count++;
            continue;
        }
        
        match_count++;
    }
    
    if (match_count > 0) {
        result->records = malloc(sizeof(DataRecord*) * match_count);
        if (!result->records) {
            free(all_records);
            result->count = 0;
            return result;
        }
        
        size_t j = 0;
        for (size_t i = 0; i < record_count; i++) {
            DataRecord* record = all_records[i];
            
            if (record->state == DATA_STATE_GHOST) {
                if (!query->include_ghosts || record->ghost_strength < query->ghost_threshold) {
                    continue;
                }
            } else if (record->state == DATA_STATE_EXORCISED) {
                continue;
            }
            
            result->records[j++] = record;
        }
        result->count = match_count;
    }
    
    free(all_records);
    return result;
}

static QueryResult* execute_insert_query(Query* query, QueryResult* result, MemoryTable* table) {
    if (!query->insert_values || query->value_count != table->schema->column_count) {
        result->count = 0;
        return result;
    }
    
    uint64_t new_id = memory_table_insert(table, query->insert_values);
    if (new_id == 0) {
        result->count = 0;
        return result;
    }
    
    DataRecord* new_record = memory_table_get(table, new_id);
    if (new_record) {
        result->records = malloc(sizeof(DataRecord*));
        if (result->records) {
            result->records[0] = new_record;
            result->count = 1;
        }
    }
    
    return result;
}

static QueryResult* execute_delete_query(Query* query, QueryResult* result, MemoryTable* table) {
    bool success = memory_table_delete(table, query->target_id, time(NULL));
    
    if (success) {
        DataRecord* ghost_record = memory_table_get(table, query->target_id);
        if (ghost_record) {
            result->records = malloc(sizeof(DataRecord*));
            if (result->records) {
                result->records[0] = ghost_record;
                result->count = 1;
                result->ghost_count = 1;
            }
        }
    }
    
    return result;
}

static QueryResult* execute_drop_table_query(MemoryStorage* storage, Query* query, QueryResult* result) {
    bool success = memory_storage_drop_table(storage, query->table_name);
    
    if (success) {
        result->count = 0;
        return result;
    } else {
        free(result);
        return NULL;
    }
}

QueryResult* execute_query(MemoryStorage* storage, Query* query) {
    if (!storage || !query || !query->table_name) return NULL;

    if (query->type == QUERY_DROP_TABLE) {
        QueryResult* result = malloc(sizeof(QueryResult));
        if (!result) return NULL;
        
        result->records = NULL;
        result->count = 0;
        result->ghost_count = 0;
        result->exorcised_count = 0;
        
        return execute_drop_table_query(storage, query, result);
    }
    
    MemoryTable* table = memory_storage_get_table(storage, query->table_name);
    if (!table) return NULL;
    
    QueryResult* result = malloc(sizeof(QueryResult));
    if (!result) return NULL;
    
    result->records = NULL;
    result->count = 0;
    result->ghost_count = 0;
    result->exorcised_count = 0;
    
    switch (query->type) {
        case QUERY_SELECT:
            return execute_select(query, result, table);
        case QUERY_INSERT:
            return execute_insert_query(query, result, table);
        case QUERY_DELETE:
            return execute_delete_query(query, result, table);
        case QUERY_DROP_TABLE:
            free(result);
            return NULL;
        default:
            free(result);
            return NULL;
    }
}

void queryresult_destroy(QueryResult* result) {
    if (!result) return;
    
    free(result->records);
    free(result);
}

QueryResult* execute_select_all(MemoryStorage* storage, const char* table_name) {
    Query* query = query_create(QUERY_SELECT, table_name);
    if (!query) return NULL;
    query->include_ghosts = true;
    
    QueryResult* result = execute_query(storage, query);
    query_destroy(query);
    return result;
}

QueryResult* execute_insert_simple(MemoryStorage* storage, const char* table_name, Value* values, size_t value_count) {
    Query* query = query_create(QUERY_INSERT, table_name);
    if (!query) return NULL;
    
    query->insert_values = values;
    query->value_count = value_count;
    
    QueryResult* result = execute_query(storage, query);
    
    query->insert_values = NULL;
    query_destroy(query);
    
    return result;
}

bool execute_delete_simple(MemoryStorage* storage, const char* table_name, uint64_t id) {
    Query* query = query_create(QUERY_DELETE, table_name);
    if (!query) return false;
    
    query->target_id = id;
    
    QueryResult* result = execute_query(storage, query);
    bool success = (result && result->count > 0);
    
    if (result) queryresult_destroy(result);
    query_destroy(query);
    
    return success;
}
