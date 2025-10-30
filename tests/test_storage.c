#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../src/types/data.h"
#include "../src/types/value.h"
#include "../src/types/schema.h"
#include "../src/storage/memory.h"

void test_schema_creation() {
    printf("Testing schema creation...\n");
    
    ColumnSchema columns[] = {
        column_create("id", VALUE_INTEGER),
        column_create("name", VALUE_STRING),
        column_create("age", VALUE_INTEGER),
        column_create("active", VALUE_BOOLEAN)
    };
    
    TableSchema* schema = tableschema_create("users", columns, 4);
    assert(schema != NULL);
    assert(strcmp(schema->name, "users") == 0);
    assert(schema->column_count == 4);
    
    assert(strcmp(schema->columns[0].name, "id") == 0);
    assert(schema->columns[0].type == VALUE_INTEGER);
    
    assert(strcmp(schema->columns[1].name, "name") == 0);
    assert(schema->columns[1].type == VALUE_STRING);
    
    assert(strcmp(schema->columns[2].name, "age") == 0);
    assert(schema->columns[2].type == VALUE_INTEGER);
    
    assert(strcmp(schema->columns[3].name, "active") == 0);
    assert(schema->columns[3].type == VALUE_BOOLEAN);
    
    tableschema_destroy(schema);
    
    for (int i = 0; i < 4; i++) {
        free((char*)columns[i].name);
    }
    
    printf("Schema creation tests passed\n");
}

void test_storage_creation() {
    printf("Testing storage creation...\n");
    
    MemoryStorage* storage = memory_storage_create();
    assert(storage != NULL);
    assert(storage->table_count == 0);
    assert(storage->capacity > 0);
    
    memory_storage_destroy(storage);
    printf("Storage creation tests passed\n");
}

void test_table_operations() {
    printf("Testing table operations...\n");
    
    MemoryStorage* storage = memory_storage_create();
    assert(storage != NULL);
    
    ColumnSchema columns[] = {
        column_create("id", VALUE_INTEGER),
        column_create("name", VALUE_STRING)
    };
    
    TableSchema* schema = tableschema_create("test_table", columns, 2);
    assert(schema != NULL);
    
    MemoryTable* table = memory_storage_create_table(storage, "users", schema);
    assert(table != NULL);
    assert(storage->table_count == 1);
    assert(strcmp(table->name, "users") == 0);
    assert(table->record_count == 0);
    assert(table->next_id == 1);
    
    MemoryTable* duplicate = memory_storage_create_table(storage, "users", schema);
    assert(duplicate == NULL);
    assert(storage->table_count == 1);
    
    MemoryTable* found = memory_storage_get_table(storage, "users");
    assert(found == table);
    
    MemoryTable* not_found = memory_storage_get_table(storage, "nonexistent");
    assert(not_found == NULL);
    
    memory_storage_destroy(storage);
    
    for (int i = 0; i < 2; i++) {
        free((char*)columns[i].name);
    }
    
    printf("Table operations tests passed\n");
}

void test_data_operations() {
    printf("Testing data operations...\n");
    
    MemoryStorage* storage = memory_storage_create();
    assert(storage != NULL);
    
    ColumnSchema columns[] = {
        column_create("id", VALUE_INTEGER),
        column_create("name", VALUE_STRING),
        column_create("age", VALUE_INTEGER)
    };
    
    TableSchema* schema = tableschema_create("users", columns, 3);
    MemoryTable* table = memory_storage_create_table(storage, "users", schema);
    assert(table != NULL);
    
    Value user1_values[] = {
        value_integer(1),      
        value_string("Alice"), 
        value_integer(30)      
    };
    
    uint64_t id1 = memory_table_insert(table, user1_values);
    assert(id1 == 1);
    assert(table->record_count == 1);
    assert(table->next_id == 2);
    
    Value user2_values[] = {
        value_integer(2),
        value_string("Bob"),
        value_integer(25)
    };
    
    uint64_t id2 = memory_table_insert(table, user2_values);
    assert(id2 == 2);
    assert(table->record_count == 2);
    
    DataRecord* record1 = memory_table_get(table, 1);
    assert(record1 != NULL);
    assert(record1->id == 1);
    assert(record1->state == DATA_STATE_LIVING);
    assert(value_equals(&record1->values[1], &user1_values[1])); 
    
    DataRecord* record_nonexistent = memory_table_get(table, 999);
    assert(record_nonexistent == NULL);
    
    size_t result_count = 0;
    DataRecord** results = memory_table_scan(table, &result_count);
    assert(results != NULL);
    assert(result_count == 2);
    
    bool found1 = false, found2 = false;
    for (size_t i = 0; i < result_count; i++) {
        if (results[i]->id == 1) found1 = true;
        if (results[i]->id == 2) found2 = true;
    }
    assert(found1 && found2);
    free(results);
    
    memory_storage_destroy(storage);
    
    for (int i = 0; i < 3; i++) {
        free((char*)columns[i].name);
    }
    
    value_destroy(&user1_values[1]); 
    value_destroy(&user2_values[1]); 
    
    printf("Data operations tests passed\n");
}

void test_ghost_operations() {
    printf("Testing ghost operations...\n");
    
    MemoryStorage* storage = memory_storage_create();
    assert(storage != NULL);
    
    ColumnSchema columns[] = {
        column_create("id", VALUE_INTEGER),
        column_create("data", VALUE_STRING)
    };
    
    TableSchema* schema = tableschema_create("test", columns, 2);
    MemoryTable* table = memory_storage_create_table(storage, "test", schema);
    
    Value values1[] = { value_integer(1), value_string("record1") };
    Value values2[] = { value_integer(2), value_string("record2") };
    Value values3[] = { value_integer(3), value_string("record3") };
    
    memory_table_insert(table, values1);
    memory_table_insert(table, values2);
    memory_table_insert(table, values3);
    assert(table->record_count == 3);
    
    bool delete_success = memory_table_delete(table, 2, 1234567890);
    assert(delete_success);
    
    DataRecord* ghost_record = memory_table_get(table, 2);
    assert(ghost_record != NULL);
    assert(ghost_record->state == DATA_STATE_GHOST);
    assert(ghost_record->deleted_at == 1234567890);
    assert(ghost_record->ghost_strength == 1.0f);
    
    size_t result_count = 0;
    DataRecord** results = memory_table_scan(table, &result_count);
    assert(results != NULL);
    assert(result_count == 3); 
    
    free(results);
    
    datarecord_decay_ghost(ghost_record, 0.6f);
    assert(fabsf(ghost_record->ghost_strength - 0.4f) < 0.0001f);
    assert(ghost_record->state == DATA_STATE_GHOST);
    
    datarecord_decay_ghost(ghost_record, 0.5f);
    assert(ghost_record->state == DATA_STATE_EXORCISED);
    assert(!datarecord_is_queryable(ghost_record));
    
    results = memory_table_scan(table, &result_count);
    assert(result_count == 2); 
    
    free(results);
    memory_storage_destroy(storage);
    
    for (int i = 0; i < 2; i++) {
        free((char*)columns[i].name);
    }
    value_destroy(&values1[1]);
    value_destroy(&values2[1]);
    value_destroy(&values3[1]);
    
    printf("Ghost operations tests passed\n");
}

void test_storage_scalability() {
    printf("Testing storage scalability...\n");
    
    MemoryStorage* storage = memory_storage_create();
    ColumnSchema columns[] = { column_create("id", VALUE_INTEGER) };
    TableSchema* schema = tableschema_create("perf", columns, 1);
    MemoryTable* table = memory_storage_create_table(storage, "perf", schema);
    
    const size_t NUM_RECORDS = 100;
    for (size_t i = 0; i < NUM_RECORDS; i++) {
        Value values[] = { value_integer(i) };
        uint64_t id = memory_table_insert(table, values);
        assert(id == i + 1);
    }
    
    assert(table->record_count == NUM_RECORDS);
    assert(table->capacity >= NUM_RECORDS);
    
    for (size_t i = 0; i < NUM_RECORDS; i++) {
        DataRecord* record = memory_table_get(table, i + 1);
        assert(record != NULL);
        assert(record->id == i + 1);
    }
    
    memory_storage_destroy(storage);
    free((char*)columns[0].name);
    
    printf("Storage scalability tests passed\n");
}

int main() {
    printf("=== Shade Database Storage Tests ===\n\n");
    
    test_schema_creation();
    test_storage_creation();
    test_table_operations();
    test_data_operations();
    test_ghost_operations();
    test_storage_scalability();
    
    printf("\nAll storage tests passed!\n");
    return 0;
}
