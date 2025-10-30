#include <assert.h>
#include <stdio.h>
#include "types/data.h"
#include "types/value.h"

void test_value_creation() {
    Value int_val = value_integer(42);
    Value str_val = value_string("hello");
    Value bool_val = value_boolean(true);
    
    assert(int_val.type == VALUE_INTEGER);
    assert(int_val.data.integer == 42);
    
    assert(str_val.type == VALUE_STRING);
    assert(strcmp(str_val.data.string, "hello") == 0);
    
    assert(bool_val.type == VALUE_BOOLEAN);
    assert(bool_val.data.boolean == true);
    
    value_destroy(&str_val);  
    printf("Value creation tests passed\n");
}

void test_datarecord_lifecycle() {
    Value values[] = {
        value_integer(1),
        value_string("test user")
    };
    
    DataRecord* record = datarecord_create(1, values, 2);
    assert(record != NULL);
    assert(record->state == DATA_STATE_LIVING);
    assert(datarecord_is_queryable(record));
    
    datarecord_mark_ghost(record, 1234567890);
    assert(record->state == DATA_STATE_GHOST);
    assert(record->ghost_strength == 1.0f);
    
    datarecord_decay_ghost(record, 0.3f);
    assert(record->ghost_strength == 0.7f);
    assert(datarecord_is_queryable(record));
    
    datarecord_decay_ghost(record, 0.8f);
    assert(record->state == DATA_STATE_EXORCISED);
    assert(!datarecord_is_queryable(record));
    
    datarecord_destroy(record);
    printf("DataRecord lifecycle tests passed\n");
}

int main() {
    test_value_creation();
    test_datarecord_lifecycle();
    printf("All tests passed!\n");
    return 0;
}
