#ifndef SHADE_GHOST_ANALYTICS_H
#define SHADE_GHOST_ANALYTICS_H

#include "../storage/memory.h"
#include <stdlib.h>
#include <stdio.h>

typedef struct {
    size_t total_ghosts;
    size_t total_living;
    size_t total_exorcised;
    float avg_ghost_strength;
    float ghost_ratio;
    size_t strongest_ghost_id;
    float strongest_ghost_strength;
    size_t weakest_ghost_id;
    float weakest_ghost_strength;
} GhostStats;

typedef struct {
    char* table_name;
    GhostStats stats;
} TableGhostStats;

typedef struct {
    TableGhostStats* table_stats;
    size_t table_count;
    GhostStats overall;
} DatabaseGhostReport;

GhostStats calculate_ghost_stats(MemoryTable* table);
DatabaseGhostReport* generate_ghost_report(MemoryStorage* storage);
void ghost_report_destroy(DatabaseGhostReport* report);
void print_ghost_report(const DatabaseGhostReport* report);

typedef struct {
    size_t resurrection_count;
    size_t potential_resurrections; 
    float avg_time_as_ghost; 
} ResurrectionStats;

ResurrectionStats analyze_resurrection_potential(MemoryStorage* storage);

typedef struct {
    size_t queries_influenced; 
    float influence_strength; 
    size_t most_influential_ghost_id;
} GhostInfluenceStats;

#endif
