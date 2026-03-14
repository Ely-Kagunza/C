#include <stdio.h>
#include <stdlib.h>
#include "batch.h"

// ============================================================================
// CREATE - Allocate batch
// ============================================================================
Batch *batch_create(int initial_capacity)
{
    if (initial_capacity < 10)
    {
        printf("Batch size must be at least 10\n");
        return NULL;
    }

    Batch *batch = malloc(sizeof(Batch));
    if (batch == NULL)
    {
        printf("Failed to allocate batch\n");
        return NULL;
    }

    batch->records = malloc(initial_capacity * sizeof(Person));
    if (batch->records == NULL)
    {
        printf("Failed to allocate batch records\n");
        free(batch);
        return NULL;
    }

    batch->count = 0;
    batch->capacity = initial_capacity;

    printf("Batch created with capacity %d\n", initial_capacity);
    return batch;
}

// ============================================================================
// FREE - Deallocate batch
// ============================================================================
void batch_free(Batch *batch)
{
    if (batch == NULL)
        return;

    free(batch->records);
    free(batch);
}

// ============================================================================
// ADD - Queue record for batch insert
// ============================================================================
void batch_add(Batch *batch, Person record)
{
    if (batch == NULL)
        return;

    // Resize if needed
    if (batch->count >= batch->capacity)
    {
        int new_capacity = batch->capacity * 2;
        Person *new_records = realloc(batch->records, new_capacity * sizeof(Person));
        if (new_records == NULL)
        {
            printf("Failed to resize batch\n");
            return;
        }
        batch->records = new_records;
        batch->capacity = new_capacity;
        printf("Batch resized to %d\n", new_capacity);
    }

    // Add record
    batch->records[batch->count++] = record;
}

// ============================================================================
// EXECUTE - Insert all batch records into database at once
// ============================================================================
//
// KEY OPTIMIZATION: Resize database once before inserting all records
// Instead of resizing inside db_add_record for each insert
//
// TIME SAVED:
// - Single resize vs multiple resizes
// - Single cache invalidation vs many
// - Better CPU cache locality
// ============================================================================
int batch_execute(Database *db, Batch *batch)
{
    if (db == NULL || batch == NULL)
        return 0;

    if (batch->count == 0)
    {
        printf("Batch is empty\n");
        return 0;
    }

    printf("\n=== Executing Batch Insert ===\n");
    printf("Inserting %d records...\n", batch->count);

    // Calculate how much space we need
    int space_needed = db->count + batch->count;
    int new_capacity = db->capacity;

    // Pre-allocate enough space for all records
    while (new_capacity < space_needed)
        new_capacity *= 2;

    // Resize if needed (ONCE)
    if (new_capacity > db->capacity)
    {
        printf("Pre-allocating space: %d -> %d\n", db->capacity, new_capacity);
        if (!resize_database(db, new_capacity))
        {
            printf("Failed to allocate space for batch\n");
            return 0;
        }
    }

    // Insert all records WITHOUT triggering db_add_record
    // (which would invalidate cache each time)
    int inserted = 0;
    for (int i = 0; i < batch->count; i++)
    {
        if (db->count >= db->capacity)
        {
            printf("Database full, inserted %d records\n", inserted);
            break;
        }

        // Add directly to array
        db->records[db->count++] = batch->records[i];

        // Add to hash index
        hash_insert(db->id_index, batch->records[i].id, batch->records[i]);

        inserted++;
    }

    // SINGLE cache invalidation after all inserts
    cache_invalidate(db->query_cache);

    printf("Batch insert complete: %d records added\n", inserted);
    printf("Database now has %d records\n\n", db->count);

    return inserted;
}

// ============================================================================
// STATISTICS - Show batch contents
// ============================================================================
void batch_display_stats(Batch *batch)
{
    if (batch == NULL)
        return;

    printf("\n=== Batch Statistics ===\n");
    printf("Records queued: %d/%d\n", batch->count, batch->capacity);
    printf("Utilization:   %.1f%%\n", (100.0 * batch->count) / batch->capacity);

    if (batch->count > 0)
    {
        printf("\nFirst 5 records in batch:\n");
        int show_count = batch->count < 5 ? batch->count : 5;
        for (int i = 0; i < show_count; i++)
        {
          printf("  [%d] ID:%d, %s (Age %d)\n", 
                 i, batch->records[i].id, batch->records[i].name, batch->records[i].age);
      }
  }

  printf("=======================\n\n");
}