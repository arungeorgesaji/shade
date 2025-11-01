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

void test_type_safe_value_getters() {
    printf("Testing type-safe value getters...\n");
    
    ShadeDB* db = shade_db_create();
    assert(db != NULL);
    
    const char* column_names[] = {"id", "name", "score", "active"};
    const char* column_types[] = {"INT", "STRING", "FLOAT", "BOOL"};
    shade_create_table(db, "test_data", column_names, column_types, 4);
    
    int64_t id = 1;
    const char* name = "Test User";
    double score = 95.5;
    bool active = true;
    
    const void* values[] = {&id, name, &score, &active};
    shade_insert(db, "test_data", values, 4);
    
    ShadeQueryResult* result = shade_select(db, "test_data", false);
    assert(result != NULL);
    assert(shade_result_count(result) == 1);
    
    int64_t out_int;
    bool success = shade_get_int(result, 0, 0, &out_int);
    assert(success == true);
    assert(out_int == 1);
    
    const char* out_string;
    success = shade_get_string(result, 0, 1, &out_string);
    assert(success == true);
    assert(strcmp(out_string, "Test User") == 0);
    
    double out_float;
    success = shade_get_float(result, 0, 2, &out_float);
    assert(success == true);
    assert(out_float == 95.5);
    
    bool out_bool;
    success = shade_get_bool(result, 0, 3, &out_bool);
    assert(success == true);
    assert(out_bool == true);
    
    success = shade_get_int(result, 0, 999, &out_int);
    assert(success == false);
    
    success = shade_get_int(result, 999, 0, &out_int);
    assert(success == false);
    
    shade_free_result(result);
    shade_db_destroy(db);
    printf("Type-safe value getters tests passed\n");
}

void test_column_information() {
    printf("Testing column information...\n");
    
    ShadeDB* db = shade_db_create();
    assert(db != NULL);
    
    const char* column_names[] = {"user_id", "user_name", "user_age"};
    const char* column_types[] = {"INT", "STRING", "INT"};
    shade_create_table(db, "users", column_names, column_types, 3);
    
    int64_t id = 1;
    const char* name = "Alice";
    int64_t age = 30;
    const void* values[] = {&id, name, &age};
    shade_insert(db, "users", values, 3);
    
    ShadeQueryResult* result = shade_select(db, "users", false);
    assert(result != NULL);
    
    size_t col_count = shade_result_column_count(result);
    assert(col_count == 3);
    
    const char* col0_name = shade_result_column_name(result, 0);
    const char* col1_name = shade_result_column_name(result, 1);
    const char* col2_name = shade_result_column_name(result, 2);
    assert(strcmp(col0_name, "user_id") == 0);
    assert(strcmp(col1_name, "user_name") == 0);
    assert(strcmp(col2_name, "user_age") == 0);
    
    const char* col0_type = shade_result_column_type(result, 0);
    const char* col1_type = shade_result_column_type(result, 1);
    const char* col2_type = shade_result_column_type(result, 2);
    assert(strcmp(col0_type, "INT") == 0);
    assert(strcmp(col1_type, "STRING") == 0);
    assert(strcmp(col2_type, "INT") == 0);
    
    const char* invalid_name = shade_result_column_name(result, 999);
    assert(invalid_name == NULL);
    
    const char* invalid_type = shade_result_column_type(result, 999);
    assert(invalid_type == NULL);
    
    shade_free_result(result);
    shade_db_destroy(db);
    printf("Column information tests passed\n");
}

void test_ghost_stats_accessors() {
    printf("Testing ghost stats accessors...\n");
    
    ShadeDB* db = shade_db_create();
    assert(db != NULL);
    
    const char* cols[] = {"id", "data"};
    const char* types[] = {"INT", "STRING"};
    
    shade_create_table(db, "table1", cols, types, 2);
    shade_create_table(db, "table2", cols, types, 2);
    
    int64_t id1 = 1; const char* data1 = "record1";
    int64_t id2 = 2; const char* data2 = "record2";
    
    const void* values1[] = {&id1, data1};
    const void* values2[] = {&id2, data2};
    
    shade_insert(db, "table1", values1, 2);
    shade_insert(db, "table1", values2, 2);
    shade_insert(db, "table2", values1, 2);
    
    shade_delete(db, "table1", 2);
    shade_delete(db, "table2", 1);
    
    ShadeQueryResult* stats = shade_get_ghost_stats(db);
    assert(stats != NULL);
    
    size_t total_living, total_ghosts, total_exorcised;
    float overall_ratio, overall_avg_strength;
    
    bool success = shade_ghost_stats_get_overall(stats, 
        &total_living, &total_ghosts, &total_exorcised, 
        &overall_ratio, &overall_avg_strength);
    
    assert(success == true);
    assert(total_living >= 1);
    assert(total_ghosts >= 2);
    
    size_t table_count = shade_ghost_stats_table_count(stats);
    assert(table_count >= 2);
    
    for (size_t i = 0; i < table_count; i++) {
        const char* table_name = shade_ghost_stats_table_name(stats, i);
        assert(table_name != NULL);
        
        size_t living, ghosts, exorcised;
        float ratio, avg_strength;
        
        success = shade_ghost_stats_get_table_stats(stats, i,
            &living, &ghosts, &exorcised, &ratio, &avg_strength);
        assert(success == true);
        
        if (ghosts > 0) {
            size_t strongest_id, weakest_id;
            float strongest_str, weakest_str;
            
            success = shade_ghost_stats_get_strength_info(stats, i,
                &strongest_id, &strongest_str, &weakest_id, &weakest_str);
            assert(success == true);
            assert(strongest_str >= 0.0f && strongest_str <= 1.0f);
            assert(weakest_str >= 0.0f && weakest_str <= 1.0f);
        }
    }
    
    const char* invalid_name = shade_ghost_stats_table_name(stats, 999);
    assert(invalid_name == NULL);
    
    size_t dummy;
    success = shade_ghost_stats_get_table_stats(stats, 999, &dummy, &dummy, &dummy, NULL, NULL);
    assert(success == false);
    
    shade_free_result(stats);
    shade_db_destroy(db);
    printf("Ghost stats accessors tests passed\n");
}

void test_memory_management() {
    printf("Testing memory management...\n");
    
    for (int i = 0; i < 10; i++) {
        ShadeDB* db = shade_db_create();
        assert(db != NULL);
        
        const char* cols[] = {"id", "name"};
        const char* types[] = {"INT", "STRING"};
        
        shade_create_table(db, "temp", cols, types, 2);
        
        for (int j = 0; j < 5; j++) {
            int64_t id = j;
            const char* name = "test";
            const void* values[] = {&id, name};
            
            shade_insert(db, "temp", values, 2);
            
            ShadeQueryResult* result = shade_select(db, "temp", false);
            assert(result != NULL);
            shade_free_result(result);
        }
        
        ShadeQueryResult* stats = shade_get_ghost_stats(db);
        if (stats) shade_free_result(stats);
        
        shade_db_destroy(db);
    }
    printf("Memory management tests passed\n");
}

void test_edge_cases() {
    printf("Testing edge cases...\n");
    
    ShadeDB* db = shade_db_create();
    assert(db != NULL);
    
    ShadeQueryResult* stats = shade_get_ghost_stats(db);
    assert(stats != NULL);
    
    size_t table_count = shade_ghost_stats_table_count(stats);
    assert(table_count == 0);
    
    size_t total_living, total_ghosts, total_exorcised;
    bool success = shade_ghost_stats_get_overall(stats, 
        &total_living, &total_ghosts, &total_exorcised, NULL, NULL);
    assert(success == true);
    assert(total_living == 0 && total_ghosts == 0 && total_exorcised == 0);
    
    shade_free_result(stats);
    
    const char* cols[] = {"id"};
    const char* types[] = {"INT"};
    shade_create_table(db, "empty", cols, types, 1);
    
    stats = shade_get_ghost_stats(db);
    assert(stats != NULL);
    
    table_count = shade_ghost_stats_table_count(stats);
    assert(table_count == 1);
    
    size_t living, ghosts, exorcised;
    success = shade_ghost_stats_get_table_stats(stats, 0,
        &living, &ghosts, &exorcised, NULL, NULL);
    assert(success == true);
    assert(living == 0 && ghosts == 0 && exorcised == 0);
    
    shade_free_result(stats);
    shade_db_destroy(db);
    printf("Edge cases tests passed\n");
}

int main() {
    printf("=== Shade Embedded API Tests ===\n\n");
    
    test_db_creation_and_destruction();
    test_table_operations();
    test_data_operations();
    test_ghost_operations();
    test_error_handling();
    test_query_result_handling();
    test_type_safe_value_getters();
    test_column_information();
    test_ghost_stats_accessors();
    test_memory_management();
    test_edge_cases();
    
    printf("\nAll embedded API tests passed!\n");
    return 0;
}
