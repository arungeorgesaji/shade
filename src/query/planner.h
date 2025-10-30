#ifndef SHADE_QUERY_PLANNER_H
#define SHADE_QUERY_PLANNER_H

#include "executor.h"

Query* plan_query(QueryType type, const char* table_name);

#endif
