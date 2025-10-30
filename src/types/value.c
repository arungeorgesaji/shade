#include "value.h"

Value value_integer(int64_t val) {
    return (Value){
        .type = VALUE_INTEGER,
        .data.integer = val
    };
}

Value value_float(double val) {
    return (Value){
        .type = VALUE_FLOAT,
        .data.float_val = val
    };
}

Value value_boolean(bool val) {
    return (Value){
        .type = VALUE_BOOLEAN,
        .data.boolean = val
    };
}

Value value_string(const char* val) {
    Value result = {
        .type = VALUE_STRING,
        .data.string = NULL
    };
    
    if (val != NULL) {
        result.data.string = string_duplicate(val);  
    }
    
    return result;
}

Value value_null(void) {
    return (Value){
        .type = VALUE_NULL
    };
}

bool value_equals(const Value* a, const Value* b) {
    if (a->type != b->type) return false;
    
    switch (a->type) {
        case VALUE_INTEGER:
            return a->data.integer == b->data.integer;
        case VALUE_FLOAT:
            return a->data.float_val == b->data.float_val;
        case VALUE_BOOLEAN:
            return a->data.boolean == b->data.boolean;
        case VALUE_STRING:
            if (a->data.string == NULL && b->data.string == NULL) return true;
            if (a->data.string == NULL || b->data.string == NULL) return false;
            return strcmp(a->data.string, b->data.string) == 0;
        case VALUE_NULL:
            return true;
        default:
            return false;
    }
}

void value_destroy(Value* value) {
    if (value->type == VALUE_STRING && value->data.string != NULL) {
        free(value->data.string);
        value->data.string = NULL;
    }
    value->type = VALUE_NULL;
}

Value value_clone(const Value* value) {
    Value result = *value;
    
    if (value->type == VALUE_STRING && value->data.string != NULL) {
        result.data.string = string_duplicate(value->data.string);
    }
    
    return result;
}

const char* value_type_to_string(ValueType type) {
    switch (type) {
        case VALUE_INTEGER: return "INTEGER";
        case VALUE_FLOAT: return "FLOAT";
        case VALUE_BOOLEAN: return "BOOLEAN";
        case VALUE_STRING: return "STRING";
        case VALUE_NULL: return "NULL";
        default: return "UNKNOWN";
    }
}
