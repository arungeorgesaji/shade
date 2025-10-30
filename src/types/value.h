#ifndef SHADE_VALUE_H
#define SHADE_VALUE_H

#include <stdint.h>
#include <stdbool.h>
#include "../util/string_utils.h"
#include <stdlib.h>
#include <stdio.h>

typedef enum {
    VALUE_INTEGER,
    VALUE_FLOAT,
    VALUE_BOOLEAN,
    VALUE_STRING,
    VALUE_NULL
} ValueType;

typedef struct {
    ValueType type;
    union {
        int64_t integer;
        double float_val;
        bool boolean;
        char* string;  
    } data;
} Value;

Value value_integer(int64_t val);
Value value_float(double val);
Value value_boolean(bool val);
Value value_string(const char* val);  
Value value_null(void);

bool value_equals(const Value* a, const Value* b);
void value_destroy(Value* value);
Value value_clone(const Value* value);

const char* value_type_to_string(ValueType type);

#endif
