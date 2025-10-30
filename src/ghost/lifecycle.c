#include "lifecycle.h"
#include <stdlib.h>

bool resurrect_ghost(MemoryStorage* storage, const char* table_name, uint64_t id) {
    if (!storage || !table_name) return false;
    
    MemoryTable* table = memory_storage_get_table(storage, table_name);
    if (!table) return false;
    
    for (size_t i = 0; i < table->record_count; i++) {
        DataRecord* record = table->records[i];
        if (record->id == id && record->state == DATA_STATE_GHOST) {
            record->state = DATA_STATE_LIVING;
            record->deleted_at = 0;
            record->ghost_strength = 1.0f; 
            return true;
        }
    }
    
    return false;
}

size_t resurrect_strong_ghosts(MemoryStorage* storage, float strength_threshold) {
    if (!storage) return 0;
    
    size_t resurrected_count = 0;
    
    for (size_t t = 0; t < storage->table_count; t++) {
        MemoryTable* table = storage->tables[t];
        
        for (size_t i = 0; i < table->record_count; i++) {
            DataRecord* record = table->records[i];
            if (record->state == DATA_STATE_GHOST && record->ghost_strength >= strength_threshold) {
                record->state = DATA_STATE_LIVING;
                record->deleted_at = 0;
                record->ghost_strength = 1.0f;
                resurrected_count++;
            }
        }
    }
    
    return resurrected_count;
}

void decay_all_ghosts(MemoryStorage* storage, float decay_amount) {
    if (!storage) return;
    
    for (size_t t = 0; t < storage->table_count; t++) {
        MemoryTable* table = storage->tables[t];
        
        for (size_t i = 0; i < table->record_count; i++) {
            DataRecord* record = table->records[i];
            if (record->state == DATA_STATE_GHOST) {
                datarecord_decay_ghost(record, decay_amount);
            }
        }
    }
}
