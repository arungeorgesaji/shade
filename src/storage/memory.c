#include "memory.h"

#define INITIAL_CAPACITY 16
#define GROWTH_FACTOR 2
#define DEFAULT_BTREE_ORDER 4

static int get_primary_key_column(TableSchema* schema) {
    (void)schema;
    return 0;
}

static char* create_btree_filename(const char* data_dir, const char* table_name) {
    size_t len = strlen(data_dir) + strlen(table_name) + 10; 
    char* filename = malloc(len);
    if (!filename) return NULL;
    
    snprintf(filename, len, "%s/%s.btree", data_dir, table_name);
    return filename;
}

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
    storage->persistence_enabled = false;
    storage->data_directory = NULL;
    
    return storage;
}

void memory_storage_destroy(MemoryStorage* storage) {
    if (!storage) return;
    
    if (storage->persistence_enabled) {
        memory_storage_save(storage);
    }
    
    for (size_t i = 0; i < storage->table_count; i++) {
        MemoryTable* table = storage->tables[i];
        if (table) {
            free(table->name);
            
            if (table->primary_index) {
                btree_close(table->primary_index);
            }
            
            for (size_t j = 0; j < table->record_count; j++) {
                datarecord_destroy(table->records[j]);
            }
            free(table->records);
            
            if (table->schema) {
                tableschema_destroy(table->schema);
            }
            
            free(table);
        }
    }
    free(storage->tables);
    free(storage->data_directory);
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
    table->use_persistence = storage->persistence_enabled;
    table->primary_index = NULL;
    
    if (!table->name || !table->records) {
        free(table->name);
        free(table->records);
        free(table);
        return NULL;
    }
    
    if (storage->persistence_enabled && storage->data_directory) {
        char* btree_filename = create_btree_filename(storage->data_directory, name);
        if (btree_filename) {
            table->primary_index = btree_create(btree_filename, DEFAULT_BTREE_ORDER);
            free(btree_filename);
        }
    }
    
    storage->tables[storage->table_count++] = table;
    return table;
}

bool memory_storage_drop_table(MemoryStorage* storage, const char* name) {
    if (!storage || !name) return false;
    
    size_t table_index = -1;
    MemoryTable* table_to_drop = NULL;
    
    for (size_t i = 0; i < storage->table_count; i++) {
        if (strcmp(storage->tables[i]->name, name) == 0) {
            table_index = i;
            table_to_drop = storage->tables[i];
            break;
        }
    }
    
    if (table_index == (size_t)-1) {
        return false; 
    }
    
    if (table_to_drop) {
        if (table_to_drop->primary_index && storage->data_directory) {
            char* btree_filename = create_btree_filename(storage->data_directory, name);
            if (btree_filename) {
                remove(btree_filename); 
                free(btree_filename);
            }
            btree_close(table_to_drop->primary_index);
        }
        
        free(table_to_drop->name);
        
        for (size_t i = 0; i < table_to_drop->record_count; i++) {
            datarecord_destroy(table_to_drop->records[i]);
        }
        free(table_to_drop->records);
        
        tableschema_destroy(table_to_drop->schema);
        free(table_to_drop);
    }
    
    for (size_t i = table_index; i < storage->table_count - 1; i++) {
        storage->tables[i] = storage->tables[i + 1];
    }
    
    storage->table_count--;

    if (storage->capacity > INITIAL_CAPACITY && 
        storage->table_count * 4 <= storage->capacity) {
        
        size_t new_capacity = storage->capacity / GROWTH_FACTOR;
        if (new_capacity < INITIAL_CAPACITY) {
            new_capacity = INITIAL_CAPACITY;
        }
        
        MemoryTable** new_tables = realloc(storage->tables, 
                                         sizeof(MemoryTable*) * new_capacity);
        if (new_tables) {
            storage->tables = new_tables;
            storage->capacity = new_capacity;
        }
    }
    
    return true;
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
    uint64_t new_id = table->next_id++;
    
    if (table->primary_index) {
        int key_column = get_primary_key_column(table->schema);
        if (key_column >= 0 && (uint32_t)key_column < table->schema->column_count) {
            btree_insert(table->primary_index, &values[key_column], new_id);
        }
    }
    
    return new_id;
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

DataRecord* memory_table_get_by_key(MemoryTable* table, const Value* key, uint32_t key_column) {
    (void)key_column;
    if (!table || !key || !table->primary_index) return NULL;
    
    uint64_t* record_ids = NULL;
    uint32_t count = 0;
    
    if (btree_search(table->primary_index, key, &record_ids, &count) && count > 0) {
        DataRecord* record = memory_table_get(table, record_ids[0]);
        btree_free_results(record_ids);
        return record;
    }
    
    if (record_ids) {
        btree_free_results(record_ids);
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

DataRecord** memory_table_find_ghosts(MemoryTable* table, size_t* result_count) {
    if (!table || !result_count) return NULL;
    
    size_t count = 0;
    for (size_t i = 0; i < table->record_count; i++) {
        if (table->records[i]->state == DATA_STATE_GHOST) {
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
        if (table->records[i]->state == DATA_STATE_GHOST) {
            results[j++] = table->records[i];
        }
    }
    
    *result_count = count;
    return results;
}

uint64_t* memory_table_range_query(MemoryTable* table, const BTreeRange* range, uint32_t key_column, uint32_t* result_count) {
    (void)key_column;
    if (!table || !range || !result_count || !table->primary_index) {
        *result_count = 0;
        return NULL;
    }
    
    return btree_range_query(table->primary_index, range, result_count);
}

bool memory_storage_enable_persistence(MemoryStorage* storage, const char* data_dir) {
    if (!storage || !data_dir) return false;
    
    storage->data_directory = string_duplicate(data_dir);
    if (!storage->data_directory) return false;
    
    char command[256];
    snprintf(command, sizeof(command), "mkdir -p %s", data_dir);
    system(command);
    
    storage->persistence_enabled = true;
    
    for (size_t i = 0; i < storage->table_count; i++) {
        MemoryTable* table = storage->tables[i];
        table->use_persistence = true;
        
        if (!table->primary_index) {
            char* btree_filename = create_btree_filename(data_dir, table->name);
            if (btree_filename) {
                table->primary_index = btree_create(btree_filename, DEFAULT_BTREE_ORDER);
                free(btree_filename);
                
                if (table->primary_index) {
                    int key_column = get_primary_key_column(table->schema);
                    for (size_t j = 0; j < table->record_count; j++) {
                        if (datarecord_is_queryable(table->records[j]) && key_column >= 0) {
                            btree_insert(table->primary_index, 
                                       &table->records[j]->values[key_column], 
                                       table->records[j]->id);
                        }
                    }
                }
            }
        }
    }
    
    return true;
}

bool memory_storage_save(MemoryStorage* storage) {
    if (!storage || !storage->persistence_enabled) return false;
    
    for (size_t i = 0; i < storage->table_count; i++) {
        if (storage->tables[i]->primary_index) {
            btree_flush(storage->tables[i]->primary_index);
        }
    }
    
    return true;
}

MemoryStorage* memory_storage_load(const char* data_dir) {
    if (!data_dir) return NULL;
    
    MemoryStorage* storage = memory_storage_create();
    if (!storage) return NULL;
    
    if (!memory_storage_enable_persistence(storage, data_dir)) {
        memory_storage_destroy(storage);
        return NULL;
    }
    
    return storage;
}

bool memory_storage_flush(MemoryStorage* storage) {
    return memory_storage_save(storage);
}

void memory_storage_debug_info(const MemoryStorage* storage) {
    if (!storage) {
        printf("Storage: NULL\n");
        return;
    }
    
    printf("Storage Debug Info:\n");
    printf("  Tables: %zu / %zu (%.1f%% usage)\n", 
           storage->table_count, storage->capacity,
           (double)storage->table_count / storage->capacity * 100);
    printf("  Persistence: %s\n", storage->persistence_enabled ? "enabled" : "disabled");
    if (storage->persistence_enabled) {
        printf("  Data directory: %s\n", storage->data_directory);
    }
    
    size_t total_records = 0;
    size_t total_memory_estimate = 0;
    
    for (size_t i = 0; i < storage->table_count; i++) {
        MemoryTable* table = storage->tables[i];
        memory_table_debug_info(table);
        total_records += table->record_count;
        
        total_memory_estimate += sizeof(MemoryTable) + 
                                strlen(table->name) + 1 +
                                sizeof(DataRecord*) * table->capacity;
        
        for (size_t j = 0; j < table->record_count; j++) {
            DataRecord* record = table->records[j];
            total_memory_estimate += sizeof(DataRecord) + 
                                   sizeof(Value) * record->value_count;
        }
        
        if (table->primary_index) {
            total_memory_estimate += sizeof(BTree);
        }
    }
    
    printf("  Total records: %zu\n", total_records);
    printf("  Estimated memory: ~%zu bytes\n", total_memory_estimate);
}

void memory_table_debug_info(const MemoryTable* table) {
    if (!table) {
        printf("    Table: NULL\n");
        return;
    }
    
    printf("    Table '%s':\n", table->name);
    printf("      Records: %zu / %zu (%.1f%% usage)\n",
           table->record_count, table->capacity,
           (double)table->record_count / table->capacity * 100);
    printf("      Next ID: %lu\n", table->next_id);
    printf("      Persistence: %s\n", table->use_persistence ? "enabled" : "disabled");
    printf("      B-tree: %s\n", table->primary_index ? "present" : "not present");
    
    size_t living = 0, ghosts = 0, exorcised = 0;
    for (size_t i = 0; i < table->record_count; i++) {
        switch (table->records[i]->state) {
            case DATA_STATE_LIVING: living++; break;
            case DATA_STATE_GHOST: ghosts++; break;
            case DATA_STATE_EXORCISED: exorcised++; break;
        }
    }
    
    printf("      Record states: %zu living, %zu ghosts, %zu exorcised\n", 
           living, ghosts, exorcised);
}
