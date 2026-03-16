#ifndef PROFILER_H
#define PROFILER_H

#include "database.h"
#include "btree.h"
#include "cache.h"

// ============================================================================
// PERFORMANCE PROFILING AND BENCHMARKING
// ============================================================================
//
// GOAL: Measure real performance gains from Phase 9 features
//
// METRICS:
// - Execution time (ms, microseconds)
// - Operations per second
// - Memory usage
// - Cache hit/miss rates
//
// COMPARISONS:
// - Linear scan vs hash lookup vs B-tree range
// - Cached vs uncached queries
// - Single inserts vs batch inserts
//
// Python equivalent: timeit module, cProfile
// ============================================================================

typedef struct
{
    char test_name[100];                // Name of test
    long execution_time_us;             // Time in microseconds
    int operations_count;               // Number of operations performed
    double ops_per_second;              // Throughput
    long memory_used;                   // Memory in bytes allocated
} BenchmarkResult;

typedef struct
{
    BenchmarkResult *results;           // Array of results
    int result_count;                   // Number of results
    int result_capacity;                // Allocated size
} BenchmarkSuite;

// Create benchmark suite
BenchmarkSuite *benchmark_create(void);

// Free benchmark suite
void benchmark_free(BenchmarkSuite *suite);

// Record a result
void benchmark_record(BenchmarkSuite *suite, const char *test_name, long execution_time_us, int op_count, long memory);

// Run all Phase 9 benchmarks
void benchmark_run_all(Database *db);

// Display results table
void benchmark_display_results(BenchmarkSuite *suite);

// Compare two results (speedup calculation)
void benchmark_compare(BenchmarkResult *a, BenchmarkResult *b);

#endif // PROFILER_H