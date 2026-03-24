#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "cache.h"

// ============================================================================
// CREATE - Allocate cache
// ============================================================================
QueryCache *cache_create(int initial_size)
{
    if (initial_size < 5)
    {
        printf("Cache size must be at least 5\n");
        return NULL;
    }

    QueryCache *cache = malloc(sizeof(QueryCache));
    if (cache == NULL)
    {
        printf("Failed to allocate cache\n");
        return NULL;
    }

    cache->queries = calloc(initial_size, sizeof(CachedQuery));
    if (cache->queries == NULL)
    {
        printf("Failed to allocate cache queries\n");
        free(cache);
        return NULL;
    }

    cache->cache_count = 0;
    cache->cache_size = initial_size;

    printf("Query cache created with capacity %d\n", initial_size);
    return cache;
}

// ============================================================================
// FREE - Deallocate cache
// ============================================================================
void cache_free(QueryCache *cache)
{
    if (cache == NULL)
        return;

    // Free each cached query's results
    for (int i = 0; i < cache->cache_count; i++)
    {
        if (cache->queries[i].results != NULL)
            free(cache->queries[i].results);
    }

    free(cache->queries);
    free(cache);
}

// ============================================================================
// LOOKUP - Find query in cache and update timestamp (LRU touch)
// ============================================================================
//
// KEY CHANGE: Update timestamp on access
// This "touches" the entry to mark it as recently used
// If we never access it again, it becomes the LRU candidate for eviction
// ============================================================================
CachedQuery *cache_lookup(QueryCache *cache, const char *query_string)
{
    if (cache == NULL || query_string == NULL)
        return NULL;

    // Search through cache for matching query string
    for (int i = 0; i < cache->cache_count; i++)
    {
        if (strcmp(cache->queries[i].query_string, query_string) == 0)
        {
            printf("[CACHE HIT] Query found in cache\n");

            // Update timestamp to mark as recently used
            cache->queries[i].timestamp = time(NULL);

            return &cache->queries[i];
        }
    }

    printf("[CACHE MISS] Query not found in cache\n");
    return NULL;
}

// ============================================================================
// FIND LRU - Find entry with oldest timestamp
// ============================================================================
//
// ALGORITHM:
// 1. Loop through all cached queries
// 2. Track minimum timestamp and its index
// 3. Return index of least recently used
// ============================================================================
static int cache_find_lru(QueryCache *cache)
{
    if (cache->cache_count == 0)
        return -1;

    int lru_index = 0;
    long oldest_time = cache->queries[0].timestamp;

    for (int i = 1; i < cache->cache_count; i++)
    {
        if (cache->queries[i].timestamp < oldest_time)
        {
            oldest_time = cache->queries[i].timestamp;
            lru_index = i;
        }
    }

    return lru_index;
}

// ============================================================================
// INSERT - Add query results to cache with LRU eviction
// ============================================================================
//
// STRATEGY: LRU (Least Recently Used)
// - Track timestamp of last access
// - When cache is full, evict the entry with oldest timestamp
// - Much better than FIFO for real workloads
// ============================================================================
void cache_insert(QueryCache *cache, const char *query_string, Person **results, int result_count)
{
    if (cache == NULL || query_string == NULL)
        return;

    int insert_index;

    // Check if we're updating an existing entry
    int existing_index = -1;
    for (int i = 0; i < cache->cache_count; i++)
    {
        if (strcmp(cache->queries[i].query_string, query_string) == 0)
        {
            existing_index = i;
            break;
        }
    }

    if (existing_index != -1)
    {
        // Update existing cache entry
        insert_index = existing_index;
        free(cache->queries[insert_index].results);
        printf("[CACHE UPDATE] Updating existing query: %s\n", query_string);
    }
    else if (cache->cache_count < cache->cache_size)
    {
        // Add to empty slot
        insert_index = cache->cache_count;
        cache->cache_count++;
    }
    else 
    {
        // Cache full - find and evict LRU entry
        int lru_index = cache_find_lru(cache);
        insert_index = lru_index;

        printf("[CACHE EVICT] Removing LRU entry: '%s' (timestamp: %ld)\n", cache->queries[lru_index].query_string, cache->queries[lru_index].timestamp);

        free(cache->queries[lru_index].results);
    }

    // Copy query string
    strncpy(cache->queries[insert_index].query_string, query_string, 255);
    cache->queries[insert_index].query_string[255] = '\0';

    // Copy results
    cache->queries[insert_index].results = malloc(result_count * sizeof(Person *));
    if (cache->queries[insert_index].results == NULL && result_count > 0)
    {
        printf("Failed to allocate memory for results\n");
        return;
    }

    // Copy pointers and set count
    for (int i = 0; i < result_count; i++)
        cache->queries[insert_index].results[i] = results[i];

    cache->queries[insert_index].result_count = result_count;
    cache->queries[insert_index].timestamp = time(NULL);

    printf("[CACHE INSERT] Stored %d results for query: '%s'\n", result_count, query_string);

}

// ============================================================================
// INVALIDATE - Clear cache when DB changes
// ============================================================================
//
// RULE: Call this whenever database is modified (insert/delete)
// Otherwise queries return stale data!
// ============================================================================
void cache_invalidate(QueryCache *cache)
{
    if (cache == NULL)
        return;

    // Free all cached results
    for (int i = 0; i < cache->cache_count; i++)
    {
        if (cache->queries[i].results != NULL)
            free(cache->queries[i].results);
    }

    cache->cache_count = 0;
    printf("[CACHE INVALIDATED] All entries cleared\n");
}

// ============================================================================
// STATISTICS - Show cache usage
// ============================================================================
void cache_display_stats(QueryCache *cache)
{
    if (cache == NULL)
        return;

    printf("\n=== Query Cache Statistics ===\n");
    printf("Cached queries: %d/%d\n", cache->cache_count, cache->cache_size);
    printf("Utilization:   %.1f%%\n", (100.0 * cache->cache_count) / cache->cache_size);

    if (cache->cache_count > 0)
    {
        printf("\nCached queries (ordered by recency):\n");
        // Find LRU to show it
        int lru_index = cache_find_lru(cache);
        time_t now = time(NULL);

        for (int i = 0; i < cache->cache_count; i++)
        {
            long age = now - cache->queries[i].timestamp;
            char marker[20] = "";

            if (i == lru_index)
                strcpy(marker, " <-- LRU");

            printf("  [%d] %s (%d results, age: %ld sec)%s\n",
                i, cache->queries[i].query_string, 
                cache->queries[i].result_count, age, marker);
        }
    }
  
      printf("==============================\n\n");
  } 