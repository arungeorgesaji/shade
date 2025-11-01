#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/shade.h"

void test_db_creation_and_destruction() {
    printf("Testing database creation and destruction...\n");
    
    ShadeDB* db = shade_db_create();
    assert(db != NULL);
    assert(shade_get_error() == NULL);
    
    shade_db_destroy(db);
    
    printf("Database creation/destruction tests passed\n");
}

void test_table_operations() {
    printf("Testing table operations...\n");
    
    ShadeDB* db = shade_db_create();
    assert(db != NULL);
    
    const char* column_names[] = {"id", "name", "age", "active"};
    const char* column_types[] = {"INT", "STRING", "INT", "BOOL"};
    size_t column_count = 4;
    
    ShadeTable* table = shade_create_table(db, "users", column_names, column_types, column_count);
    assert(table != NULL);
    assert(shade_get_error() == NULL);
    
    ShadeTable* duplicate = shade_create_table(db, "users", column_names, column_types, column_count);
    assert(duplicate == NULL);
    assert(shade_get_error() != NULL);
    shade_clear_error();
    
    bool drop_success = shade_drop_table(db, "users");
    assert(drop_success == true);
    assert(shade_get_error() == NULL);
    
    drop_success = shade_drop_table(db, "nonexistent");
    assert(drop_success == false);
    assert(shade_get_error() != NULL);
    shade_clear_error();
    
    shade_db_destroy(db);
    printf("Table operations tests passed\n");
}

void test_data_operations() {
    printf("Testing data operations...\n");
    
    ShadeDB* db = shade_db_create();
    assert(db != NULL);
    
    const char* column_names[] = {"id", "name", "age", "active"};
    const char* column_types[] = {"INT", "STRING", "INT", "BOOL"};
    ShadeTable* table = shade_create_table(db, "users", column_names, column_types, 4);
    assert(table != NULL);
    
    int64_t id1 = 1;
    const char* name1 = "Alice";
    int64_t age1 = 30;
    bool active1 = true;
    
    const void* values1[] = {&id1, name1, &age1, &active1};
    uint64_t inserted_id = shade_insert(db, "users", values1, 4);
    assert(inserted_id == 1);
    assert(shade_get_error() == NULL);
    
    int64_t id2 = 2;
    const char* name2 = "Bob";
    int64_t age2 = 25;
    bool active2 = false;
    
    const void* values2[] = {&id2, name2, &age2, &active2};
    inserted_id = shade_insert(db, "users", values2, 4);
    assert(inserted_id == 2);
    assert(shade_get_error() == NULL);
    
    ShadeQueryResult* result = shade_select(db, "users", false);
    assert(result != NULL);
    assert(shade_get_error() == NULL);
    
    size_t count = shade_result_count(result);
    assert(count == 2);
    
    shade_free_result(result);
    
    result = shade_select(db, "nonexistent", false);
    assert(result == NULL);
    assert(shade_get_error() != NULL);
    shade_clear_error();
    
    shade_db_destroy(db);
    printf("Data operations tests passed\n");
}

void test_ghost_operations() {
    printf("Testing ghost operations...\n");
    
    ShadeDB* db = shade_db_create();
    assert(db != NULL);
    
    const char* column_names[] = {"id", "data"};
    const char* column_types[] = {"INT", "STRING"};
    shade_create_table(db, "test", column_names, column_types, 2);
    
    int64_t id1 = 1;
    const char* data1 = "record1";
    const void* values1[] = {&id1, data1};
    shade_insert(db, "test", values1, 2);
    
    int64_t id2 = 2;
    const char* data2 = "record2";
    const void* values2[] = {&id2, data2};
    shade_insert(db, "test", values2, 2);
    
    ShadeQueryResult* result = shade_select(db, "test", false);
    assert(shade_result_count(result) == 2);
    shade_free_result(result);
    
    bool delete_success = shade_delete(db, "test", 2);
    assert(delete_success == true);
    assert(shade_get_error() == NULL);
    
    result = shade_select(db, "test", false);
    assert(shade_result_count(result) == 1);
    shade_free_result(result);
    
    result = shade_select(db, "test", true);
    assert(shade_result_count(result) == 2);
    shade_free_result(result);
    
    bool resurrect_success = shade_resurrect(db, "test", 2);
    assert(resurrect_success == true);
    assert(shade_get_error() == NULL);
    
    result = shade_select(db, "test", false);
    assert(shade_result_count(result) == 2);
    shade_free_result(result);
    
    bool decay_success = shade_decay_ghosts(db, 0.5f);
    assert(decay_success == true);
    assert(shade_get_error() == NULL);
    
    ShadeQueryResult* stats = shade_get_ghost_stats(db);
    assert(stats != NULL);
    shade_free_result(stats);
    
    shade_db_destroy(db);
    printf("Ghost operations tests passed\n");
}

void test_error_handling() {
    printf("Testing error handling...\n");
    
    ShadeDB* db = shade_db_create();
    assert(db != NULL);
    
    ShadeTable* table = shade_create_table(NULL, "test", NULL, NULL, 0);
    assert(table == NULL);
    assert(shade_get_error() != NULL);
    shade_clear_error();
    
    uint64_t id = shade_insert(NULL, "test", NULL, 0);
    assert(id == 0);
    assert(shade_get_error() != NULL);
    shade_clear_error();
    
    int64_t test_id = 1;
    const char* test_data = "test";
    const void* test_values[] = {&test_id, test_data};
    id = shade_insert(db, "nonexistent", test_values, 2);
    assert(id == 0);
    assert(shade_get_error() != NULL);
    shade_clear_error();
    
    bool success = shade_delete(db, "nonexistent", 1);
    assert(success == false);
    assert(shade_get_error() != NULL);
    shade_clear_error();
    
    success = shade_resurrect(db, "nonexistent", 1);
    assert(success == false);
    assert(shade_get_error() != NULL);
    shade_clear_error();
    
    shade_db_destroy(db);
    printf("Error handling tests passed\n");
}

void test_query_result_handling() {
    printf("Testing query result handling...\n");
    
    ShadeDB* db = shade_db_create();
    assert(db != NULL);
    
    const char* column_names[] = {"id", "name", "score"};
    const char* column_types[] = {"INT", "STRING", "FLOAT"};
    shade_create_table(db, "scores", column_names, column_types, 3);
    
    int64_t id1 = 1;
    const char* name1 = "Alice";
    double score1 = 95.5;
    
    int64_t id2 = 2;
    const char* name2 = "Bob";
    double score2 = 87.3;
    
    const void* values1[] = {&id1, name1, &score1};
    const void* values2[] = {&id2, name2, &score2};
    
    shade_insert(db, "scores", values1, 3);
    shade_insert(db, "scores", values2, 3);
    
    ShadeQueryResult* result = shade_select(db, "scores", false);
    assert(result != NULL);
    
    size_t count = shade_result_count(result);
    assert(count == 2);
    
    shade_free_result(result);
    
    shade_free_result(NULL);
    
    shade_db_destroy(db);
    printf("Query result handling tests passed\n");
}

int main() {
    printf("=== Shade Embedded API Tests ===\n\n");
    
    test_db_creation_and_destruction();
    test_table_operations();
    test_data_operations();
    test_ghost_operations();
    test_error_handling();
    test_query_result_handling();
    
    printf("\nAll embedded API tests passed!\n");
    return 0;
}
