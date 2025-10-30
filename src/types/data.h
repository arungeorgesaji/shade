#ifndef SHADE_DATA_H
#define SHADE_DATA_H

#include "value.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    DATA_STATE_LIVING,
    DATA_STATE_GHOST,
    DATA_STATE_EXORCISED
} DataState;

typedef struct {
    uint64_t id;
    DataState state;
    Value* values;           
    size_t value_count;
    int64_t deleted_at;      
    float ghost_strength;    
} DataRecord;

DataRecord* datarecord_create(uint64_t id, const Value* values, size_t value_count);
void datarecord_destroy(DataRecord* record);

void datarecord_mark_ghost(DataRecord* record, int64_t timestamp);
void datarecord_decay_ghost(DataRecord* record, float decay_rate);
bool datarecord_is_queryable(const DataRecord* record);

const char* datarecord_state_to_string(DataState state);

#endif
