#ifndef SHADE_GHOST_LIFECYCLE_H
#define SHADE_GHOST_LIFECYCLE_H

#include "../storage/memory.h"

bool resurrect_ghost(MemoryStorage* storage, const char* table_name, uint64_t id);
size_t resurrect_strong_ghosts(MemoryStorage* storage, float strength_threshold);

void decay_all_ghosts(MemoryStorage* storage, float decay_amount);
size_t cleanup_exorcised(MemoryStorage* storage); 

#endif
