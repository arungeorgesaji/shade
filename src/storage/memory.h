#ifndef SHADE_MEMORY_STORAGE_H
#define SHADE_MEMORY_STORAGE_H

#include "../types/data.h"
#include "../types/schema.h"
#include "btree.h"
#include <stdbool.h>
#include <stdlib.h>
#include "../util/string_utils.h"
#include <stdio.h>

typedef struct BTree BTree;
typedef struct BTreeRange BTreeRange;

typedef struct {
    char* name;
    TableSchema* schema;
    DataRecord** records;
    size_t record_count;
    size_t capacity;
    uint64_t next_id;

    BTree* primary_index;
    bool use_persistence;
} MemoryTable;

typedef struct {
    MemoryTable** tables;
    size_t table_count;
    size_t capacity;

    bool persistence_enabled;
    char* data_directory;
} MemoryStorage;

MemoryStorage* memory_storage_create(void);
void memory_storage_destroy(MemoryStorage* storage);

MemoryTable* memory_storage_create_table(MemoryStorage* storage, const char* name, TableSchema* schema);
bool memory_storage_drop_table(MemoryStorage* storage, const char* name);
MemoryTable* memory_storage_get_table(MemoryStorage* storage, const char* name);

uint64_t memory_table_insert(MemoryTable* table, const Value* values);
DataRecord* memory_table_get(MemoryTable* table, uint64_t id);
bool memory_table_update(MemoryTable* table, uint64_t id, const Value* values);
bool memory_table_delete(MemoryTable* table, uint64_t id, int64_t timestamp);
DataRecord** memory_table_scan(MemoryTable* table, size_t* result_count);
DataRecord** memory_table_find_ghosts(MemoryTable* table, size_t* result_count);

DataRecord* memory_table_get_by_key(MemoryTable* table, const Value* key, uint32_t key_column);
uint64_t* memory_table_range_query(MemoryTable* table, const BTreeRange* range, uint32_t key_column, uint32_t* result_count);

bool memory_storage_enable_persistence(MemoryStorage* storage, const char* data_dir);
bool memory_storage_save(MemoryStorage* storage);
MemoryStorage* memory_storage_load(const char* data_dir);
bool memory_storage_flush(MemoryStorage* storage);

void memory_storage_debug_info(const MemoryStorage* storage);
void memory_table_debug_info(const MemoryTable* table);

#endif 
