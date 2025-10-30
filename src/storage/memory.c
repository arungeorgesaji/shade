#include "memory.h"

#define INITIAL_CAPACITY 16
#define GROWTH_FACTOR 2

MemoryStorage* memory_storage_create(void) {
    MemoryStorage* storage = malloc(sizeof(MemoryStorage));
    if (!storage) return NULL;
    
    storage->tables = malloc(sizeof(MemoryTable*) * INITIAL_CAPACITY);
    if (!storage->tables) {
        free(storage);
        return NULL;
    }
    
    storage->table_count = 0;
    storage->capacity = INITIAL_CAPACITY;
    return storage;
}

void memory_storage_destroy(MemoryStorage* storage) {
    if (!storage) return;
    
    for (size_t i = 0; i < storage->table_count; i++) {
        MemoryTable* table = storage->tables[i];
        if (table) {
            free(table->name);
            for (size_t j = 0; j < table->record_count; j++) {
                datarecord_destroy(table->records[j]);
            }
            free(table->records);
            free(table);
        }
    }
    free(storage->tables);
    free(storage);
}

MemoryTable* memory_storage_create_table(MemoryStorage* storage, const char* name, TableSchema* schema) {
    if (!storage || !name || !schema) return NULL;
    
    for (size_t i = 0; i < storage->table_count; i++) {
        if (strcmp(storage->tables[i]->name, name) == 0) {
            return NULL; 
        }
    }
    
    if (storage->table_count >= storage->capacity) {
        size_t new_capacity = storage->capacity * GROWTH_FACTOR;
        MemoryTable** new_tables = realloc(storage->tables, sizeof(MemoryTable*) * new_capacity);
        if (!new_tables) return NULL;
        storage->tables = new_tables;
        storage->capacity = new_capacity;
    }
    
    MemoryTable* table = malloc(sizeof(MemoryTable));
    if (!table) return NULL;
    
    table->name = string_duplicate(name);
    table->schema = schema; 
    table->records = malloc(sizeof(DataRecord*) * INITIAL_CAPACITY);
    table->record_count = 0;
    table->capacity = INITIAL_CAPACITY;
    table->next_id = 1;
    
    if (!table->name || !table->records) {
        free(table->name);
        free(table->records);
        free(table);
        return NULL;
    }
    
    storage->tables[storage->table_count++] = table;
    return table;
}

MemoryTable* memory_storage_get_table(MemoryStorage* storage, const char* name) {
    if (!storage || !name) return NULL;
    
    for (size_t i = 0; i < storage->table_count; i++) {
        if (strcmp(storage->tables[i]->name, name) == 0) {
            return storage->tables[i];
        }
    }
    return NULL;
}

uint64_t memory_table_insert(MemoryTable* table, const Value* values) {
    if (!table || !values) return 0;
    
    if (table->record_count >= table->capacity) {
        size_t new_capacity = table->capacity * GROWTH_FACTOR;
        DataRecord** new_records = realloc(table->records, sizeof(DataRecord*) * new_capacity);
        if (!new_records) return 0;
        table->records = new_records;
        table->capacity = new_capacity;
    }
    
    DataRecord* record = datarecord_create(table->next_id, values, table->schema->column_count);
    if (!record) return 0;
    
    table->records[table->record_count++] = record;
    return table->next_id++;
}

DataRecord* memory_table_get(MemoryTable* table, uint64_t id) {
    if (!table) return NULL;
    
    for (size_t i = 0; i < table->record_count; i++) {
        if (table->records[i]->id == id && datarecord_is_queryable(table->records[i])) {
            return table->records[i];
        }
    }
    return NULL;
}

bool memory_table_delete(MemoryTable* table, uint64_t id, int64_t timestamp) {
    if (!table) return false;
    
    for (size_t i = 0; i < table->record_count; i++) {
        if (table->records[i]->id == id && table->records[i]->state == DATA_STATE_LIVING) {
            datarecord_mark_ghost(table->records[i], timestamp);
            return true;
        }
    }
    return false;
}

DataRecord** memory_table_scan(MemoryTable* table, size_t* result_count) {
    if (!table || !result_count) return NULL;
    
    size_t count = 0;
    for (size_t i = 0; i < table->record_count; i++) {
        if (datarecord_is_queryable(table->records[i])) {
            count++;
        }
    }
    
    if (count == 0) {
        *result_count = 0;
        return NULL;
    }
    
    DataRecord** results = malloc(sizeof(DataRecord*) * count);
    if (!results) {
        *result_count = 0;
        return NULL;
    }
    
    size_t j = 0;
    for (size_t i = 0; i < table->record_count; i++) {
        if (datarecord_is_queryable(table->records[i])) {
            results[j++] = table->records[i];
        }
    }
    
    *result_count = count;
    return results;
}
