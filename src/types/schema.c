#include "schema.h"

TableSchema* tableschema_create(const char* name, const ColumnSchema* columns, size_t column_count) {
    if (!name || !columns || column_count == 0) return NULL;
    
    TableSchema* schema = malloc(sizeof(TableSchema));
    if (!schema) return NULL;
    
    schema->name = string_duplicate(name);
    schema->columns = malloc(sizeof(ColumnSchema) * column_count);
    schema->column_count = column_count;
    
    if (!schema->name || !schema->columns) {
        free(schema->name);
        free(schema->columns);
        free(schema);
        return NULL;
    }
    
    for (size_t i = 0; i < column_count; i++) {
        schema->columns[i].name = string_duplicate(columns[i].name);
        schema->columns[i].type = columns[i].type;
        
        if (!schema->columns[i].name) {
            for (size_t j = 0; j < i; j++) {
                free(schema->columns[j].name);
            }
            free(schema->columns);
            free(schema->name);
            free(schema);
            return NULL;
        }
    }
    
    return schema;
}

void tableschema_destroy(TableSchema* schema) {
    if (!schema) return;
    
    free(schema->name);
    for (size_t i = 0; i < schema->column_count; i++) {
        free(schema->columns[i].name);
    }
    free(schema->columns);
    free(schema);
}

ColumnSchema column_create(const char* name, ValueType type) {
    ColumnSchema column = {
        .name = name ? string_duplicate(name) : NULL,
        .type = type
    };
    return column;
}
