#include <stdio.h>
#include <stdlib.h>
#include "database.h"

// Create a new empty database with initial capacity
Database *db_create(int initial_capacity)
{
    Database *db = malloc(sizeof(Database));
    if (db == NULL)
    {
        printf("Failed to allocate database struct!\n");
        return NULL;
    }

    db->records = malloc(sizeof(Person) * initial_capacity);
    if (db->records == NULL)
    {
        printf("Failed to allocate memory for records!\n");
        free(db);
        return NULL;
    }

    db->count = 0;
    db->capacity = initial_capacity;

    return db;
}

void db_free(Database *db)
{
    if (db != NULL)
    {
        free(db->records);
        free(db);
    }
}

// Resize database: grow capacity when needed
int resize_database(Database *db, int new_capacity)
{
    if (new_capacity <= db->capacity)
    {
        printf("Error: cannot shrink below %d records!\n", db->capacity);
        return 0;
    }

    Person *new_records = realloc(db->records, sizeof(Person) * new_capacity);
    if (new_records == NULL)
    {
        printf("Memory allocation failed! Keeping old size.\n");
        return 0;
    }

    db->records = new_records;
    db->capacity = new_capacity;
    printf("Database resized to %d records.\n", new_capacity);
    return 1;
}

// Add a record to the database with automatic resizing
int db_add_record(Database *db, Person record)
{
    // Check if we need to resize
    if (db->count >= db->capacity)
    {
        int new_capacity = db->capacity * 2; // Grow by 2x
        if (!resize_database(db, new_capacity))
        {
            printf("Failed to add record: out of memory\n");
            return 0;
        }
    }

    // Add the record
    db->records[db->count++] = record;
    printf("Record added successfully. Database has %d/%d records.\n", db->count, db->capacity);
    return 1;
}

// Display all records in formatted table
void db_display(Database *db)
{
    if (db->count == 0)
    {
        printf("Database is empty.\n");
        return;
    }

    printf("\n");
    printf("┌────┬──────────────┬─────┬──────────┐\n");
    printf("│ ID │ Name         │ Age │ Salary   │\n");
    printf("├────┼──────────────┼─────┼──────────┤\n");

    for (int i = 0; i < db->count; i++)
    {
        printf("│ %-2d │ %-12s │ %3d │ %8.2f │\n",
               db->records[i].id,
               db->records[i].name,
               db->records[i].age,
               db->records[i].salary);
    }
    printf("└────┴──────────────┴─────┴──────────┘\n");
}

// Report memory usage statistics
void db_memory_stats(Database *db)
{
    printf("\n=== Database Memory Stats ===\n");
    printf("Records in use (count):    %d\n", db->count);
    printf("Space allocated (capacity): %d\n", db->capacity);
    printf("Bytes per record:          %zu\n", sizeof(Person));
    printf("Total allocated:           %ld bytes (%.2f KB)\n",
           (long)(db->capacity * sizeof(Person)),
           (db->capacity * sizeof(Person)) / 1024.0);
    printf("Actual data:               %ld bytes (%.2f KB)\n",
           (long)(db->count * sizeof(Person)),
           (db->count * sizeof(Person)) / 1024.0);
    printf("Unused space:              %ld bytes (%.1f%%)\n",
           (long)((db->capacity - db->count) * sizeof(Person)),
           100.0 * (db->capacity - db->count) / db->capacity);
    printf("Load factor:               %.1f%%\n",
           100.0 * db->count / db->capacity);
    printf("==============================\n\n");
}