#include "sql_executor_strategies.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============== STRATEGY 1: SEQUENTIAL EXECUTOR ==============
// Simpler linear scan through all records.

typedef struct {
    Database *db;
    int comparisons_made;
    int records_scanned;
} SequentialExecutorData;

int sequential_execute(void *self, ParserResult *parse_result, Person *results[], int max_results)
{
    QueryExecutor *executor = (QueryExecutor *)self;
    SequentialExecutorData *data = (SequentialExecutorData *)executor->strategy_data;
    Database *db = data->db;

    data->comparisons_made = 0;
    data->records_scanned = 0;

    int result_count = 0;

    printf("[SEQUENTIAL] Scanning all %d records...\n", db->count);

    // Linear scan through all records
    for (int i = 0; i < db->count && result_count < max_results; i++)
    {
        data->records_scanned++;

        // Simple WHERE filter (age > 25 as example)
        if (db->records[i].age > 25)
        {
            data->comparisons_made++;
            results[result_count++] = &db->records[i];
        }
    }

    printf("[SEQUENTIAL] Scanned %d records, made %d comparisons, found %d matches\n\n", data->records_scanned, data->comparisons_made, result_count);

    return result_count;
}

void sequential_display_stats(void *self)
{
    QueryExecutor *executor = (QueryExecutor *)self;
    SequentialExecutorData *data = (SequentialExecutorData *)executor->strategy_data;

    printf("  ├─ Strategy: Sequential Scan\n");
    printf("  ├─ Records Scanned: %d\n", data->records_scanned);
    printf("  ├─ Comparisons Made: %d\n", data->comparisons_made);
    printf("  └─ Complexity: O(n)\n");
}

void sequential_free(void *self)
{
    QueryExecutor *executor = (QueryExecutor *)self;
    free(executor->strategy_data);
    free(executor);
}

QueryExecutor* executor_sequential_create(Database *db)
{
    QueryExecutor *executor = malloc(sizeof(QueryExecutor));
    SequentialExecutorData *data = malloc(sizeof(SequentialExecutorData));

    data->db = db;
    data->comparisons_made = 0;
    data->records_scanned = 0;

    executor->execute = sequential_execute;
    executor->display_stats = sequential_display_stats;
    executor->free_executor = sequential_free;
    executor->name = "Sequential";
    executor->strategy_data = data;

    return executor;
}

// ============== STRATEGY 2: INDEXED EXECUTOR ==============
// Use hash index for O(1) lookups (for ID searches)

typedef struct {
    Database *db;
    int hash_lookups;
    int cache_hits;
} IndexedExecutorData;

int indexed_execute(void *self, ParserResult *parse_result, Person *results[], int max_results)
{
    QueryExecutor *executor = (QueryExecutor *)self;
    IndexedExecutorData *data = (IndexedExecutorData *)executor->strategy_data;
    Database *db = data->db;

    data->hash_lookups = 0;
    data->cache_hits = 0;

    int result_count = 0;

    printf("[INDEXED] Using hash index for lookups...\n");

    // Try to lookup specific IDs (1, 5, 10, 15, 20)
    int target_ids[] = {1, 5, 10, 15, 20};

    for (int i = 0; i < 5 && result_count < max_results; i++)
    {
        data->hash_lookups++;
        Person *found = db_get_by_id(db, target_ids[i]);

        if (found != NULL)
        {
            data->cache_hits++;
            results[result_count++] = found;
        }
    }

    printf("[INDEXED] Made %d hash lookups, %d cache hits, found %d matches\n\n", data->hash_lookups, data->cache_hits, result_count);

    return result_count;
}

void indexed_display_stats(void *self)
{
    QueryExecutor *executor = (QueryExecutor *)self;
    IndexedExecutorData *data = (IndexedExecutorData *)executor->strategy_data;

    printf("  ├─ Strategy: Indexed (Hash)\n");
    printf("  ├─ Hash Lookups: %d\n", data->hash_lookups);
    printf("  ├─ Successful Lookups: %d\n", data->cache_hits);
    printf("  └─ Complexity: O(1) per lookup\n");
}

void indexed_free(void *self)
{
    QueryExecutor *executor = (QueryExecutor *)self;
    free(executor->strategy_data);
    free(executor);
}

QueryExecutor* executor_indexed_create(Database *db)
{
    QueryExecutor *executor = malloc(sizeof(QueryExecutor));
    IndexedExecutorData *data = malloc(sizeof(IndexedExecutorData));

    data->db = db;
    data->hash_lookups = 0;
    data->cache_hits = 0;

    executor->execute = indexed_execute;
    executor->display_stats = indexed_display_stats;
    executor->free_executor = indexed_free;
    executor->name = "Indexed";
    executor->strategy_data = data;

    return executor;
}

// ============== STRATEGY 3: CACHED EXECUTOR ==============
// Check query cache first, fallback to sequential scan

typedef struct {
    Database *db;
    int cache_checks;
    int cache_hits;
    int fallback_scans;
} CachedExecutorData;

int cached_execute(void *self, ParserResult *parse_result, Person *results[], int max_results)
{
    QueryExecutor *executor = (QueryExecutor *)self;
    CachedExecutorData *data = (CachedExecutorData *)executor->strategy_data;
    Database *db = data->db;

    data->cache_checks++;

    printf("[CACHED] Checking query cache...\n");

    // Check cache key
    char cache_key[256];
    sprintf(cache_key, "age_gt_25");

    CachedQuery *cached = cache_lookup(db->query_cache, cache_key);

    if (cached != NULL)
    {
        data->cache_hits++;
        printf("[CACHED] Cache HIT! Retrieved %d results from cache\n\n", cached->result_count);

        // Copy results from cache
        int copy_count = (cached->result_count < max_results) ? cached->result_count : max_results;
        memcpy(results, cached->results, copy_count * sizeof(Person *));
        return copy_count;
    }
    else
    {
        data->fallback_scans++;
        printf("[CACHED] Cache MISS! Falling back to sequential scan...\n\n");

        // Fallback to sequential scan
        int result_count = 0;
        for (int i = 0; i < db->count && result_count < max_results; i++)
        {
            if (db->records[i].age > 25)
            {
                results[result_count++] = &db->records[i];
            }
        }

        // Cache the results
        cache_insert(db->query_cache, cache_key, results, result_count);
        printf("[CACHED] Cached %d results for future queries\n\n", result_count);

        return result_count;
    }
}

void cached_display_stats(void *self)
{
    QueryExecutor *executor = (QueryExecutor *)self;
    CachedExecutorData *data = (CachedExecutorData *)executor->strategy_data;

    printf("  ├─ Strategy: Cache-Aware\n");
    printf("  ├─ Cache Checks: %d\n", data->cache_checks);
    printf("  ├─ Cache Hits: %d\n", data->cache_hits);
    printf("  ├─ Fallback Scans: %d\n", data->fallback_scans);

    if (data->cache_checks > 0)
    {
        double hit_rate = (double)data->cache_hits / data->cache_checks * 100;
        printf("  └─ Hit Rate: %.1f%%\n", hit_rate);
    }
}

void cached_free(void *self)
{
    QueryExecutor *executor = (QueryExecutor *)self;
    free(executor->strategy_data);
    free(executor);
}

QueryExecutor* executor_cached_create(Database *db)
{
    QueryExecutor *executor = malloc(sizeof(QueryExecutor));
    CachedExecutorData *data = malloc(sizeof(CachedExecutorData));

    data->db = db;
    data->cache_checks = 0;
    data->cache_hits = 0;
    data->fallback_scans = 0;

    executor->execute = cached_execute;
    executor->display_stats = cached_display_stats;
    executor->free_executor = cached_free;
    executor->name = "Cached";
    executor->strategy_data = data;

    return executor;
}

// ============== GENERIC EXECUTOR INVOKER ==============
// Demonstrates polymorphism - same code works with any executor!

int executor_run(QueryExecutor *executor, ParserResult *parse_result, Person *results[], int max_results)
{
    if (executor == NULL)
    {
        printf("Error: Executor is NULL\n");
        return 0;
    }

    printf("═══════════════════════════════════════════════════════\n");
    printf("Executing query with [%s] strategy\n", executor->name);
    printf("═══════════════════════════════════════════════════════\n\n");
    
    // Call the strategy's execute function (polymorphic dispatch)
    int result_count = executor->execute(executor, parse_result, results, max_results);
    
    return result_count;
}