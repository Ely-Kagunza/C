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
// LOOKUP - Find query in cache
// ============================================================================
//
// TIME: O(n) where n = cache_count (could optimize with hash if needed)
// RETURN: Pointer to cached results if found, NULL if not
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
            return &cache->queries[i];
        }
    }

    printf("[CACHE MISS] Query not found in cache\n");
    return NULL;
}

// ============================================================================
// INSERT - Add query results to cache
// ============================================================================
//
// STRATEGY: Simple FIFO (first-in-first-out)
// When cache is full, overwrite oldest entry
// (Production systems use LRU - least recently used)
// ============================================================================
void cache_insert(QueryCache *cache, const char *query_string, Person **results, int result_count)
{
    if (cache == NULL || query_string == NULL)
        return;

    int insert_index;

    // Check if we're overwritting an existing entry
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
    }
    else if (cache->cache_count < cache->cache_size)
    {
        // Add to empty slot
        insert_index = cache->cache_count;
        cache->cache_count++;
    }
    else
    {
        // Cache full - overwrite first entry (FIFO)
        printf("[CACHE FULL] Evicting oldest entry\n");
        free(cache->queries[0].results);
        insert_index = 0;

        // Shift all down by 1
        for (int i = 1; i < cache->cache_count - 1; i++)
        {
            cache->queries[i] = cache->queries[i + 1];
        }
        insert_index = cache->cache_count - 1;
    }

    // Copy query string
    strncpy(cache->queries[insert_index].query_string, query_string, 255);
    cache->queries[insert_index].query_string[255] = '\0';

    // Copy results
    cache->queries[insert_index].results = malloc(sizeof(Person *) * result_count);
    if (cache->queries[insert_index].results == NULL && result_count > 0)
    {
        printf("Failed to allocate cache results\n");
        return;
    }

    for (int i = 0; i < result_count; i++)
        cache->queries[insert_index].results[i] = results[i];

    cache->queries[insert_index].result_count = result_count;
    cache->queries[insert_index].timestamp = time(NULL);

    printf("[CACHE INSERT] Stored %d results for query: %s\n", result_count, query_string);

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
          printf("\nCached queries:\n");
          for (int i = 0; i < cache->cache_count; i++)
          {
              printf("  [%d] %s (%d results)\n", i, cache->queries[i].query_string, cache->queries[i].result_count);
          }
      }
  
      printf("==============================\n\n");
  } 