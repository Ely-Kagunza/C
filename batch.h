#ifndef BATCH_H
#define BATCH_H

#include "database.h"

// ============================================================================
// BATCH OPERATIONS
// ============================================================================
//
// PROBLEM: Inserting 1000 records one-by-one causes:
// - 1000 cache invalidations (slow)
// - Multiple hash table resizes (expensive)
// - Repeated memory allocations
//
// SOLUTION: Queue records, insert all at once
// - 1 cache invalidation after all inserts
// - 1 hash table resize (if needed)
// - Batch allocations
//
// PERFORMANCE: ~50-70% faster for bulk inserts
//
// Python equivalent: PostgreSQL COPY or INSERT ... VALUES (...), (...), ...
// ============================================================================

typedef struct
{
    Person *records;              // Array of records to insert
    int count;                    // How many records in batch
    int capacity;                 // Allocated size
} Batch;

// Create empty batch
Batch *batch_create(int initial_capacity);

// Free batch
void batch_free(Batch *batch);

// Add record to batch
void batch_add(Batch *batch, Person record);

// Execute batch insert into database (ONE cache invalidation)
int batch_execute(Database *db, Batch *batch);

// Show batch statistics
void batch_display_stats(Batch *batch);

#endif // BATCH_H