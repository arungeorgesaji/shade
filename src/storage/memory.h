#ifndef SHADE_MEMORY_STORAGE_H
#define SHADE_MEMORY_STORAGE_H

#include "../types/data.h"
#include "../types/schema.h"
#include <stdbool.h>
#include <stdlib.h>
#include "../util/string_utils.h"
#include <stdio.h>

typedef struct {
    char* name;
    TableSchema* schema;
    DataRecord** records;
    size_t record_count;
    size_t capacity;
    uint64_t next_id;
} MemoryTable;

typedef struct {
    MemoryTable** tables;
    size_t table_count;
    size_t capacity;
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
void memory_storage_debug_info(const MemoryStorage* storage);

DataRecord** memory_table_scan(MemoryTable* table, size_t* result_count);
DataRecord** memory_table_find_ghosts(MemoryTable* table, size_t* result_count);

#endif
