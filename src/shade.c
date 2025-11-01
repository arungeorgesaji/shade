#include "shade.h"

struct ShadeDB {
    MemoryStorage* storage;
    char* last_error;
};

struct ShadeGhostTableStats {
    char* table_name;
    size_t living_count;
    size_t ghost_count; 
    size_t exorcised_count;
    float ghost_ratio;
    float avg_ghost_strength;
    size_t strongest_ghost_id;
    float strongest_ghost_strength;
    size_t weakest_ghost_id;
    float weakest_ghost_strength;
};

struct ShadeGhostStatsResult {
    size_t table_count;
    size_t total_living;
    size_t total_ghosts;
    size_t total_exorcised;
    float overall_ghost_ratio;
    float overall_avg_strength;
    struct ShadeGhostTableStats* table_stats;
};

struct ShadeQueryResult {
    QueryResult* internal_result;
    MemoryTable* table; 
    size_t current_row;
    struct ShadeGhostStatsResult* ghost_stats;  
    bool is_ghost_stats;                 
};

static char* last_error = NULL;

static void set_error(const char* message) {
    if (last_error) free(last_error);
    last_error = message ? string_duplicate(message) : NULL;
}

static ValueType string_to_type(const char* type_str) {
    if (strcmp(type_str, "INT") == 0) return VALUE_INTEGER;
    if (strcmp(type_str, "FLOAT") == 0) return VALUE_FLOAT;
    if (strcmp(type_str, "BOOL") == 0) return VALUE_BOOLEAN;
    if (strcmp(type_str, "STRING") == 0) return VALUE_STRING;
    return VALUE_NULL;
}

static const char* type_to_string(ValueType type) {
    switch (type) {
        case VALUE_INTEGER: return "INT";
        case VALUE_FLOAT: return "FLOAT";
        case VALUE_BOOLEAN: return "BOOL";
        case VALUE_STRING: return "STRING";
        case VALUE_NULL: return "NULL";
        default: return "UNKNOWN";
    }
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

    result->is_ghost_stats = false;
    result->ghost_stats = NULL;
    
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
    
    ShadeQueryResult* result = malloc(sizeof(ShadeQueryResult));
    if (!result) {
        ghost_report_destroy(report);
        set_error("Memory allocation failed");
        return NULL;
    }
    
    result->internal_result = NULL;
    result->table = NULL;
    result->current_row = 0;
    result->is_ghost_stats = true;
    
    result->ghost_stats = malloc(sizeof(ShadeGhostStatsResult));
    if (!result->ghost_stats) {
        ghost_report_destroy(report);
        free(result);
        set_error("Memory allocation failed");
        return NULL;
    }
    
    result->ghost_stats->table_count = report->table_count;
    result->ghost_stats->total_living = report->overall.total_living;
    result->ghost_stats->total_ghosts = report->overall.total_ghosts;
    result->ghost_stats->total_exorcised = report->overall.total_exorcised;
    result->ghost_stats->overall_ghost_ratio = report->overall.ghost_ratio;
    result->ghost_stats->overall_avg_strength = report->overall.avg_ghost_strength;
    
    if (report->table_count > 0) {
        result->ghost_stats->table_stats = malloc(sizeof(ShadeGhostTableStats) * report->table_count);
        if (!result->ghost_stats->table_stats) {
            ghost_report_destroy(report);
            free(result->ghost_stats);
            free(result);
            set_error("Memory allocation failed");
            return NULL;
        }
        
        for (size_t i = 0; i < report->table_count; i++) {
            TableGhostStats* src = &report->table_stats[i];
            ShadeGhostTableStats* dest = &result->ghost_stats->table_stats[i];
            
            dest->table_name = string_duplicate(src->table_name);
            dest->living_count = src->stats.total_living;
            dest->ghost_count = src->stats.total_ghosts;
            dest->exorcised_count = src->stats.total_exorcised;
            dest->ghost_ratio = src->stats.ghost_ratio;
            dest->avg_ghost_strength = src->stats.avg_ghost_strength;
            dest->strongest_ghost_id = src->stats.strongest_ghost_id;
            dest->strongest_ghost_strength = src->stats.strongest_ghost_strength;
            dest->weakest_ghost_id = src->stats.weakest_ghost_id;
            dest->weakest_ghost_strength = src->stats.weakest_ghost_strength;
        }
    } else {
        result->ghost_stats->table_stats = NULL;
    }
    
    ghost_report_destroy(report);
    
    return result;
}

size_t shade_ghost_stats_table_count(ShadeQueryResult* result) {
    if (!result || !result->is_ghost_stats || !result->ghost_stats) return 0;
    return result->ghost_stats->table_count;
}

const char* shade_ghost_stats_table_name(ShadeQueryResult* result, size_t table_index) {
    if (!result || !result->is_ghost_stats || !result->ghost_stats) return NULL;
    if (table_index >= result->ghost_stats->table_count) return NULL;
    return result->ghost_stats->table_stats[table_index].table_name;
}

bool shade_ghost_stats_get_table_stats(ShadeQueryResult* result, size_t table_index,
                                      size_t* living_count, size_t* ghost_count,
                                      size_t* exorcised_count, float* ghost_ratio,
                                      float* avg_strength) {
    if (!result || !result->is_ghost_stats || !result->ghost_stats) return false;
    if (table_index >= result->ghost_stats->table_count) return false;
    
    ShadeGhostTableStats* stats = &result->ghost_stats->table_stats[table_index];
    
    if (living_count) *living_count = stats->living_count;
    if (ghost_count) *ghost_count = stats->ghost_count;
    if (exorcised_count) *exorcised_count = stats->exorcised_count;
    if (ghost_ratio) *ghost_ratio = stats->ghost_ratio;
    if (avg_strength) *avg_strength = stats->avg_ghost_strength;
    
    return true;
}

bool shade_ghost_stats_get_strength_info(ShadeQueryResult* result, size_t table_index,
                                        size_t* strongest_id, float* strongest_strength,
                                        size_t* weakest_id, float* weakest_strength) {
    if (!result || !result->is_ghost_stats || !result->ghost_stats) return false;
    if (table_index >= result->ghost_stats->table_count) return false;
    
    ShadeGhostTableStats* stats = &result->ghost_stats->table_stats[table_index];
    
    if (strongest_id) *strongest_id = stats->strongest_ghost_id;
    if (strongest_strength) *strongest_strength = stats->strongest_ghost_strength;
    if (weakest_id) *weakest_id = stats->weakest_ghost_id;
    if (weakest_strength) *weakest_strength = stats->weakest_ghost_strength;
    
    return true;
}

bool shade_ghost_stats_get_overall(ShadeQueryResult* result,
                                  size_t* total_living, size_t* total_ghosts,
                                  size_t* total_exorcised, float* overall_ratio,
                                  float* overall_avg_strength) {
    if (!result || !result->is_ghost_stats || !result->ghost_stats) return false;
    
    struct ShadeGhostStatsResult* stats = result->ghost_stats;
    
    if (total_living) *total_living = stats->total_living;
    if (total_ghosts) *total_ghosts = stats->total_ghosts;
    if (total_exorcised) *total_exorcised = stats->total_exorcised;
    if (overall_ratio) *overall_ratio = stats->overall_ghost_ratio;
    if (overall_avg_strength) *overall_avg_strength = stats->overall_avg_strength;
    
    return true;
}

size_t shade_result_count(ShadeQueryResult* result) {
    if (!result || !result->internal_result) return 0;
    return result->internal_result->count;
}

size_t shade_result_column_count(ShadeQueryResult* result) {
    if (!result || !result->table) return 0;
    return result->table->schema->column_count;
}

const char* shade_result_column_name(ShadeQueryResult* result, size_t col) {
    if (!result || !result->table) return NULL;
    if (col >= result->table->schema->column_count) return NULL;
    return result->table->schema->columns[col].name;
}

const char* shade_result_column_type(ShadeQueryResult* result, size_t col) {
    if (!result || !result->table) return NULL;
    if (col >= result->table->schema->column_count) return NULL;
    return type_to_string(result->table->schema->columns[col].type);
}

static Value* get_value_checked(ShadeQueryResult* result, size_t row, size_t col) {
    if (!result || !result->internal_result || !result->table) return NULL;
    if (row >= result->internal_result->count) return NULL;
    if (col >= result->table->schema->column_count) return NULL;
    
    DataRecord* record = result->internal_result->records[row];
    return &record->values[col];
}

bool shade_get_int(ShadeQueryResult* result, size_t row, size_t col, int64_t* out_value) {
    Value* value = get_value_checked(result, row, col);
    if (!value || value->type != VALUE_INTEGER) return false;
    
    if (out_value) *out_value = value->data.integer;
    return true;
}

bool shade_get_float(ShadeQueryResult* result, size_t row, size_t col, double* out_value) {
    Value* value = get_value_checked(result, row, col);
    if (!value || value->type != VALUE_FLOAT) return false;
    
    if (out_value) *out_value = value->data.float_val;
    return true;
}

bool shade_get_bool(ShadeQueryResult* result, size_t row, size_t col, bool* out_value) {
    Value* value = get_value_checked(result, row, col);
    if (!value || value->type != VALUE_BOOLEAN) return false;
    
    if (out_value) *out_value = value->data.boolean;
    return true;
}

bool shade_get_string(ShadeQueryResult* result, size_t row, size_t col, const char** out_value) {
    Value* value = get_value_checked(result, row, col);
    if (!value || value->type != VALUE_STRING) return false;
    
    if (out_value) *out_value = value->data.string;
    return true;
}

bool shade_is_null(ShadeQueryResult* result, size_t row, size_t col) {
    Value* value = get_value_checked(result, row, col);
    return value && value->type == VALUE_NULL;
}

void shade_free_result(ShadeQueryResult* result) {
    if (!result) return;
    
    if (result->internal_result) {
        queryresult_destroy(result->internal_result);
    }
    
    if (result->is_ghost_stats && result->ghost_stats) {
        if (result->ghost_stats->table_stats) {
            for (size_t i = 0; i < result->ghost_stats->table_count; i++) {
                free(result->ghost_stats->table_stats[i].table_name);
            }
            free(result->ghost_stats->table_stats);
        }
        free(result->ghost_stats);
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
