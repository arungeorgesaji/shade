#ifndef SHADE_DB_H
#define SHADE_DB_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "storage/memory.h"
#include "types/schema.h"
#include "types/value.h"
#include "query/executor.h"
#include "ghost/analytics.h"
#include "ghost/lifecycle.h"

typedef struct ShadeDB ShadeDB;
typedef struct ShadeTable ShadeTable;
typedef struct ShadeQueryResult ShadeQueryResult;

ShadeDB* shade_db_create(void);
void shade_db_destroy(ShadeDB* db);

ShadeTable* shade_create_table(ShadeDB* db, const char* name, 
                              const char** column_names, 
                              const char** column_types, 
                              size_t column_count);
bool shade_drop_table(ShadeDB* db, const char* name);

uint64_t shade_insert(ShadeDB* db, const char* table_name, 
                     const void** values, size_t value_count);
ShadeQueryResult* shade_select(ShadeDB* db, const char* table_name, 
                              bool include_ghosts);
bool shade_delete(ShadeDB* db, const char* table_name, uint64_t id);
bool shade_resurrect(ShadeDB* db, const char* table_name, uint64_t id);

bool shade_decay_ghosts(ShadeDB* db, float amount);
ShadeQueryResult* shade_get_ghost_stats(ShadeDB* db);

size_t shade_result_count(ShadeQueryResult* result);
void* shade_get_value(ShadeQueryResult* result, size_t row, size_t col);
void shade_free_result(ShadeQueryResult* result);

const char* shade_get_error(void);
void shade_clear_error(void);

#endif
