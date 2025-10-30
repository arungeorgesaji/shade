#ifndef SHADE_SCHEMA_H
#define SHADE_SCHEMA_H

#include "value.h"
#include <stdlib.h>
#include "../util/string_utils.h" 

typedef struct {
    char* name;
    ValueType type;
} ColumnSchema;

typedef struct {
    char* name;
    ColumnSchema* columns;
    size_t column_count;
} TableSchema;

TableSchema* tableschema_create(const char* name, const ColumnSchema* columns, size_t column_count);
void tableschema_destroy(TableSchema* schema);

ColumnSchema column_create(const char* name, ValueType type);

#endif
