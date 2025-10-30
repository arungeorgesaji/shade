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
#include "../src/query/executor.h"
#include "../src/query/planner.h"

void test_query_creation() {
    printf("Testing query creation...\n");
    
    Query* query = query_create(QUERY_SELECT, "users");
    assert(query != NULL);
    assert(query->type == QUERY_SELECT);
    assert(strcmp(query->table_name, "users") == 0);
    
    query_destroy(query);
    printf("Query creation tests passed\n");
}

void test_basic_select() {
    printf("Testing basic SELECT queries...\n");
    
    MemoryStorage* storage = memory_storage_create();
    ColumnSchema columns[] = {
        column_create("id", VALUE_INTEGER),
        column_create("name", VALUE_STRING)
    };
    TableSchema* schema = tableschema_create("users", columns, 2);
    memory_storage_create_table(storage, "users", schema);
    
    Value values1[] = { value_integer(1), value_string("Alice") };
    Value values2[] = { value_integer(2), value_string("Bob") };
    
    execute_insert_simple(storage, "users", values1, 2);
    execute_insert_simple(storage, "users", values2, 2);
    
    QueryResult* result = execute_select_all(storage, "users");
    assert(result != NULL);
    assert(result->count == 2);
    assert(result->ghost_count == 0);
    
    queryresult_destroy(result);
    
    Query* query = query_create(QUERY_SELECT, "users");
    query->include_ghosts = false; 
    
    result = execute_query(storage, query);
    assert(result != NULL);
    assert(result->count == 2);
    
    queryresult_destroy(result);
    query_destroy(query);
    memory_storage_destroy(storage);
    
    for (int i = 0; i < 2; i++) {
        free((char*)columns[i].name);
    }
    value_destroy(&values1[1]);
    value_destroy(&values2[1]);
    
    printf("Basic SELECT tests passed\n");
}

void test_ghost_queries() {
    printf("Testing ghost-aware queries...\n");
    
    MemoryStorage* storage = memory_storage_create();
    ColumnSchema columns[] = {
        column_create("id", VALUE_INTEGER),
        column_create("data", VALUE_STRING)
    };
    TableSchema* schema = tableschema_create("test", columns, 2);
    memory_storage_create_table(storage, "test", schema);
    
    Value values[] = { value_integer(1), value_string("living") };
    Value values2[] = { value_integer(2), value_string("ghost") };
    Value values3[] = { value_integer(3), value_string("living2") };
    
    execute_insert_simple(storage, "test", values, 2);
    execute_insert_simple(storage, "test", values2, 2);
    execute_insert_simple(storage, "test", values3, 2);
    
    execute_delete_simple(storage, "test", 2);
    
    Query* no_ghosts = query_create(QUERY_SELECT, "test");
    no_ghosts->include_ghosts = false;
    
    QueryResult* result = execute_query(storage, no_ghosts);
    assert(result != NULL);
    assert(result->count == 2); 
    assert(result->ghost_count == 1);
    
    queryresult_destroy(result);
    query_destroy(no_ghosts);
    
    Query* with_ghosts = query_create(QUERY_SELECT, "test");
    with_ghosts->include_ghosts = true;
    
    result = execute_query(storage, with_ghosts);
    assert(result != NULL);
    assert(result->count == 3); 
    assert(result->ghost_count == 1);
    
    queryresult_destroy(result);
    query_destroy(with_ghosts);
    
    memory_storage_destroy(storage);
    
    for (int i = 0; i < 2; i++) {
        free((char*)columns[i].name);
    }
    value_destroy(&values[1]);
    value_destroy(&values2[1]);
    value_destroy(&values3[1]);
    
    printf("Ghost-aware query tests passed\n");
}

void test_ghost_threshold() {
    printf("Testing ghost strength threshold...\n");
    
    MemoryStorage* storage = memory_storage_create();
    ColumnSchema columns[] = { column_create("id", VALUE_INTEGER) };
    TableSchema* schema = tableschema_create("threshold", columns, 1);
    memory_storage_create_table(storage, "threshold", schema);
    
    Value values[] = { value_integer(1) };
    execute_insert_simple(storage, "threshold", values, 1);
    execute_delete_simple(storage, "threshold", 1);
    
    DataRecord* ghost = memory_table_get(memory_storage_get_table(storage, "threshold"), 1);
    datarecord_decay_ghost(ghost, 0.7f); 
    
    Query* high_threshold = query_create(QUERY_SELECT, "threshold");
    high_threshold->include_ghosts = true;
    high_threshold->ghost_threshold = 0.5f; 
    
    QueryResult* result = execute_query(storage, high_threshold);
    assert(result != NULL);
    assert(result->count == 0); 
    
    queryresult_destroy(result);
    query_destroy(high_threshold);
    
    Query* low_threshold = query_create(QUERY_SELECT, "threshold");
    low_threshold->include_ghosts = true;
    low_threshold->ghost_threshold = 0.1f; 
    
    result = execute_query(storage, low_threshold);
    assert(result != NULL);
    assert(result->count == 1); 
    
    queryresult_destroy(result);
    query_destroy(low_threshold);
    
    memory_storage_destroy(storage);
    free((char*)columns[0].name);
    
    printf("Ghost threshold tests passed\n");
}

int main() {
    printf("=== Shade Database Query Tests ===\n\n");
    
    test_query_creation();
    test_basic_select();
    test_ghost_queries();
    test_ghost_threshold();
    
    printf("\nAll query tests passed!\n");
    return 0;
}
