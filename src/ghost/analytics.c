#include "analytics.h"

GhostStats calculate_ghost_stats(MemoryTable* table) {
    GhostStats stats = {0};
    
    if (!table) return stats;
    
    size_t record_count = 0;
    DataRecord** records = memory_table_scan(table, &record_count);
    
    if (!records) return stats;
    
    float total_strength = 0.0f;
    stats.strongest_ghost_strength = -1.0f;
    stats.weakest_ghost_strength = 2.0f; 
    
    for (size_t i = 0; i < record_count; i++) {
        DataRecord* record = records[i];
        
        switch (record->state) {
            case DATA_STATE_LIVING:
                stats.total_living++;
                break;
            case DATA_STATE_GHOST:
                stats.total_ghosts++;
                total_strength += record->ghost_strength;
                
                if (record->ghost_strength > stats.strongest_ghost_strength) {
                    stats.strongest_ghost_strength = record->ghost_strength;
                    stats.strongest_ghost_id = record->id;
                }
                if (record->ghost_strength < stats.weakest_ghost_strength) {
                    stats.weakest_ghost_strength = record->ghost_strength;
                    stats.weakest_ghost_id = record->id;
                }
                break;
            case DATA_STATE_EXORCISED:
                stats.total_exorcised++;
                break;
        }
    }
    
    if (stats.total_ghosts > 0) {
        stats.avg_ghost_strength = total_strength / stats.total_ghosts;
    }
    
    size_t total_active = stats.total_living + stats.total_ghosts;
    if (total_active > 0) {
        stats.ghost_ratio = (float)stats.total_ghosts / total_active;
    }
    
    free(records);
    return stats;
}

DatabaseGhostReport* generate_ghost_report(MemoryStorage* storage) {
    if (!storage) return NULL;
    
    DatabaseGhostReport* report = malloc(sizeof(DatabaseGhostReport));
    if (!report) return NULL;
    
    report->table_stats = malloc(sizeof(TableGhostStats) * storage->table_count);
    report->table_count = storage->table_count;
    report->overall = (GhostStats){0};
    
    if (!report->table_stats) {
        free(report);
        return NULL;
    }
    
    for (size_t i = 0; i < storage->table_count; i++) {
        MemoryTable* table = storage->tables[i];
        report->table_stats[i].table_name = string_duplicate(table->name);
        report->table_stats[i].stats = calculate_ghost_stats(table);
        
        report->overall.total_living += report->table_stats[i].stats.total_living;
        report->overall.total_ghosts += report->table_stats[i].stats.total_ghosts;
        report->overall.total_exorcised += report->table_stats[i].stats.total_exorcised;
    }
    
    if (report->overall.total_ghosts > 0) {
        float weighted_strength = 0.0f;
        size_t total_ghosts_for_avg = 0;
        
        for (size_t i = 0; i < report->table_count; i++) {
            weighted_strength += report->table_stats[i].stats.avg_ghost_strength * 
                               report->table_stats[i].stats.total_ghosts;
            total_ghosts_for_avg += report->table_stats[i].stats.total_ghosts;
        }
        
        if (total_ghosts_for_avg > 0) {
            report->overall.avg_ghost_strength = weighted_strength / total_ghosts_for_avg;
        }
    }
    
    size_t total_active = report->overall.total_living + report->overall.total_ghosts;
    if (total_active > 0) {
        report->overall.ghost_ratio = (float)report->overall.total_ghosts / total_active;
    }
    
    return report;
}

void ghost_report_destroy(DatabaseGhostReport* report) {
    if (!report) return;
    
    for (size_t i = 0; i < report->table_count; i++) {
        free(report->table_stats[i].table_name);
    }
    free(report->table_stats);
    free(report);
}

void print_ghost_report(const DatabaseGhostReport* report) {
    if (!report) return;
    
    printf("=== SHADE GHOST ANALYTICS REPORT ===\n\n");
    printf("OVERALL DATABASE STATS:\n");
    printf("  Living records: %zu\n", report->overall.total_living);
    printf("  Ghost records: %zu\n", report->overall.total_ghosts);
    printf("  Exorcised records: %zu\n", report->overall.total_exorcised);
    printf("  Ghost ratio: %.2f%%\n", report->overall.ghost_ratio * 100);
    printf("  Average ghost strength: %.2f\n\n", report->overall.avg_ghost_strength);
    
    printf("TABLE BREAKDOWN:\n");
    for (size_t i = 0; i < report->table_count; i++) {
        const TableGhostStats* table = &report->table_stats[i];
        printf("  %s:\n", table->table_name);
        printf("    Living: %zu, Ghosts: %zu, Exorcised: %zu\n",
               table->stats.total_living, table->stats.total_ghosts, table->stats.total_exorcised);
        printf("    Ghost ratio: %.2f%%, Avg strength: %.2f\n",
               table->stats.ghost_ratio * 100, table->stats.avg_ghost_strength);
        
        if (table->stats.total_ghosts > 0) {
            printf("    Strongest ghost: #%zu (%.2f), Weakest: #%zu (%.2f)\n",
                   table->stats.strongest_ghost_id, table->stats.strongest_ghost_strength,
                   table->stats.weakest_ghost_id, table->stats.weakest_ghost_strength);
        }
        printf("\n");
    }
}
