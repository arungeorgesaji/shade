#include "data.h"

DataRecord* datarecord_create(uint64_t id, const Value* values, size_t value_count) {
    DataRecord* record = malloc(sizeof(DataRecord));
    if (!record) return NULL;
    
    record->id = id;
    record->state = DATA_STATE_LIVING;
    record->value_count = value_count;
    record->deleted_at = 0;
    record->ghost_strength = 1.0f;
    
    if (value_count > 0) {
        record->values = malloc(sizeof(Value) * value_count);
        if (!record->values) {
            free(record);
            return NULL;
        }
        
        for (size_t i = 0; i < value_count; i++) {
            record->values[i] = value_clone(&values[i]);
        }
    } else {
        record->values = NULL;
    }
    
    return record;
}

void datarecord_destroy(DataRecord* record) {
    if (!record) return;
    
    if (record->values) {
        for (size_t i = 0; i < record->value_count; i++) {
            value_destroy(&record->values[i]);
        }
        free(record->values);
    }
    
    free(record);
}

void datarecord_mark_ghost(DataRecord* record, int64_t timestamp) {
    record->state = DATA_STATE_GHOST;
    record->deleted_at = timestamp;
    record->ghost_strength = 1.0f;
}

void datarecord_decay_ghost(DataRecord* record, float decay_rate) {
    record->ghost_strength -= decay_rate;
    if (record->ghost_strength <= 0.0f) {
        record->state = DATA_STATE_EXORCISED;
        record->ghost_strength = 0.0f;
    }
}

bool datarecord_is_queryable(const DataRecord* record) {
    return record->state != DATA_STATE_EXORCISED;
}

const char* datarecord_state_to_string(DataState state) {
    switch (state) {
        case DATA_STATE_LIVING: return "LIVING";
        case DATA_STATE_GHOST: return "GHOST";
        case DATA_STATE_EXORCISED: return "EXORCISED";
        default: return "UNKNOWN";
    }
}
