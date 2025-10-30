#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "../src/types/data.h"
#include "../src/types/value.h"
#include "../src/types/schema.h"
#include "../src/storage/memory.h"
#include "../src/ghost/analytics.h"
#include "../src/ghost/lifecycle.h"

void test_ghost_stats_calculation() {
    printf("Testing ghost stats calculation...\n");
    
    MemoryStorage* storage = memory_storage_create();
    ColumnSchema columns[] = { column_create("id", VALUE_INTEGER) };
    TableSchema* schema = tableschema_create("test", columns, 1);
    MemoryTable* table = memory_storage_create_table(storage, "test", schema);
    
    Value values[] = { value_integer(1) };
    
    memory_table_insert(table, values);
    
    memory_table_insert(table, values);
    memory_table_delete(table, 2, time(NULL));
    
    memory_table_insert(table, values);
    DataRecord* weak_ghost = memory_table_get(table, 3);
    memory_table_delete(table, 3, time(NULL));
    datarecord_decay_ghost(weak_ghost, 0.8f); 
    
    GhostStats stats = calculate_ghost_stats(table);
    
    assert(stats.total_living == 1);
    assert(stats.total_ghosts == 2);
    assert(stats.total_exorcised == 0);
    assert(fabsf(stats.avg_ghost_strength - 0.6f) < 0.01f); 
    assert(fabsf(stats.ghost_ratio - 0.666f) < 0.01f); 
    
    memory_storage_destroy(storage);
    free((char*)columns[0].name);
    
    printf("Ghost stats calculation tests passed\n");
}

void test_ghost_resurrection() {
    printf("Testing ghost resurrection...\n");
    
    MemoryStorage* storage = memory_storage_create();
    ColumnSchema columns[] = { column_create("id", VALUE_INTEGER) };
    TableSchema* schema = tableschema_create("test", columns, 1);
    MemoryTable* table = memory_storage_create_table(storage, "test", schema);
    
    Value values[] = { value_integer(1) };
    memory_table_insert(table, values);
    memory_table_delete(table, 1, time(NULL));
    
    DataRecord* ghost = memory_table_get(table, 1);
    assert(ghost->state == DATA_STATE_GHOST);
    
    bool success = resurrect_ghost(storage, "test", 1);
    assert(success);
    
    DataRecord* living = memory_table_get(table, 1);
    assert(living->state == DATA_STATE_LIVING);
    assert(living->ghost_strength == 1.0f);
    
    memory_storage_destroy(storage);
    free((char*)columns[0].name);
    
    printf("Ghost resurrection tests passed\n");
}

void test_ghost_report() {
    printf("Testing ghost report generation...\n");
    
    MemoryStorage* storage = memory_storage_create();
    
    ColumnSchema cols1[] = { column_create("id", VALUE_INTEGER) };
    TableSchema* schema1 = tableschema_create("table1", cols1, 1);
    MemoryTable* table1 = memory_storage_create_table(storage, "table1", schema1);
    
    ColumnSchema cols2[] = { column_create("id", VALUE_INTEGER) };
    TableSchema* schema2 = tableschema_create("table2", cols2, 1);
    MemoryTable* table2 = memory_storage_create_table(storage, "table2", schema2);
    
    Value values[] = { value_integer(1) };
    
    memory_table_insert(table1, values);
    memory_table_insert(table1, values);
    memory_table_insert(table1, values);
    memory_table_delete(table1, 3, time(NULL));
    
    memory_table_insert(table2, values);
    memory_table_insert(table2, values);
    memory_table_insert(table2, values);
    memory_table_delete(table2, 2, time(NULL));
    memory_table_delete(table2, 3, time(NULL));
    
    DatabaseGhostReport* report = generate_ghost_report(storage);
    assert(report != NULL);
    assert(report->table_count == 2);
    
    assert(report->overall.total_living == 3);
    assert(report->overall.total_ghosts == 3);
    
    ghost_report_destroy(report);
    memory_storage_destroy(storage);
    free((char*)cols1[0].name);
    free((char*)cols2[0].name);
    
    printf("Ghost report tests passed\n");
}

int main() {
    printf("=== Shade Ghost Analytics Tests ===\n\n");
    
    test_ghost_stats_calculation();
    test_ghost_resurrection();
    test_ghost_report();
    
    printf("\nAll ghost analytics tests passed!\n");
    return 0;
}
