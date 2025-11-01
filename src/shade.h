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
typedef struct ShadeGhostTableStats ShadeGhostTableStats;
typedef struct ShadeGhostStatsResult ShadeGhostStatsResult;
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

bool shade_ghost_stats_get_table_stats(ShadeQueryResult* result, size_t table_index, size_t* living_count, size_t* ghost_count, size_t* exorcised_count, float* ghost_ratio, float* avg_strength);
bool shade_ghost_stats_get_strength_info(ShadeQueryResult* result, size_t table_index, size_t* strongest_id, float* strongest_strength, size_t* weakest_id, float* weakest_strength);
bool shade_ghost_stats_get_overall(ShadeQueryResult* result, size_t* total_living, size_t* total_ghosts, size_t* total_exorcised, float* overall_ratio, float* overall_avg_strength);
size_t shade_ghost_stats_table_count(ShadeQueryResult* result);
const char* shade_ghost_stats_table_name(ShadeQueryResult* result, size_t index);

size_t shade_result_count(ShadeQueryResult* result);
size_t shade_result_column_count(ShadeQueryResult* result);
const char* shade_result_column_name(ShadeQueryResult* result, size_t col);
const char* shade_result_column_type(ShadeQueryResult* result, size_t col);

bool shade_get_int(ShadeQueryResult* result, size_t row, size_t col, int64_t* out_value);
bool shade_get_float(ShadeQueryResult* result, size_t row, size_t col, double* out_value);
bool shade_get_bool(ShadeQueryResult* result, size_t row, size_t col, bool* out_value);
bool shade_get_string(ShadeQueryResult* result, size_t row, size_t col, const char** out_value);

bool shade_is_null(ShadeQueryResult* result, size_t row, size_t col);

void shade_free_result(ShadeQueryResult* result);

const char* shade_get_error(void);
void shade_clear_error(void);

#endif
