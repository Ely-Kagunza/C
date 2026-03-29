#define _CRT_SECURE_NO_WARNINGS
#include "../../include/query_cache.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

QueryCache *query_cache_create(int capacity) {
    if (capacity < 5) return NULL;

    QueryCache *cache = malloc(sizeof(QueryCache));
    if (!cache) return NULL;

    cache->queries = calloc(capacity, sizeof(CachedRangeQuery));
    if (!cache->queries) {
        free(cache);
        return NULL;
    }

    cache->cache_count = 0;
    cache->cache_size = capacity;
    printf("[QUERY CACHE] Created with capacity: %d\n", capacity);
    return cache;
}

void query_cache_free(QueryCache *cache) {
    if (!cache) return;

    for (int i = 0; i < cache->cache_count; i++) {
        if (cache->queries[i].results) {
            free(cache->queries[i].results);
        }
    }

    free(cache->queries);
    free(cache);
}

char *format_cache_key(char *buffer, int field_type, double min_val, double max_val) {
    if (!buffer) return NULL;

    if (field_type == 0) {
        snprintf(buffer, 256, "age:%.0f:%.0f", min_val, max_val);
    }
    else if (field_type == 1) {
        snprintf(buffer, 256, "salary:%.2f:%.2f", min_val, max_val);
    }
    else {
        return NULL;
    }

    return buffer;
}

CachedRangeQuery *query_cache_lookup(QueryCache *cache, const char *query_key) {
    if (!cache || !query_key) return NULL;

    for (int i = 0; i < cache->cache_count; i++) {
        if (strcmp(cache->queries[i].query_key, query_key) == 0) {
            printf("[CACHE HIT] %s (%d results)\n", query_key, cache->queries[i].result_count);
            return &cache->queries[i];
        }
    }

    printf("[CACHE MISS] %s\n", query_key);
    return NULL;
}

static int cache_find_lru(QueryCache *cache) {
    if (cache->cache_count == 0) return -1;

    int lru_index = 0;

    long oldest = cache->queries[0].timestamp;

    for (int i = 1; i < cache->cache_count; i++) {
        if (cache->queries[i].timestamp < oldest) {
            oldest = cache->queries[i].timestamp;
            lru_index = i;
        }
    }

    return lru_index;
}

void query_cache_insert(QueryCache *cache, const char *query_key, Person **results, int result_count) {
    if (!cache || !query_key) return;

    int insert_index = -1;

    // Check if updating existing
    for (int i = 0; i < cache->cache_count; i++) {
        if (strcmp(cache->queries[i].query_key, query_key) == 0) {
            insert_index = i;
            free(cache->queries[i].results);
            break;
        }
    }

    // Use empty slot
    if (insert_index == -1) {
        if (cache->cache_count < cache->cache_size) {
            insert_index = cache->cache_count;
            cache->cache_count++;
        } else {
            // Evict LRU
            int lru = cache_find_lru(cache);
            insert_index = lru;
            free(cache->queries[lru].results);
            printf("[CACHE EVICT] Removed LRU: %s\n", cache->queries[lru].query_key);
        }
    }

    strncpy(cache->queries[insert_index].query_key, query_key, sizeof(cache->queries[insert_index].query_key) - 1);

    cache->queries[insert_index].results = malloc((size_t)result_count * sizeof(Person *));
    for (int i = 0; i < result_count; i++) {
        cache->queries[insert_index].results[i] = results[i];
    }

    cache->queries[insert_index].result_count = result_count;
    cache->queries[insert_index].timestamp = time(NULL);

    printf("[CACHE STORE] %s (%d results)\n", query_key, result_count);
}

void query_cache_invalidate(QueryCache *cache) {
    if (!cache) return;

    for (int i = 0; i < cache->cache_count; i++) {
        if (cache->queries[i].results)
            free(cache->queries[i].results);
    }

    cache->cache_count = 0;
    printf("[CACHE INVALIDATED] All entries cleared\n");
}