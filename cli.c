#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cli.h"
#include "cache.h"
#include "database.h"
#include "btree.h"
#include "batch.h"
#include "threads.h"

// ============================================================================
// COMMAND PARSER - Determine what command user requested
// ============================================================================
//
// CONCEPT: Look at argv[1] to determine which command to run
// This is the "dispatcher" that routes to the right handler
//
// PYTHON COMPARISON:
// Similar to if/elif chains based on sys.argv[1]
// Or how web frameworks route URLs to handlers
// ============================================================================
CommandType parse_command(int argc, char *argv[])
{
    if (argc == 1)
        return CMD_INTERACTIVE;  // No arguments, go interactive mode

    if (argc < 2)
        return CMD_INVALID;      // Invalid command

    char *cmd = argv[1];

    if (strcmp(cmd, "query") == 0)
        return CMD_QUERY;
    else if (strcmp(cmd, "filter") == 0)
        return CMD_FILTER;
    else if (strcmp(cmd, "export") == 0)
        return CMD_EXPORT;
    else if (strcmp(cmd, "help") == 0 || strcmp(cmd, "--help") == 0)
        return CMD_HELP;
    else if (strcmp(cmd, "btree-range") == 0)
        return CMD_BTREE_RANGE;
    else if (strcmp(cmd, "cache-stats") == 0)
        return CMD_CACHE_STATS;
    else if (strcmp(cmd, "batch-insert") == 0)
        return CMD_BATCH_INSERT;
    else if (strcmp(cmd, "thread-test") == 0)
        return CMD_THREAD_TEST;
    else if (strcmp(cmd, "replication-sync") == 0)
        return CMD_REPLICATION_SYNC;
    else if (strcmp(cmd, "replication-failover") == 0)
        return CMD_REPLICATION_FAILOVER;
    else if (strcmp(cmd, "replication-stats") == 0)
        return CMD_REPLICATION_STATS;
    else
        return CMD_INVALID;
}

// ============================================================================
// HELP TEXT - Show usage information
// ============================================================================
void show_help(void)
{
    printf("\n");
    printf("╔════════════════════════════════════════════════════════╗\n");
    printf("║           Database CLI - Usage Information             ║\n");
    printf("╚════════════════════════════════════════════════════════╝\n\n");

    printf("USAGE:\n");
    printf("  ./database [COMMAND] [OPTIONS]\n\n");

    printf("COMMANDS:\n");
    printf("  query                Query a single record\n");
    printf("    --id <ID>          Find record by ID\n");
    printf("    --name <NAME>      Find record by name\n");
    printf("    Example: ./database query --id 1\n");
    printf("    Example: ./database query --name Joe\n\n");

    printf("  filter               Filter records by criteria\n");
    printf("    --age-min <NUM>    Minimum age\n");
    printf("    --age-max <NUM>    Maximum age\n");
    printf("    --salary-min <NUM> Minimum salary\n");
    printf("    --name <NAME>      Filter by name substring\n");
    printf("    Example: ./database filter --age-min 25 --age-max 35\n\n");

    printf("  export               Export records to file\n");
    printf("    --format <FORMAT>  Output format: csv or text\n");
    printf("    --output <FILE>    Output filename\n");
    printf("    Example: ./database export --format csv --output results.txt\n\n");

    printf("  help                 Show this help message\n\n");

    // ADD THESE PHASE 9 COMMANDS
    printf("PHASE 9 COMMANDS:\n");
    printf("  btree-range          B-tree range search\n");
    printf("    --min <AGE>        Minimum age\n");
    printf("    --max <AGE>        Maximum age\n");
    printf("    Example: ./database btree-range --min 25 --max 35\n\n");

    printf("  cache-stats          Show query cache statistics\n");
    printf("    Example: ./database cache-stats\n\n");

    printf("  batch-insert         Demo batch insert (5 records)\n");
    printf("    Example: ./database batch-insert\n\n");

    printf("  thread-test          Concurrent threading demo\n");
    printf("    Example: ./database thread-test\n\n");

    printf("INTERACTIVE MODE:\n");
    printf("  ./database           Run interactive menu (no arguments)\n\n");
}

// ============================================================================
// OPTION PARSER - Find option value in argv
// ============================================================================
//
// CONCEPT: Search argv for a flag like "--id" and return the next argument
// This is called by handlers to extract option values
//
// ALGORITHM:
// 1. Loop through argv looking for the flag
// 2. If found, check there's a next argument (the value)
// 3. Return that value
// 4. If not found or no value, return NULL
//
// EXAMPLE:
// argv = ["./db", "query", "--id", "1", "--name", "Joe"]
// find_option(argv, argc, "--id") returns "1"
// find_option(argv, argc, "--name") returns "Joe"
// ============================================================================
static char *find_option(int argc, char *argv[], const char *option)
{
    for (int i = 1; i < argc - 1; i++)
    {
        if (strcmp(argv[i], option) == 0)
        {
            // Found the option, return the next argument
            return argv[i + 1];
        }
    }
    return NULL;  // Option not found or no value
}

// ============================================================================
// QUERY HANDLER - Find a single record by ID or name
// ============================================================================
//
// COMMAND SYNTAX:
//   ./database query --id 1
//   ./database query --name Joe
// ============================================================================
int handle_query(int argc, char *argv[], Database *db)
{
    char *id_str = find_option(argc, argv, "--id");
    char *name_str = find_option(argc, argv, "--name");

    // Check that user provided at least one search criterion
    if (id_str == NULL && name_str == NULL)
    {
        printf("Error: Query requires --id or --name\n");
        printf("Usage: ./database query --id <ID>\n");
        printf("       ./database query --name <NAME>\n");
        return 1;
    }

    // Search by ID
    if (id_str != NULL)
    {
        int id = atoi(id_str);  // Convert string to int
        Person *found = db_get_by_id(db, id);

        if (found != NULL)
        {
            printf("\n✓ Found record:\n");
            printf("  ID:     %d\n", found->id);
            printf("  Name:   %s\n", found->name);
            printf("  Age:    %d\n", found->age);
            printf("  Salary: %.2f\n\n", found->salary);
            return 0;
        }
        else
        {
            printf("✗ No record found with ID %d\n\n", id);
            return 1;
        }
    }

    // Search by name
    if (name_str != NULL)
    {
        int idx = db_search_by_name(db, name_str);

        if (idx >= 0)
        {
            Person *found = &db->records[idx];
            printf("\n✓ Found record:\n");
            printf("  ID:     %d\n", found->id);
            printf("  Name:   %s\n", found->name);
            printf("  Age:    %d\n", found->age);
            printf("  Salary: %.2f\n\n", found->salary);
            return 0;
        }
        else
        {
            printf("✗ No record found with name '%s'\n\n", name_str);
            return 1;
        }
    }

    return 0;
}

// ============================================================================
// FILTER HANDLER - Find records matching criteria
// ============================================================================
//
// COMMAND SYNTAX:
//   ./database filter --age-min 25 --age-max 35
//   ./database filter --salary-min 50000
//   ./database filter --name substring
// ============================================================================
int handle_filter(int argc, char *argv[], Database *db)
{
    FilterCriteria criteria = {0};

    // Parse age min
    char *age_min_str = find_option(argc, argv, "--age-min");
    if (age_min_str != NULL)
    {
        criteria.has_age_min = 1;
        criteria.age_min = atoi(age_min_str);
    }

    // Parse age max
    char *age_max_str = find_option(argc, argv, "--age-max");
    if (age_max_str != NULL)
    {
        criteria.has_age_max = 1;
        criteria.age_max = atoi(age_max_str);
    }

    // Parse salary min
    char *salary_min_str = find_option(argc, argv, "--salary-min");
    if (salary_min_str != NULL)
    {
        criteria.has_salary_min = 1;
        criteria.salary_min = atof(salary_min_str);  //atof for floating point
    }

    // Parse name filter
    char *name_str = find_option(argc, argv, "--name");
    if (name_str != NULL)
    {
        criteria.has_name_filter = 1;
        strncpy(criteria.name_filter, name_str, sizeof(criteria.name_filter) - 1);
    }

    // Check that user provided at least one criterion
    if (!criteria.has_age_min && !criteria.has_age_max && 
      !criteria.has_salary_min && !criteria.has_name_filter)
    {
        printf("Error: Filter requires at least one criterion\n");
        printf("Usage: ./database filter --age-min 25 --age-max 35\n");
        return 1;
    }

    // Apply filters
    int match_count = 0;
    printf("\n");

    for (int i = 0; i < db->count; i++)
    {
        Person *p = &db->records[i];
        int matches = 1;

        // Check age-min
        if (criteria.has_age_min && p->age < criteria.age_min)
            matches = 0;

        // Check age-max
        if (criteria.has_age_max && p->age > criteria.age_max)
            matches = 0;

        // Check salary-min
        if (criteria.has_salary_min && p->salary < criteria.salary_min)
            matches = 0;

        // Check name (substring match)
        if (criteria.has_name_filter && strstr(p->name, criteria.name_filter) == NULL)
            matches = 0;

        if (matches)
        {
            printf("ID: %d | Name: %-15s | Age: %3d | Salary: %.2f\n",
                    p->id, p->name, p->age, p->salary);
            match_count++;
        }
    }
    
    printf("\nFound %d matching record(s).\n\n", match_count);
    return 0;
}

// ============================================================================
// EXPORT HANDLER - Save records to file
// ============================================================================
//
// COMMAND SYNTAX:
//   ./database export --format csv --output results.txt
//   ./database export --format text --output results.txt
// ============================================================================
int handle_export(int argc, char *argv[], Database *db)
{
    ExportOptions opts = {0};

    // Parse format
    char *format_str = find_option(argc, argv, "--format");
    if (format_str == NULL)
    {
        printf("Error: Invalid format '%s'\n", format_str);
        return 1;
    }
    strncpy(opts.format, format_str, sizeof(opts.format) - 1);

    // Parse output file
    char *output_str = find_option(argc, argv, "--output");
    if (output_str == NULL)
    {
        printf("Error: Invalid output file '%s'\n", output_str);
        return 1;
    }
    strncpy(opts.output_file, output_str, sizeof(opts.output_file) - 1);

    // Validate format
    if (strcmp(opts.format, "csv") != 0 && strcmp(opts.format, "text") != 0)
    {
        printf("Error: Invalid format '%s'\n", opts.format);
        return 1;
    }

    // Open output file
    FILE *file = fopen(opts.output_file, "w");
    if (file == NULL)
    {
        printf("Error: Failed to open output file '%s'\n", opts.output_file);
        return 1;
    }

    // Export as CSV
    if (strcmp(opts.format, "csv") == 0)
    {
        fprintf(file, "ID,Name,Age,Salary\n");
        for (int i = 0; i < db->count; i++)
        {
            Person *p = &db->records[i];
            fprintf(file, "%d,%s,%d,%.2f\n", p->id, p->name, p->age, p->salary);
        }
    }

    // Export as text table
    else if (strcmp(opts.format, "text") == 0)
    {
        fprintf(file, "┌────┬──────────────┬─────┬──────────┐\n");
        fprintf(file, "│ ID │ Name         │ Age │ Salary   │\n");
        fprintf(file, "├────┼──────────────┼─────┼──────────┤\n");

        for (int i = 0; i < db->count; i++)
        {
            Person *p = &db->records[i];
            fprintf(file, "│ %-2d │ %-12s │ %3d │ %8.2f │\n",
                    p->id, p->name, p->age, p->salary);
        }

        fprintf(file, "└────┴──────────────┴─────┴──────────┘\n");
    }

    // Close file
    fclose(file);
    printf("✓ Exported %d record(s) to '%s' (%s format)\n\n",
           db->count, opts.output_file, opts.format);
    return 0;
}

// ============================================================================
// PHASE 9: B-TREE RANGE SEARCH
// ============================================================================
//
// USAGE: database btree-range --min AGE --max AGE
// EXAMPLE: database btree-range --min 25 --max 35
// ============================================================================
void handle_btree_range(int argc, char *argv[], Database *db)
{
    printf("\n=== B-TREE RANGE SEARCH ===\n");

    int min_age = -1, max_age = -1;

    // Parse --min and --max arguments
    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "--min") == 0 && i + 1 < argc)
            min_age = atoi(argv[i + 1]);

        if (strcmp(argv[i], "--max") == 0 && i + 1 < argc)
            max_age = atoi(argv[i + 1]);
    }

    if (min_age == -1 || max_age == -1)
    {
        printf("Usage: database btree-range --min AGE --max AGE\n");
        printf("Example: database btree-range --min 25 --max 35\n");
        return;
    }

    // Create B-tree indexed by age
    BTree *age_index = btree_create(3); // B-tree with 3 children per node
    if (age_index == NULL)
    {
        printf("Failed to create B-tree index\n");
        return;
    }

    printf("Building B-tree index...\n");
    for (int i = 0; i < db->count; i++)
    {
        btree_insert(age_index, db->records[i].age, &db->records[i]);
    }

    // Search range
    Person **results = malloc(db->count * sizeof(Person *));
    if (results == NULL)
    {
        printf("Memory allocation failed\n");
        btree_free(age_index);
        return;
    }

    int count = btree_range_search(age_index, min_age, max_age, results);

    printf("\nPeople aged %d-%d:\n", min_age, max_age);
    printf("Found %d results (O(log n) range search)\n\n", count);

    for (int i = 0; i < count; i++)
    {
        printf("  ID: %d | Name: %-15s | Age: %d | Salary: %.2f\n",
               results[i]->id, results[i]->name, results[i]->age, results[i]->salary);
    }
    printf("\n");

    free(results);
    btree_free(age_index);
}

// ============================================================================
// PHASE 9: QUERY CACHE STATISTICS
// ============================================================================
//
// USAGE: database cache-stats
// ============================================================================
void handle_cache_stats(Database *db)
{
    printf("\n=== QUERY CACHE STATISTICS ===\n");

    if (db->query_cache == NULL)
    {
        printf("Query cache not initialized\n");
        return;
    }

    cache_display_stats(db->query_cache);
}

// ============================================================================
// PHASE 9: BATCH INSERT DEMO
// ============================================================================
//
// USAGE: database batch-insert
// Creates a batch with 5 sample records and inserts them
// ============================================================================
void handle_batch_insert(Database *db)
{
    printf("\n=== BATCH INSERT DEMO ===\n\n");

    Batch *batch = batch_create(100);
    if (batch == NULL)
    {
        printf("Failed to create batch\n");
        return;
    }

    // Create sample records
    Person samples[] = {
        {101, "Alice", 25, 50000.00},
        {102, "Bob", 30, 60000.00},
        {103, "Charlie", 35, 70000.00},
        {104, "David", 40, 80000.00},
        {105, "Eve", 45, 90000.00},
    };

    printf("Queueing 5 records in batch...\n");
    for (int i = 0; i < 5; i++)
    {
        batch_add(batch, samples[i]);
    }

    batch_display_stats(batch);

    int inserted = batch_execute(db, batch);
    printf("Successfully inserted %d records\n", inserted);
    printf("Database now has %d total records\n\n", db->count);

    batch_free(batch);
}

// ============================================================================
// PHASE 9: THREADING TEST
// ============================================================================
//
// USAGE: database thread-test
// Spawns 5 concurrent threads searching the database
// ============================================================================
void handle_thread_test(Database *db)
{
    printf("\n=== CONCURRENT THREADING TEST ===\n\n");

    ThreadSafeDatabase *tsdb = threadsafe_db_create(db);
    if (tsdb == NULL)
    {
        printf("Failed to create thread-safe database\n");
        return;
    }

    printf("Spawning 5 concurrent search threads...\n");

    HANDLE threads[5];
    ThreadSearchTask tasks[5];
    int search_ids[5] = {101, 102, 101, 102, 101};

    // Create threads
    for (int i = 0; i < 5; i++)
    {
        tasks[i].thread_id = i;
        tasks[i].tsdb = tsdb;
        tasks[i].search_id = search_ids[i];
        tasks[i].result = NULL;

        threads[i] = CreateThread(NULL, 0, worker_search_thread, &tasks[i], 0, NULL);
        if (threads[i] == NULL)
        {
            printf("Failed to create thread %d\n", i);
            return;
        }
    }

    // Wait for all threads to complete
    WaitForMultipleObjects(5, threads, TRUE, INFINITE);

    printf("All threads completed\n");
    for (int i = 0; i < 5; i++)
    {
        if (tasks[i].result != NULL)
            printf("  Thread %d: Found %s\n", i, tasks[i].result->name);
        else
            printf("  Thread %d: Not found\n", i);
    }
    printf("\n");

    // Cleanup
    for (int i = 0; i < 5; i++)
        CloseHandle(threads[i]);

    threadsafe_db_free(tsdb);
}

// void handle_replication_sync(Database *db)
// {
//     printf("\n=== Replication Sync ===\n");
//     printf("This would sync replica (feature requires replication manager integration)\n\n");
// }

// void handle_replication_stats(ReplicationManager *rm)
// {
//     if (rm == NULL)
//     {
//         printf("Replication not initialized\n");
//         return;
//     }
//     replication_display_stats(rm);
// }

// void handle_replication_failover(ReplicationManager *rm)
// {
//     if (rm == NULL)
//     {
//         printf("Replication not initialized\n");
//         return;
//     }
//     replication_failover(rm);
// }
          


// ============================================================================
// MAIN CLI DISPATCHER - Route commands to handlers
// ============================================================================
void run_cli_mode(int argc, char *argv[], Database *db)
{
    CommandType cmd = parse_command(argc, argv);

    switch (cmd)
    {
    case CMD_QUERY:
        handle_query(argc, argv, db);
        break;
    case CMD_FILTER:
        handle_filter(argc, argv, db);
        break;
    case CMD_EXPORT:
        handle_export(argc, argv, db);
        break;
    case CMD_HELP:
        show_help();
        break;
    case CMD_BTREE_RANGE:
        handle_btree_range(argc, argv, db);
        break;
    case CMD_CACHE_STATS:
        handle_cache_stats(db);
        break;
    case CMD_BATCH_INSERT:
        handle_batch_insert(db);
        break;
    case CMD_THREAD_TEST:
        handle_thread_test(db);
        break;
    case CMD_INTERACTIVE:
        // This won't happen here, but for completeness
        printf("Error: Should use interactive mode\n");
        break;
    case CMD_INVALID:
        printf("Error: Unknown command '%s'\n", argv[1]);
        printf("Use './database help' for usage information\n");
        break;
    }
}