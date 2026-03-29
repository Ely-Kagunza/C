#ifndef QUERY_CACHE_H
#define QUERY_CACHE_H

#include "btree_index.h"

typedef struct CachedRangeQuery {
    char query_key[255];                   // "age:25:35" or "salary:50000:100000"
    Person **results;                      // Array of matching persons
    int result_count;                      // Number of results
    long timestamp;                        // When cached
} CachedRangeQuery;

typedef struct QueryCache {
    CachedRangeQuery *queries;
    int cache_count;
    int cache_size;
} QueryCache;

// Create cache
QueryCache *query_cache_create(int capacity);

// query cache format
char *format_cache_key(char *buffer, int field_type, double min_val, double max_val);

// Free cache
void query_cache_free(QueryCache *cache);

// Lookup query in cache
CachedRangeQuery *query_cache_lookup(QueryCache *cache, const char *query_key);

// Insert results into cache with LRU eviction
void query_cache_insert(QueryCache *cache, const char *query_key, Person **results, int result_count);

// Clear all cache (call when DB modifies)
void query_cache_invalidate(QueryCache *cache);

#endif