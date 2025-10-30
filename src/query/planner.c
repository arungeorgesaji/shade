#include "planner.h"

Query* plan_query(QueryType type, const char* table_name) {
    return query_create(type, table_name);
}
