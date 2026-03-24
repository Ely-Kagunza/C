#ifndef SQL_EXECUTOR_STRATEGIES
#define SQL_EXECUTOR_STRATEGIES

#include "database.h"
#include "sql_parser.h"

// ============== VTABLE INTERFACE ==============
// All executors implement this interface.
typedef struct QueryExecutor {
    // Function pointers (the vtable)
    int (*execute)(void *self, ParserResult *parse_result, Person *results[], int max_results);
    void (*display_stats)(void *self);
    void (*free_executor)(void *self);

    // Metadata
    const char *name;
    void *strategy_data;      // Points to strategy-specific data
} QueryExecutor;

// ============== STRATEGY IMPLEMENTATIONS ==============

// Strategy 1: Sequential Scan - iterate all records
QueryExecutor* executor_sequential_create(Database *db);

// Strategy 2: Index Scan - use hash index for 0(1) lookup
QueryExecutor* executor_indexed_create(Database *db);

// Strategy 3: Cached - check cache first, fallback to sequential
QueryExecutor* executor_cached_create(Database *db);

// ============== HELPER FUNCTIONS ==============
// Generic executor invoker (demonstrates polymorphism)
int executor_run(QueryExecutor *executor, ParserResult *parse_result, Person *results[], int max_results);

#endif // SQL_EXECUTOR_STRATEGIES