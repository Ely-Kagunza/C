#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <time.h>
#include "profiler.h"
#include "database.h"

// ============================================================================
// HIGH-RESOLUTION TIMER - Windows performance counter
// ============================================================================
//
// Much more accurate than clock() for short operations
// Returns microseconds (0.000001 second)
// ============================================================================

static LARGE_INTEGER perf_freq;
static int freq_initialized = 0;

static void timer_init(void)
{
    if (!freq_initialized)
    {
        QueryPerformanceFrequency(&perf_freq);
        freq_initialized = 1;
    }
}

static long get_time_us(void)
{
    timer_init();
    LARGE_INTEGER count;
    QueryPerformanceCounter(&count);
    return (count.QuadPart * 1000000) / perf_freq.QuadPart;
}

// ============================================================================
// CREATE - Allocate benchmark suite
// ============================================================================
BenchmarkSuite *benchmark_create(void)
{
    BenchmarkSuite *suite = malloc(sizeof(BenchmarkSuite));
    if (suite == NULL)
        return NULL;

    suite->results = malloc(sizeof(BenchmarkResult) * 50);  // Max 50 results
    if (suite->results == NULL)
    {
        free(suite);
        return NULL;
    }

    suite->result_count = 0;
    suite->result_capacity = 50;

    return suite;
}

// ============================================================================
// FREE - Deallocate benchmark suite
// ============================================================================
void benchmark_free(BenchmarkSuite *suite)
{
    if (suite == NULL)
        return;

    free(suite->results);
    free(suite);
}

// ============================================================================
// RECORD - Store benchmark result
// ============================================================================
void benchmark_record(BenchmarkSuite *suite, const char *test_name, long execution_time_us, int op_count, long memory)
{
    if (suite == NULL || suite->result_count >= suite->result_capacity)
        return;

    BenchmarkResult *result = &suite->results[suite->result_count];

    strncpy(result->test_name, test_name, sizeof(result->test_name) - 1);
    result->test_name[sizeof(result->test_name) - 1] = '\0';

    result->execution_time_us = execution_time_us;
    result->operations_count = op_count;
    result->memory_used = memory;
    result->ops_per_second = (op_count > 0 && execution_time_us > 0) ? (double)op_count * 1000000.0 / execution_time_us : 0;

    suite->result_count++;
}

// ============================================================================
// BENCHMARK 1: Linear Scan vs Hash Lookup vs B-Tree Range
// ============================================================================
void benchmark_search_methods(Database *db, BenchmarkSuite *suite)
{
    printf("\n=== Benchmark 1: Search Methods ===\n");

    if (db->count < 10)
    {
        printf("Need at least 10 records\n");
        return;
    }

    int target_id = db->records[db->count / 2].id;
    int iterations = 1000;

    // TEST 1: Linear scan (O(n))
    printf("Linear scan (%d iterations)...\n", iterations);
    long start = get_time_us();
    int found_count = 0;

    for (int iter = 0; iter < iterations; iter++)
    {
        for (int i = 0; i < db->count; i++)
        {
            if (db->records[i].id == target_id)
            {
                found_count++;
                break;
            }
        }
    }

    long linear_time = get_time_us() - start;
    benchmark_record(suite, "Linear Scan (1000x)", linear_time, iterations, 0);
    printf("  Time: %ld us, Found: %d\n\n", linear_time, found_count);

    // TEST 2: Hash lookup (O(1))
    printf("Hash lookup (%d iterations)...\n", iterations);
    start = get_time_us();
    found_count = 0;

    for (int iter = 0; iter < iterations; iter++)
    {
        Person *p = db_get_by_id(db, target_id);
        if (p != NULL)
            found_count++;
    }

    long hash_time = get_time_us() - start;
    benchmark_record(suite, "Hash Lookup (1000x)", hash_time, iterations, 0);
    printf("  Time: %ld us, Found: %d\n\n", hash_time, found_count);

    // TEST 3: B-Tree range search
    printf("B-Tree range search (100 iterations)...\n");
    start = get_time_us();

    BTree *bt = btree_create(5);
    for (int i = 0; i < db->count; i++)
    {
        btree_insert(bt, db->records[i].age, &db->records[i]);
    }

    Person **results = malloc(sizeof(Person *) * db->count);
    int range_count = 0;

    for (int iter = 0; iter < 100; iter++)
    {
        range_count += btree_range_search(bt, 25, 35, results);
    }

    long btree_time = get_time_us() - start;
    benchmark_record(suite, "B-Tree Range (100x)", btree_time, 100, 0);
    printf("  Time: %ld us, Found: %d\n\n", btree_time, range_count);

    free(results);
    btree_free(bt);

    // Summary
    printf("SPEEDUP (Hash vs Linear): %.2fx\n", (double)linear_time / hash_time);
    printf("SPEEDUP (B-Tree vs Linear): %.2fx\n\n", (double)linear_time / btree_time);
}

// ============================================================================
// BENCHMARK 2: Single Insert vs Batch Insert
// ============================================================================
void benchmark_inserts(Database *db, BenchmarkSuite *suite)
{
    printf("\n=== Benchmark 2: Insert Performance ===\n");

    // Skip if can't modify database
    printf("(Skipping - would modify database)\n");
}

// ============================================================================
// BENCHMARK 3: Cache Performance
// ============================================================================
void benchmark_cache(Database *db, BenchmarkSuite *suite)
{
    printf("\n=== Benchmark 3: Cache Performance ===\n");

    if (db->query_cache == NULL)
    {
        printf("Cache not initialized\n");
        return;
    }

    // Simulate repeated queries
    char query[256];
    int iterations = 100;

    // TEST 1: First access (cache miss)
    printf("First access (cache miss, 100 iterations)...\n");
    sprintf(query, "age:25-35");

    long start = get_time_us();
    for (int i = 0; i < iterations; i++)
    {
        cache_lookup(db->query_cache, query);
    }
    long miss_time = get_time_us() - start;

    benchmark_record(suite, "Cache Miss (100x)", miss_time, iterations, 0);
    printf("  Time: %ld us\n\n", miss_time);

    // Insert into cache
    Person **dummy_results = malloc(sizeof(Person *) * 10);
    dummy_results[0] = &db->records[0];
    cache_insert(db->query_cache, query, dummy_results, 1);
    free(dummy_results);

    // TEST 2: Repeated access (cache hit)
    printf("Repeated access (cache hit, 100 iterations)...\n");
    start = get_time_us();

    for (int i = 0; i < iterations; i++)
    {
        cache_lookup(db->query_cache, query);
    }

    long hit_time = get_time_us() - start;
    benchmark_record(suite, "Cache Hit (100x)", hit_time, iterations, 0);
    printf("  Time: %ld us\n\n", hit_time);

    printf("SPEEDUP (Cache Hit vs Miss): %.2fx\n\n", (double)hit_time / miss_time);

}

// ============================================================================
// RUN ALL BENCHMARKS
// ============================================================================
void benchmark_run_all(Database *db)
{
    printf("\n╔════════════════════════════════════════════╗\n");
    printf("║    PHASE 9 PERFORMANCE BENCHMARKING       ║\n");
    printf("╚════════════════════════════════════════════╝\n");
    
    BenchmarkSuite *suite = benchmark_create();
    if (suite == NULL)
    {
        printf("Failed to create benchmark suite\n");
        return;
    }
    
    benchmark_search_methods(db, suite);
    benchmark_cache(db, suite);
    
    benchmark_display_results(suite);
    benchmark_free(suite);
}

// ============================================================================
// DISPLAY RESULTS - Pretty table
// ============================================================================
void benchmark_display_results(BenchmarkSuite *suite)
{
    printf("\n╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║                   BENCHMARK RESULTS                          ║\n");
    printf("╠═══════════════════════════════════════════════════════════════╣\n");
    printf("║ Test Name              │ Time (µs)  │ Ops   │ Ops/sec        ║\n");
    printf("╟───────────────────────┼────────────┼───────┼────────────────╢\n");
    
    for (int i = 0; i < suite->result_count; i++)
    {
        BenchmarkResult *r = &suite->results[i];
        printf("║ %-21s │ %10ld │ %5d │ %14.0f ║\n",
               r->test_name, r->execution_time_us, r->operations_count, r->ops_per_second);
    }
    
    printf("╚═══════════════════════════════════════════════════════════════╝\n\n");
}

// ============================================================================
// COMPARE - Calculate speedup between two results
// ============================================================================
void benchmark_compare(BenchmarkResult *a, BenchmarkResult *b)
{
    if (a == NULL || b == NULL)
        return;
    
    if (a->execution_time_us > 0 && b->execution_time_us > 0)
    {
        double speedup = (double)a->execution_time_us / b->execution_time_us;
        printf("\n%s vs %s: %.2fx speedup\n", a->test_name, b->test_name, speedup);
    }
}