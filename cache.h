#ifndef CACHE_H
#define CACHE_H

#include "person.h"

// ============================================================================
// QUERY CACHE
// ============================================================================
//
// PROBLEM: Running the same filter query multiple times rescans the entire DB
// SOLUTION: Cache results with timestamp, invalidate when DB changes
//
// EXAMPLE:
// User: "Show me everyone age 25-35" (scans all 1000 records)
// User: "Show me everyone age 25-35" (returns cached result instantly)
//
// Python equivalent: @functools.lru_cache(maxsize=128)
// ============================================================================

typedef struct CachedQuery
{
    char query_string[256];       // The query that was cached
    Person **results;             // Array of matching records
    int result_count;             // Number of records matched
    long timestamp;               // When the query was cached
} CachedQuery;

typedef struct QueryCache
{
    CachedQuery *queries;         // Array of cached queries
    int cache_count;              // Number of cached queries
    int cache_size;               // Allocated size
} QueryCache;

// Create empty cache
QueryCache *cache_create(int initial_size);

// Free cache
void cache_free(QueryCache *cache);

// Look up a query in cache (return NULL if not found)
CachedQuery *cache_lookup(QueryCache *cache, const char *query_string);

// Store query results in cache
void cache_insert(QueryCache *cache, const char *query_string, Person **results, int result_count);

// Clear all cache (call when DB changes)
void cache_invalidate(QueryCache *cache);

// Show cache statistics
void cache_display_stats(QueryCache *cache);

#endif // CACHE_H