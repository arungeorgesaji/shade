#include "shade.h"

struct ShadeDB {
    MemoryStorage* storage;
    char* last_error;
};

struct ShadeQueryResult {
    QueryResult* internal_result;
    MemoryTable* table; 
    size_t current_row;
};

static char* last_error = NULL;

static void set_error(const char* message) {
    if (last_error) free(last_error);
    last_error = message ? strdup(message) : NULL;
}

static ValueType string_to_type(const char* type_str) {
    if (strcmp(type_str, "INT") == 0) return VALUE_INTEGER;
    if (strcmp(type_str, "FLOAT") == 0) return VALUE_FLOAT;
    if (strcmp(type_str, "BOOL") == 0) return VALUE_BOOLEAN;
    if (strcmp(type_str, "STRING") == 0) return VALUE_STRING;
    return VALUE_NULL;
}

static Value value_from_void(const void* value, ValueType type) {
    switch (type) {
        case VALUE_INTEGER:
            return value_integer(*(const int64_t*)value);
        case VALUE_FLOAT:
            return value_float(*(const double*)value);
        case VALUE_BOOLEAN:
            return value_boolean(*(const bool*)value);
        case VALUE_STRING:
            return value_string((const char*)value);
        default:
            return value_null();
    }
}

ShadeDB* shade_db_create(void) {
    ShadeDB* db = malloc(sizeof(ShadeDB));
    if (!db) return NULL;
    
    db->storage = memory_storage_create();
    db->last_error = NULL;
    
    if (!db->storage) {
        free(db);
        return NULL;
    }
    
    return db;
}

void shade_db_destroy(ShadeDB* db) {
    if (!db) return;
    
    memory_storage_destroy(db->storage);
    free(db->last_error);
    free(db);
}

ShadeTable* shade_create_table(ShadeDB* db, const char* name, 
                              const char** column_names, 
                              const char** column_types, 
                              size_t column_count) {
    if (!db || !name || !column_names || !column_types || column_count == 0) {
        set_error("Invalid parameters");
        return NULL;
    }
    
    ColumnSchema* columns = malloc(sizeof(ColumnSchema) * column_count);
    if (!columns) {
        set_error("Memory allocation failed");
        return NULL;
    }
    
    for (size_t i = 0; i < column_count; i++) {
        columns[i].name = string_duplicate(column_names[i]);
        columns[i].type = string_to_type(column_types[i]);
        
        if (!columns[i].name) {
            for (size_t j = 0; j < i; j++) {
                free((char*)columns[j].name);
            }
            free(columns);
            set_error("Failed to duplicate column name");
            return NULL;
        }
    }
    
    TableSchema* schema = tableschema_create(name, columns, column_count);
    
    for (size_t i = 0; i < column_count; i++) {
        free((char*)columns[i].name);
    }
    free(columns);
    
    if (!schema) {
        set_error("Failed to create table schema");
        return NULL;
    }
    
    MemoryTable* table = memory_storage_create_table(db->storage, name, schema);
    if (!table) {
        tableschema_destroy(schema);
        set_error("Table already exists or creation failed");
        return NULL;
    }
    
    return (ShadeTable*)table; 
}

bool shade_drop_table(ShadeDB* db, const char* name) {
    if (!db || !name) {
        set_error("Invalid parameters");
        return false;
    }
    
    bool success = execute_drop_table(db->storage, name);
    if (!success) {
        set_error("Table not found or drop failed");
    }
    
    return success;
}

uint64_t shade_insert(ShadeDB* db, const char* table_name, 
                     const void** values, size_t value_count) {
    if (!db || !table_name || !values) {
        set_error("Invalid parameters");
        return 0;
    }
    
    MemoryTable* table = memory_storage_get_table(db->storage, table_name);
    if (!table) {
        set_error("Table not found");
        return 0;
    }
    
    if (value_count != table->schema->column_count) {
        set_error("Wrong number of values");
        return 0;
    }
    
    Value* value_array = malloc(sizeof(Value) * value_count);
    if (!value_array) {
        set_error("Memory allocation failed");
        return 0;
    }
    
    for (size_t i = 0; i < value_count; i++) {
        value_array[i] = value_from_void(values[i], table->schema->columns[i].type);
    }
    
    uint64_t id = memory_table_insert(table, value_array);
    
    free(value_array);
    
    if (id == 0) {
        set_error("Failed to insert record");
    }
    
    return id;
}

ShadeQueryResult* shade_select(ShadeDB* db, const char* table_name, 
                              bool include_ghosts) {
    if (!db || !table_name) {
        set_error("Invalid parameters");
        return NULL;
    }
    
    Query* query = query_create(QUERY_SELECT, table_name);
    if (!query) {
        set_error("Failed to create query");
        return NULL;
    }
    
    query->include_ghosts = include_ghosts;
    
    QueryResult* internal_result = execute_query(db->storage, query);
    query_destroy(query);
    
    if (!internal_result) {
        set_error("Table not found or query failed");
        return NULL;
    }
    
    ShadeQueryResult* result = malloc(sizeof(ShadeQueryResult));
    if (!result) {
        queryresult_destroy(internal_result);
        set_error("Memory allocation failed");
        return NULL;
    }
    
    result->internal_result = internal_result;
    result->table = memory_storage_get_table(db->storage, table_name);
    result->current_row = 0;
    
    return result;
}

bool shade_delete(ShadeDB* db, const char* table_name, uint64_t id) {
    if (!db || !table_name) {
        set_error("Invalid parameters");
        return false;
    }
    
    bool success = execute_delete_simple(db->storage, table_name, id);
    if (!success) {
        set_error("Record not found or already deleted");
    }
    
    return success;
}

bool shade_resurrect(ShadeDB* db, const char* table_name, uint64_t id) {
    if (!db || !table_name) {
        set_error("Invalid parameters");
        return false;
    }
    
    bool success = resurrect_ghost(db->storage, table_name, id);
    if (!success) {
        set_error("Ghost not found or resurrection failed");
    }
    
    return success;
}

bool shade_decay_ghosts(ShadeDB* db, float amount) {
    if (!db) {
        set_error("Invalid parameters");
        return false;
    }
    
    decay_all_ghosts(db->storage, amount);
    return true;
}

ShadeQueryResult* shade_get_ghost_stats(ShadeDB* db) {
    if (!db) {
        set_error("Invalid parameters");
        return NULL;
    }
    
    DatabaseGhostReport* report = generate_ghost_report(db->storage);
    if (!report) {
        set_error("Failed to generate ghost report");
        return NULL;
    }
    
    print_ghost_report(report);
    ghost_report_destroy(report);
    
    ShadeQueryResult* result = malloc(sizeof(ShadeQueryResult));
    if (result) {
        result->internal_result = NULL;
        result->table = NULL;
        result->current_row = 0;
    }
    
    return result;
}

size_t shade_result_count(ShadeQueryResult* result) {
    if (!result || !result->internal_result) return 0;
    return result->internal_result->count;
}

void* shade_get_value(ShadeQueryResult* result, size_t row, size_t col) {
    if (!result || !result->internal_result || !result->table) return NULL;
    if (row >= result->internal_result->count) return NULL;
    if (col >= result->table->schema->column_count) return NULL;
    
    DataRecord* record = result->internal_result->records[row];
    Value* value = &record->values[col];
    
    switch (value->type) {
        case VALUE_INTEGER:
            return (void*)(intptr_t)value->data.integer;
        case VALUE_FLOAT:
            return NULL;
        case VALUE_BOOLEAN:
            return (void*)(intptr_t)value->data.boolean;
        case VALUE_STRING:
            return (void*)value->data.string;
        default:
            return NULL;
    }
}

void shade_free_result(ShadeQueryResult* result) {
    if (!result) return;
    
    if (result->internal_result) {
        queryresult_destroy(result->internal_result);
    }
    free(result);
}

const char* shade_get_error(void) {
    return last_error;
}

void shade_clear_error(void) {
    if (last_error) {
        free(last_error);
        last_error = NULL;
    }
}
