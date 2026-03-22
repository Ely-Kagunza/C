#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "net_socket_defs.h"
#include "database.h"
#include "io.h"
#include "query.h"
#include <time.h>
#include "cli.h"
#include "btree.h"
#include "batch.h"
#include "threads.h"
#include <windows.h>
#include "threadpool.h"
#include "profiler.h"
#include "replication.h"
#include "sql_tokenizer.h"
#include "sql_parser.h"
#include "sql_executor.h"
#include "sql_callbacks.h"
#include "sql_executor_strategies.h"
#include "sql_macro_system.h"
#include "sys_io_strategies.h"
#include "net_protocol.h"
#include "net_client.h"
#include "net_server.h"

void show_menu(void)
{
    printf("\n=== Database Menu ===\n");
    printf("1. Display all records\n");
    printf("2. Sort by age\n");
    printf("3. Sort by name\n");
    printf("4. Sort by salary (descending)\n");
    printf("5. Search by name\n");
    printf("6. Filter by age\n");
    printf("7. Binary search by ID\n");
    printf("8. Compare two records\n");
    printf("9. Sort by ID\n");
    printf("10. Compare search performance\n");
    printf("11. Add new record\n");
    printf("12. Show memory stats\n");
    printf("13. Test hash table\n");
    printf("14. Test B-tree\n");
    printf("15. Test query cache\n");
    printf("16. Test Batch Operations\n");
    printf("17. Test Threading\n");
    printf("18. Test Thread Pool\n");
    printf("19. Run Performance Benchmarks\n");
    printf("20. Test Database Replication\n");
    printf("21. Test SQL Tokenizer\n");
    printf("22. Test SQL Parser\n");
    printf("23. Test Callbacks\n");
    printf("24. Test Vtable Executors\n");
    printf("25. Test Macros\n");
    printf("26. Test Systems Programming\n");
    printf("27. Test Network Programming\n");
    printf("28. Quit\n");
    printf("Selection: ");
}

// Hash table performance and functionality
// Test Phase 7: Hash table performance and functionality
void test_hash_table(Database *db)
{
    printf("\n=== Phase 7: Hash Table Testing ===\n\n");

    // Test 1: Basic Lookup
    printf("TEST 1: Direct Hash Lookup (O(1))\n");
    printf("Looking for ID 1...\n");
    Person *found = db_get_by_id(db, 1);
    if (found != NULL)
    {
        printf("✓ Found: %s, Age=%d, Salary=%.2f\n\n", found->name, found->age, found->salary);
    }
    else
    {
        printf("✗ Not found.\n\n");
    }

    // Test 2: Multiple lookups
    printf("TEST 2: Multiple Random Lookups\n");
    int test_ids[] = {1, 5, 3, 7, 2};
    for (int i = 0; i < 5; i++)
    {
        Person *p = db_get_by_id(db, test_ids[i]);
        if (p != NULL)
            printf("  ID %d: %s\n", test_ids[i], p->name);
        else
            printf("  ID %d: Not found\n", test_ids[i]);
    }
    printf("\n");

    // Test 3: Hash table statistics
    printf("TEST 3: Hash Table Statistics\n");
    hash_display_stats(db->id_index);

    // Test 4: Performance comparison
    printf("TEST 4: Performance Comparison (Binary vs Hash)\n");
    if (db->count >= 1)
    {
        int target_id = db->records[db->count / 2].id;
        printf("Searching for ID %d in %d records...\n\n", target_id, db->count);

        // Binary search (requires sorting)
        clock_t start_binary = clock();
        int binary_result = db_binary_search_by_id(db, target_id);
        clock_t end_binary = clock();
        double binary_time = ((double)(end_binary - start_binary)) / CLOCKS_PER_SEC * 1000000;

        // Hash lookup (O(1))
        clock_t start_hash = clock();
        Person *hash_result = db_get_by_id(db, target_id);
        clock_t end_hash = clock();
        double hash_time = ((double)(end_hash - start_hash)) / CLOCKS_PER_SEC * 1000000;

        printf("Binary Search:  %.4f microseconds\n", binary_time);
        printf("Hash Lookup:    %.4f microseconds\n", hash_time);
        if (hash_time > 0)
            printf("Hash is %.1fx faster\n", binary_time / hash_time);
        printf("\n");
    }
}

// Test Phase 9: B-Tree range queries
void test_btree(Database *db)
{
    printf("\n=== Phase 9: B-Tree with Node Splitting ===\n\n");

    BTree *bt = btree_create(3); // Order 3: max 5 keys per node
    if (bt == NULL)
    {
        printf("Failed to create B-tree\n");
        return;
    }

    // TEST 1: Insert enough to cause splits
    printf("TEST 1: Inserting 10 records (will trigger splits)\n");
    printf("Order 3 = max 5 keys per node, so splits happen at 6th insert\n\n");

    Person test_people[] = {
        {1, "Alice", 25, 50000},
        {2, "Bob", 28, 55000},
        {3, "Charlie", 32, 60000},
        {4, "Diana", 29, 58000},
        {5, "Eve", 31, 62000},
        {6, "Frank", 26, 52000}, // This will trigger first split
        {7, "Grace", 34, 65000},
        {8, "Henry", 27, 54000},
        {9, "Iris", 33, 63000},
        {10, "Jack", 30, 59000}};

    for (int i = 0; i < 10; i++)
    {
        printf("Inserting: %s (ID %d, Age %d)\n", test_people[i].name, test_people[i].id, test_people[i].age);
        btree_insert(bt, test_people[i].age, &test_people[i]);
    }

    btree_display_stats(bt);

    // TEST 2: Range search on newly built tree
    printf("TEST 2: Range search (Age 25-32)\n");
    Person **results = malloc(sizeof(Person *) * 100);
    int count = btree_range_search(bt, 25, 32, results);

    printf("Found %d people:\n", count);
    for (int i = 0; i < count; i++)
    {
        printf("  - %s (Age %d)\n", results[i]->name, results[i]->age);
    }
    printf("\n");

    // TEST 3: Exact search
    printf("TEST 3: Exact search (Age 29)\n");
    Person *found = btree_search(bt, 29);
    if (found != NULL)
    {
        printf("Found: %s (Age 29)\n\n", found->name);
    }

    free(results);
    btree_free(bt);
}

// Test Phase 9: Query cache
void test_query_cache(Database *db)
{
    printf("\n=== Phase 9: Query Cache Testing ===\n\n");

    // Simulate filtering by age (without B-tree, just for demo)
    printf("TEST 1: Cache the same query twice\n");

    // First query: "age 25-35"
    char query1[256];
    sprintf(query1, "age:25-35");

    // Check cache (should miss)
    CachedQuery *cached = cache_lookup(db->query_cache, query1);
    if (cached == NULL)
    {
        printf("Building results for: %s\n", query1);
        Person **results = malloc(sizeof(Person *) * db->count);
        int count = 0;

        // Simulate query execution
        for (int i = 0; i < db->count; i++)
        {
            if (db->records[i].age >= 25 && db->records[i].age <= 35)
                results[count++] = &db->records[i];
        }

        // Cache the results
        cache_insert(db->query_cache, query1, results, count);

        printf("Results found: %d\n\n", count);
        free(results);
    }

    // Second query: same "age 25-35" (should hit cache!)
    printf("TEST 2: Same query again (should hit cache)\n");
    cached = cache_lookup(db->query_cache, query1);
    if (cached != NULL)
    {
        printf("Retrieved from cache: %d results\n", cached->result_count);
        for (int i = 0; i < cached->result_count; i++)
        {
            printf("  - %s, Age %d\n", cached->results[i]->name, cached->results[i]->age);
        }
    }
    printf("\n");

    // Different query
    printf("TEST 3: Different query\n");
    char query2[256];
    sprintf(query2, "age:20-28");

    cached = cache_lookup(db->query_cache, query2);
    if (cached == NULL)
    {
        printf("Building results for: %s\n", query2);
        Person **results = malloc(sizeof(Person *) * db->count);
        int count = 0;

        for (int i = 0; i < db->count; i++)
        {
            if (db->records[i].age >= 20 && db->records[i].age <= 28)
                results[count++] = &db->records[i];
        }

        cache_insert(db->query_cache, query2, results, count);
        printf("Results found: %d\n\n", count);
        free(results);
    }

    // Show cache stats
    printf("TEST 4: Cache Statistics\n");
    cache_display_stats(db->query_cache);
}

// Test Phase 9: Batch operations
void test_batch_operations(Database *db)
{
    printf("\n=== Phase 9: Batch Operations Testing ===\n\n");

    // TEST 1: Create batch
    printf("TEST 1: Queue records in batch\n");
    Batch *batch = batch_create(100);

    // Create fake records to add
    Person test_records[] = {
        {101, "Alice", 25, 50000},
        {102, "Bob", 28, 55000},
        {103, "Charlie", 32, 60000},
        {104, "Diana", 29, 58000},
        {105, "Eve", 31, 62000}};

    // Add to batch
    for (int i = 0; i < 5; i++)
    {
        batch_add(batch, test_records[i]);
    }

    batch_display_stats(batch);

    // TEST 2: Execute batch
    printf("TEST 2: Execute batch insert\n");
    int inserted = batch_execute(db, batch);
    printf("Successfully inserted %d records\n\n", inserted);

    // TEST 3: Verify records were inserted
    printf("TEST 3: Verify inserted records\n");
    Person *found = db_get_by_id(db, 101);
    if (found != NULL)
    {
        printf("✓ Alice (ID 101) found: Age %d, Salary %.2f\n", found->age, found->salary);
    }
    else
    {
        printf("✗ Alice not found\n");
    }

    printf("\nDatabase now contains %d records\n\n", db->count);

    batch_free(batch);
}

// Test Phase 9: Threading for concurrent access
void test_threading(Database *db)
{
    printf("\n=== Phase 9: Threading (Concurrent Access) ===\n\n");

    // Create thread-safe database wrapper
    ThreadSafeDatabase *tsdb = threadsafe_db_create(db);
    if (tsdb == NULL)
    {
        printf("Failed to create thread-safe database\n");
        return;
    }

    // TEST 1: Single-threaded baseline
    printf("TEST 1: Single-threaded baseline\n");
    Person *found = threadsafe_get_by_id(tsdb, 102);
    if (found != NULL)
        printf("✓ Found ID 102: %s\n\n", found->name);
    else
        printf("✗ ID 102 not found\n\n");

    // TEST 2: Multi-threaded concurrent searches
    printf("TEST 2: 5 concurrent threads searching simultaneously\n");
    HANDLE threads[5]; // Windows thread handles
    ThreadSearchTask tasks[5];
    int search_ids[] = {1, 2, 1, 2, 1};

    // Create worker threads
    for (int i = 0; i < 5; i++)
    {
        tasks[i].thread_id = i;
        tasks[i].tsdb = tsdb;
        tasks[i].search_id = search_ids[i];
        tasks[i].result = NULL;

        threads[i] = CreateThread(
            NULL,                 // Default security attributes
            0,                    // Default stack size
            worker_search_thread, // Thread function
            &tasks[i],            // Thread argument
            0,                    // Creation flags
            NULL                  // Thread ID (not needed)
        );

        if (threads[i] == NULL)
        {
            printf("Failed to create thread %d\n", i);
            return;
        }
    }

    // Wait for all threads to finish
    printf("\nWaiting for threads to complete...\n");
    WaitForMultipleObjects(5, threads, TRUE, INFINITE);

    // Collect results
    printf("\nResults from all threads:\n");
    for (int i = 0; i < 5; i++)
    {
        if (tasks[i].result != NULL)
            printf("  Thread %d: Found %s\n", i, tasks[i].result->name);
        else
            printf("  Thread %d: Not found\n", i);
    }
    printf("\n");

    // Close thread handles
    for (int i = 0; i < 5; i++)
    {
        CloseHandle(threads[i]);
    }

    // TEST 3: Demonstrate critical section protection
    printf("TEST 3: Why critical sections matter\n");
    printf("WITHOUT critical section: Threads could corrupt data\n");
    printf("WITH critical section: Only one thread accesses database at a time\n");
    printf("  - Prevents data corruption\n");
    printf("  - Ensures consistency\n");
    printf("  - Small performance cost for safety\n\n");

    threadsafe_db_free(tsdb);
}

// Example task: simulate some work
void example_task(void *arg)
{
    int task_num = *(int *)arg;
    printf("[Task %d] Doing work...\n", task_num);
    Sleep(500); // Simulate work
    printf("[Task %d] Done!\n", task_num);
    free(arg);
}

// Test Phase 9: Thread pool
void test_threadpool(void)
{
    printf("\n=== Phase 9: Thread Pool ===\n\n");

    // Create pool with 4 workers
    ThreadPool *pool = threadpool_create(4);
    if (pool == NULL)
    {
        printf("Failed to create thread pool\n");
        return;
    }

    // Submit 10 tasks
    printf("Submitting 10 tasks to 4-worker pool...\n\n");
    for (int i = 0; i < 10; i++)
    {
        int *task_num = malloc(sizeof(int));
        *task_num = i;
        threadpool_submit(pool, example_task, task_num);
        Sleep(100); // Stagger submissions
    }

    // Let tasks complete
    printf("\nWaiting for completion...\n");
    Sleep(3000);

    threadpool_display_stats(pool);
    threadpool_free(pool);
}

void test_replication(Database *db)
{
    printf("\n=== Phase 9: Database Replication ===\n\n");

    // Create replication manager
    ReplicationManager *rm = replication_create(db);
    if (rm == NULL)
    {
        printf("Failed to create replication\n");
        return;
    }

    // TEST 1: Initial sync
    printf("TEST 1: Initial state\n");
    replication_display_stats(rm);

    // TEST 2: Log some changes
    printf("TEST 2: Simulate 3 new inserts\n");
    Person new_records[] = {
        {201, "Repl1", 40, 70000},
        {202, "Repl2", 35, 65000},
        {203, "Repl3", 45, 75000}};

    for (int i = 0; i < 3; i++)
    {
        db_add_record(db, new_records[i]);
    }

    replication_display_stats(rm);

    printf("\nTEST 3: Detect changes (replication system reads primary)\n");
    replication_detect_changes(rm); // System notices changes

    replication_display_stats(rm);

    printf("TEST 4: Apply changes to replica\n");
    replication_sync(rm); // Replica replays log

    replication_display_stats(rm);

    printf("TEST 5: Failover\n");
    replication_failover(rm);

    replication_display_stats(rm);

    replication_free(rm);
}

// Add this to test the tokenizer
void test_tokenizer()
{
    const char *queries[] = {
        "SELECT * FROM people",
        "SELECT id, name FROM people WHERE age > 30",
        "SELECT * FROM people WHERE age > 30 AND salary < 100000",
        "INSERT INTO people VALUES (1, 'John', 30, 50000)",
        "UPDATE people SET salary = 60000 WHERE id = 1",
        "DELETE FROM people WHERE age < 18",
        "SELECT * FROM people ORDER BY salary DESC LIMIT 10"};

    printf("\n=== Testing SQL Tokenizer ===\n\n");

    for (int i = 0; i < 7; i++)
    {
        printf("Query: %s\n", queries[i]);
        TokenList *tokens = tokenize(queries[i]);
        token_list_print(tokens);
        token_list_free(tokens);
    }
}

void test_parser()
{
    const char *queries[] = {
        "SELECT * FROM people",
        "SELECT id, name FROM people WHERE age > 30",
        "SELECT * FROM people WHERE age > 30 AND salary < 100000 ORDER BY salary DESC",
        "SELECT id, name, salary FROM people WHERE age > 30 ORDER BY salary DESC LIMIT 10",
    };

    printf("\n=== Testing SQL Parser & Executor ===\n\n");

    // Load database
    Database *db = db_load_from_file("people.db");
    if (db == NULL)
    {
        printf("Failed to load database\n");
        return;
    }

    // Insert 20 sample records
    printf("Inserting 20 sample records...\n\n");
    Person test_data[] = {
        {1, "Alice", 25, 50000},
        {2, "Bob", 28, 55000},
        {3, "Charlie", 32, 60000},
        {4, "Diana", 29, 58000},
        {5, "Eve", 31, 62000},
        {6, "Frank", 26, 52000},
        {7, "Grace", 34, 65000},
        {8, "Henry", 27, 54000},
        {9, "Iris", 33, 63000},
        {10, "Jack", 30, 59000},
        {11, "Karen", 35, 70000},
        {12, "Leo", 24, 48000},
        {13, "Mia", 38, 75000},
        {14, "Noah", 22, 45000},
        {15, "Olivia", 36, 68000},
        {16, "Peter", 41, 80000},
        {17, "Quinn", 23, 47000},
        {18, "Rachel", 39, 78000},
        {19, "Sam", 40, 82000},
        {20, "Tina", 37, 72000}};

    for (int i = 0; i < 20; i++)
    {
        db_add_record(db, test_data[i]);
    }

    printf("\n✅ Database now has %d records\n\n", db->count);

    // Now run the SQL queries
    printf("═══════════════════════════════════════════════════════\n");
    printf("        SQL PARSER & EXECUTOR TEST - 4 QUERIES\n");
    printf("═══════════════════════════════════════════════════════\n\n");

    for (int i = 0; i < 4; i++)
    {
        printf("═══════════════════════════════════════════════════════\n");
        printf("Query %d: %s\n", i + 1, queries[i]);
        printf("═══════════════════════════════════════════════════════\n\n");

        TokenList *tokens = tokenize(queries[i]);
        ParserResult *parse_result = parse(tokens);

        if (parse_result->success)
        {
            print_parsed_query(parse_result);

            // Execute the parsed query
            QueryResult *exec_result = execute_query(db, parse_result);
            query_result_display(exec_result, &parse_result->parsed_query.query.select);
            query_result_free(exec_result);

            // Repeat the same query to see if cache is used
            printf("\n═══════════════════════════════════════════════════════\n");
            printf("  Query %d (repeated): %s\n", i + 1, queries[i]);
            printf("═══════════════════════════════════════════════════════\n\n");
            QueryResult *exec_result2 = execute_query(db, parse_result);
            query_result_display(exec_result2, &parse_result->parsed_query.query.select);
            query_result_free(exec_result2);
        }
        else
        {
            printf("❌ Parser error: %s\n\n", parse_result->error_message);
        }

        parser_result_free(parse_result);
        token_list_free(tokens);
    }

    db_free(db);
}

// ============== EXAMPLE CALLBACK #1: AUDIT LOG ==============
typedef struct
{
    int total_inserts;
    int total_deletes;
    int total_updates;
} AuditStats;

void audit_log_insert(Person *record, void *user_data)
{
    AuditStats *stats = (AuditStats *)user_data;
    stats->total_inserts++;

    printf("  ✓ [AUDIT LOG] INSERT event\n");
    printf("    ID: %d, Name: %s, Age: %d, Salary: %.2f\n",
           record->id, record->name, record->age, record->salary);
    printf("    Total inserts so far: %d\n", stats->total_inserts);
}

// ============== EXAMPLE CALLBACK #2: VALIDATION ==============
void validation_check_insert(Person *record, void *user_data)
{
    printf("  ✓ [VALIDATION] Checking record...\n");

    if (record->age < 18)
        printf("  WARNING: Age is less than 18!\n");

    if (record->salary <= 0)
        printf(" WARNING: Invalid salary!");

    if (strlen(record->name) == 0)
        printf(" WARNING: Name is empty!\n");

    printf(" Validation check completed.\n");
}

// ============== EXAMPLE CALLBACK #3: EMAIL NOTIFICATION ==============
void email_notification_insert(Person *record, void *user_data)
{
    printf("  ✓ [EMAIL] Sending notification...\n");
    printf("    To: admin@database.com\n");
    printf("    Subject: New Employee Added\n");
    printf("    Body: %s (ID: %d) has been added to the system\n",
           record->name, record->id);
    printf("    Email sent!\n");
}

void test_callbacks()
{
    printf("\n=== Phase 11: Callbacks Testing ===\n\n");

    // Load database
    Database *db = db_load_from_file("people.db");
    if (db == NULL)
    {
        printf("Failed to load database\n");
        return;
    }

    // Create audit statistics
    AuditStats stats = {0, 0, 0};

    // Register callbacks
    printf("Registering callbacks...\n");
    callbacks_register_insert(db->callbacks, audit_log_insert, &stats);
    callbacks_register_insert(db->callbacks, validation_check_insert, NULL);
    callbacks_register_insert(db->callbacks, email_notification_insert, NULL);

    callbacks_display_stats(db->callbacks);

    // Now insert records - callbacks will run automatically
    printf("Inserting records (callbacks will trigger)...\n\n");

    Person test_people[] = {
        {100, "Alice", 28, 60000},
        {101, "Bob", 35, 75000},
        {102, "Charlie", 16, 40000}, // Invalid age!
    };

    for (int i = 0; i < 3; i++)
    {
        printf("─────────────────────────────────────\n");
        printf("Inserting: %s\n", test_people[i].name);
        printf("─────────────────────────────────────\n");
        db_add_record(db, test_people[i]);
    }

    // Show final statistics
    printf("\n=== Final Audit Statistics ===\n");
    printf("Total inserts: %d\n", stats.total_inserts);
    printf("Total deletes: %d\n", stats.total_deletes);
    printf("Total updates: %d\n\n", stats.total_updates);

    db_free(db);
}

// ============== PHASE 11 PART 2: VTABLES - POLYMORPHIC EXECUTORS ==============
void test_vtable_executors()
{
    printf("\n=== Phase 11 Part 2: Vtables (Polymorphic Query Executors) ===\n\n");

    // Load database
    Database *db = db_load_from_file("people.db");
    if (db == NULL)
    {
        printf("Failed to load database\n");
        return;
    }

    // Insert 15 sample records
    printf("Setting up database with sample records...\n\n");
    Person test_data[] = {
        {1, "Alice", 25, 50000},
        {2, "Bob", 28, 55000},
        {3, "Charlie", 32, 60000},
        {4, "Diana", 29, 58000},
        {5, "Eve", 31, 62000},
        {6, "Frank", 26, 52000},
        {7, "Grace", 34, 65000},
        {8, "Henry", 27, 54000},
        {9, "Iris", 33, 63000},
        {10, "Jack", 30, 59000},
        {11, "Karen", 35, 70000},
        {12, "Leo", 24, 48000},
        {13, "Mia", 38, 75000},
        {14, "Noah", 22, 45000},
        {15, "Olivia", 36, 68000}};

    for (int i = 0; i < 15; i++)
    {
        db_add_record(db, test_data[i]);
    }

    printf(" Database loaded with %d records\n\n", db->count);

    // Create three different executor strategies
    printf("═══════════════════════════════════════════════════════\n");
    printf("Creating 3 different query executor strategies...\n");
    printf("═══════════════════════════════════════════════════════\n\n");

    QueryExecutor *sequential_executor = executor_sequential_create(db);
    QueryExecutor *indexed_executor = executor_indexed_create(db);
    QueryExecutor *cached_executor = executor_cached_create(db);

    printf("✓ Sequential Executor created\n");
    printf("✓ Indexed Executor created\n");
    printf("✓ Cached Executor created\n\n");

    // Array of executors to test
    QueryExecutor *executors[] = {sequential_executor, indexed_executor, cached_executor};
    int num_executors = 3;

    // Create a dummy parser result (for demonstration purposes)
    ParserResult *parse_result = malloc(sizeof(ParserResult));
    parse_result->success = 1;
    parse_result->parsed_query.type = QUERY_SELECT;

    Person *results[100];
    int max_results = 100;

    // ============= TEST 1: STRATEGY COMPARISON =============
    printf("═══════════════════════════════════════════════════════\n");
    printf("TEST 1: Running query with each strategy\n");
    printf("═══════════════════════════════════════════════════════\n\n");

    for (int i = 0; i < num_executors; i++)
    {
        int result_count = executor_run(executors[i], parse_result, results, max_results);

        printf("Results from %s strategy:\n", executors[i]->name);
        for (int j = 0; j < result_count && j < 5; j++)
        {
            printf(" - %s (Age %d, Salary %.2f)\n", results[j]->name, results[j]->age, results[j]->salary);
        }
        if (result_count > 5)
            printf(" ... and %d more\n", result_count - 5);
        printf("\n");
    }

    // ============= TEST 2: CACHE EFFECTIVENESS =============
    printf("═══════════════════════════════════════════════════════\n");
    printf("TEST 2: Cache strategy efficiency (repeated query)\n");
    printf("═══════════════════════════════════════════════════════\n\n");

    printf("Query 1 (first time - will cache):\n");
    int count1 = executor_run(cached_executor, parse_result, results, max_results);

    printf("\nQuery 2 (second time - should hit cache):\n");
    int count2 = executor_run(cached_executor, parse_result, results, max_results);

    printf("Both queries returned %d results\n\n", count1);

    // ============= TEST 3: STRATEGY STATISTICS =============
    printf("═══════════════════════════════════════════════════════\n");
    printf("TEST 3: Performance statistics for each strategy\n");
    printf("═══════════════════════════════════════════════════════\n\n");

    for (int i = 0; i < num_executors; i++)
    {
        printf("Strategy: %s\n", executors[i]->name);
        executors[i]->display_stats(executors[i]);
        printf("\n");
    }

    // ============= TEST 4: POLYMORPHISM DEMONSTRATION =============
    printf("═══════════════════════════════════════════════════════\n");
    printf("TEST 4: Polymorphism demonstration\n");
    printf("═══════════════════════════════════════════════════════\n\n");

    printf("Notice: Same function works with any executor!\n\n");
    printf("Code pattern:\n");
    printf("  for (int i = 0; i < num_executors; i++) {\n");
    printf("      executor_run(executors[i], ...);\n");
    printf("  }\n\n");
    printf("This is polymorphism without classes!\n");
    printf("Each executor has different behavior (execute, display_stats, free)\n");
    printf("But all share the same interface (QueryExecutor vtable)\n\n");

    // ============= REAL-WORLD APPLICATION =============
    printf("═══════════════════════════════════════════════════════\n");
    printf("Real-world applications of Vtables:\n");
    printf("═══════════════════════════════════════════════════════\n\n");

    printf("1. SQLite:\n");
    printf("   - Different storage engines (B-tree, WAL, memory)\n");
    printf("   - All implement same interface\n");
    printf("   - Query optimizer chooses best engine\n\n");

    printf("2. Linux Kernel:\n");
    printf("   - File system drivers (ext4, NTFS, FAT, etc.)\n");
    printf("   - All implement same VFS interface\n");
    printf("   - Kernel dispatches through function pointers\n\n");

    printf("3. Game Engines:\n");
    printf("   - Entity types: Player, Enemy, NPC\n");
    printf("   - Each has update(), render(), destroy()\n");
    printf("   - Main loop calls through vtable\n\n");

    // Cleanup
    free(parse_result);
    sequential_executor->free_executor(sequential_executor);
    indexed_executor->free_executor(indexed_executor);
    cached_executor->free_executor(cached_executor);
    db_free(db);

    printf(" Vtable demonstration complete!\n\n");
}

// =============== PHASE 11 PART 3: MACROS (METAPROGRAMMING) ==============
void test_macros()
{
    printf("\n=== Phase 11 Part 3: Macros (Metaprogramming) ===\n\n");

    // ============= TEST 1: BASIC MACROS =============
    printf("═══════════════════════════════════════════════════════\n");
    printf("TEST 1: Basic Macros (MAX, MIN, Variadic)\n");
    printf("═══════════════════════════════════════════════════════\n\n");

    int a = 15, b = 23;
    printf("MAX(%d, %d) = %d\n", a, b, MAX(a, b));
    printf("MIN(%d, %d) = %d\n", a, b, MIN(a, b));

    printf("\nVariadic Macros:\n");
    PRINT_INFO("Starting macro tests");
    PRINT_DEBUG("Database connection established");
    PRINT_ERROR("Failed to load configuration");
    printf("\n");

    // ============= TEST 2: STRINGIFICATION & TOKEN PASTING =============
    printf("═══════════════════════════════════════════════════════\n");
    printf("TEST 2: Stringification and Token Pasting\n");
    printf("═══════════════════════════════════════════════════════\n\n");

    int age_field = 30;
    printf("Field name using STRINGIFY: %s\n", FIELD_NAME(age_field));

    printf("\nGenerating function names with CONCAT:\n");
    printf("  CONCAT(sort_, by_age) → sort_by_age\n");
    printf("  CONCAT(compare_, by_salary) → compare_by_salary\n");
    printf("  (These would be actual function names in code)\n\n");

    // ============= TEST 3: X-MACRO FIELD METADATA =============
    printf("═══════════════════════════════════════════════════════\n");
    printf("TEST 3: X-Macro Field Information\n");
    printf("═══════════════════════════════════════════════════════\n");

    macro_display_field_info();

    // ============= TEST 4: X-MACRO CODE GENERATION =============
    printf("\n═══════════════════════════════════════════════════════\n");
    printf("TEST 4: X-Macro Code Generation Example\n");
    printf("═══════════════════════════════════════════════════════\n");

    macro_generate_comparison_table();

    // ============= TEST 5: GENERATED PRINT FUNCTION =============
    printf("═══════════════════════════════════════════════════════\n");
    printf("TEST 5: Using Generated macro_print_person()\n");
    printf("═══════════════════════════════════════════════════════\n\n");

    Person example_person = {42, "John Doe", 28, 65000.50};
    macro_print_person(&example_person);

    // ============= TEST 6: GENERATED COMPARISON =============
    printf("\n═══════════════════════════════════════════════════════\n");
    printf("TEST 6: Using Generated macro_compare_persons()\n");
    printf("═══════════════════════════════════════════════════════\n\n");

    Person person1 = {1, "Alice", 30, 75000.00};
    Person person2 = {1, "Alice", 30, 75000.00};
    Person person3 = {2, "Bob", 25, 60000.00};

    printf("Comparing person1 (Alice) with person2 (Alice):\n");
    macro_compare_persons(&person1, &person2);

    printf("\nComparing person1 (Alice) with person3 (Bob):\n");
    macro_compare_persons(&person1, &person3);

    // ============= TEST 7: GENERATED SERIALIZATION =============
    printf("\n═══════════════════════════════════════════════════════\n");
    printf("TEST 7: Using Generated Serialization\n");
    printf("═══════════════════════════════════════════════════════\n\n");

    char buffer[256];
    macro_serialize_person(&example_person, buffer, sizeof(buffer));

    Person deserialized;
    macro_deserialize_person(buffer, &deserialized);
    printf("Deserialized result:\n");
    macro_print_person(&deserialized);

    // ============= TEST 8: GENERATED OPERATOR TABLE =============
    printf("\n═══════════════════════════════════════════════════════\n");
    printf("TEST 8: X-Macro Generated Operator Codes\n");
    printf("═══════════════════════════════════════════════════════\n");

    macro_display_operators();

    // ============= TEST 9: MACRO POWERS SUMMARY =============
    printf("═══════════════════════════════════════════════════════\n");
    printf("TEST 9: Why Macros Are Powerful\n");
    printf("═══════════════════════════════════════════════════════\n\n");

    printf("1. ZERO RUNTIME OVERHEAD\n");
    printf("   • Expanded at compile-time\n");
    printf("   • No function call cost\n");
    printf("   • Inlined automatically\n\n");

    printf("2. CODE GENERATION (X-Macros)\n");
    printf("   • Define structure ONCE\n");
    printf("   • Generate print, compare, serialize from same definition\n");
    printf("   • Add field? Update only one place!\n\n");

    printf("3. TYPE-AGNOSTIC OPERATIONS\n");
    printf("   • MAX(a, b) works for int, float, double, etc.\n");
    printf("   • No code duplication\n\n");

    printf("4. REAL-WORLD SYSTEMS\n");
    printf("   • Linux kernel: Driver registration, system calls\n");
    printf("   • SQLite: Query optimization, virtual tables\n");
    printf("   • Game engines: Entity systems, message dispatch\n\n");

    printf("5. METAPROGRAMMING (Compile-time programming)\n");
    printf("   • Write programs that write programs\n");
    printf("   • Reduce human error\n");
    printf("   • Ensure consistency across codebase\n\n");

    // ============= TEST 10: COMPARISON: WITH vs WITHOUT MACROS =============
    printf("═══════════════════════════════════════════════════════\n");
    printf("TEST 10: Without Macros vs With Macros\n");
    printf("═══════════════════════════════════════════════════════\n\n");

    printf("WITHOUT MACROS (Manual, error-prone):\n");
    printf("─────────────────────────────────────────────────────────\n");
    printf("void print_person(Person *p) {\n");
    printf("    printf(\"id: %%d\\\\n\", p->id);\n");
    printf("    printf(\"name: %%s\\\\n\", p->name);\n");
    printf("    printf(\"age: %%d\\\\n\", p->age);\n");
    printf("    printf(\"salary: %%.2f\\\\n\", p->salary);\n");
    printf("}\n\n");
    printf("void compare_persons(Person *a, Person *b) {\n");
    printf("    printf(\"id: %%s\", (a->id == b->id) ? \"OK\" : \"DIFF\");\n");
    printf("    printf(\"name: %%s\", (a->name == b->name) ? \"OK\" : \"DIFF\");\n");
    printf("    // ... and so on for each field ...\n");
    printf("}\n\n");
    printf("Problem: Add new field? Update EVERY function!\n\n");

    printf("WITH MACROS (DRY, maintainable):\n");
    printf("─────────────────────────────────────────────────────────\n");
    printf("#define PERSON_FIELDS \\\n");
    printf("    FIELD(id, int, \"%%d\") \\\n");
    printf("    FIELD(name, char*, \"%%s\") \\\n");
    printf("    FIELD(age, int, \"%%d\") \\\n");
    printf("    FIELD(salary, double, \"%%.2f\")\n\n");
    printf("void print_person(Person *p) {\n");
    printf("    #define FIELD(name, type, fmt) printf(fmt, p->name);\n");
    printf("    PERSON_FIELDS\n");
    printf("    #undef FIELD\n");
    printf("}\n\n");
    printf("Benefit: Add new field? Update only PERSON_FIELDS!\n\n");

    printf("✅ Phase 11 Part 3 Complete!\n\n");
}

// =============== PHASE 12: SYSTEMS PROGRAMMING & OS INTEGRATION ==============

// Example callbacks: Log file operations
typedef struct
{
    int open_count;
    int read_count;
    int write_count;
    int close_count;
} FileIOStats;

void on_file_opened(int fd, const char *filepath, void *user_data)
{
    FileIOStats *stats = (FileIOStats *)user_data;
    stats->read_count++;
    printf("     Callback: File opened (fd=%d)\n", fd);
}

void on_file_read(int fd, size_t bytes_read, void *user_data)
{
    FileIOStats *stats = (FileIOStats *)user_data;
    stats->read_count++;
    printf("     Callback: Read %zu bytes\n", bytes_read);
}

void on_file_written(int fd, size_t bytes_written, void *user_data)
{
    FileIOStats *stats = (FileIOStats *)user_data;
    stats->write_count++;
    printf("     Callback: Wrote %zu bytes\n", bytes_written);
}

void on_file_closed(int fd, void *user_data)
{
    FileIOStats *stats = (FileIOStats *)user_data;
    stats->close_count++;
    printf("     Callback: File closed (fd=%d)\n", fd);
}

void test_systems_programming()
{
    printf("\n=== Phase 12: Systems Programming & OS Integration ===\n\n");

    // ============= TEST 1: I/O STRATEGY COMPARISON =============
    printf("═══════════════════════════════════════════════════════\n");
    printf("TEST 1: Three I/O Strategies (Vtables from Phase 11)\n");
    printf("═══════════════════════════════════════════════════════\n\n");

    printf("Strategy Pattern: Different I/O backends, same interface\n\n");

    // Create callbacks
    FileIOStats stats = {0, 0, 0, 0};
    IOCallbacks callbacks = {
        .on_open = on_file_opened,
        .on_read = on_file_read,
        .on_write = on_file_written,
        .on_close = on_file_closed,
        .user_data = &stats};

    // Create three strategies
    IOStrategy *standard = io_standard_create(callbacks);
    IOStrategy *buffered = io_buffered_create(callbacks);
    IOStrategy *mmap = io_mmap_create(callbacks);

    printf("Created 3 I/O strategies:\n");
    printf("  1. Standard I/O (direct system calls)\n");
    printf("  2. Buffered I/O (internal buffering)\n");
    printf("  3. Memory-Mapped I/O (advanced)\n\n");

    // ============= TEST 2: WRITE TEST WITH DIFFERENT STRATEGIES =============
    printf("═══════════════════════════════════════════════════════\n");
    printf("TEST 2: Writing to file with each strategy\n");
    printf("═══════════════════════════════════════════════════════\n\n");

    const char *test_data = "Hello from Phase 12: Systems Programming!\n";

    // Test 1: Standard I/O
    printf("--- Strategy 1: Standard I/O ---\n");
    int fd1 = standard->open(standard, "test_standard.txt",
#ifdef _WIN32
                             _O_CREAT | _O_TRUNC | _O_WRONLY
#else
                             O_CREAT | O_TRUNC | O_WRONLY
#endif
    );
    if (fd1 >= 0)
    {
        standard->write(standard, fd1, test_data, strlen(test_data));
        standard->close(standard, fd1);
    }
    printf("\n");

    // Test 2: Buffered I/O
    printf("--- Strategy 2: Buffered I/O ---\n");
    int fd2 = buffered->open(buffered, "test_buffered.txt",
#ifdef _WIN32
                             _O_CREAT | _O_TRUNC | _O_WRONLY
#else
                             O_CREAT | O_TRUNC | O_WRONLY
#endif
    );
    if (fd2 >= 0)
    {
        // Write multiple times to show buffering
        for (int i = 0; i < 3; i++)
        {
            buffered->write(buffered, fd2, test_data, strlen(test_data));
        }
        buffered->close(buffered, fd2);
    }
    printf("\n");

    // Test 3: Memory-Mapped I/O
    printf("--- Strategy 3: Memory-Mapped I/O ---\n");
    int fd3 = mmap->open(mmap, "test_mmap.txt",
#ifdef _WIN32
                         _O_CREAT | _O_WRONLY | _O_TRUNC
#else
                         O_CREAT | O_WRONLY | O_TRUNC
#endif
    );
    if (fd3 >= 0)
    {
        mmap->write(mmap, fd3, test_data, strlen(test_data));
        mmap->close(mmap, fd3);
    }
    printf("\n");

    // ============= TEST 3: CALLBACK STATISTICS =============
    printf("═══════════════════════════════════════════════════════\n");
    printf("TEST 3: Callback Statistics (Events from Phase 11)\n");
    printf("═══════════════════════════════════════════════════════\n\n");

    printf("Callbacks fired during I/O operations:\n");
    printf("  Opens:  %d\n", stats.open_count);
    printf("  Reads:  %d\n", stats.read_count);
    printf("  Writes: %d\n", stats.write_count);
    printf("  Closes: %d\n\n", stats.close_count);

    // ============= TEST 4: STRATEGY STATISTICS =============
    printf("═══════════════════════════════════════════════════════\n");
    printf("TEST 4: I/O Strategy Performance Comparison\n");
    printf("═══════════════════════════════════════════════════════\n");

    standard->display_stats(standard);
    buffered->display_stats(buffered);
    mmap->display_stats(mmap);

    // ============= TEST 5: ERROR HANDLING WITH MACROS =============
    printf("═══════════════════════════════════════════════════════\n");
    printf("TEST 5: Error Handling Macros (Phase 11 Part 3)\n");
    printf("═══════════════════════════════════════════════════════\n\n");

    printf("Macros used in Phase 12:\n");
    printf("  • SYS_CALL(call) - Wrap system calls with error checking\n");
    printf("  • HANDLE_OPEN_ERROR(fd) - Check file open results\n");
    printf("  • HANDLE_READ_ERROR(bytes) - Check read operations\n");
    printf("  • HANDLE_WRITE_ERROR(bytes) - Check write operations\n\n");

    printf("Example usage in sys_io_strategies.c:\n");
    printf("  int fd = open(path, flags);\n");
    printf("  HANDLE_OPEN_ERROR(fd);  // Macro checks if fd < 0\n\n");

    // ============= TEST 6: PHASE INTEGRATION SUMMARY =============
    printf("═══════════════════════════════════════════════════════\n");
    printf("TEST 6: Integration of All Phases\n");
    printf("═══════════════════════════════════════════════════════\n\n");

    printf("Phase 12 integrates previous learning:\n\n");

    printf("✓ Phase 11 Part 1 (Callbacks):\n");
    printf("    Events: on_file_open, on_file_read, on_file_write, on_file_close\n");
    printf("    Triggered during I/O operations\n");
    printf("    User data passed to callbacks\n\n");

    printf("✓ Phase 11 Part 2 (Vtables):\n");
    printf("    IOStrategy struct with function pointers\n");
    printf("    3 implementations: Standard, Buffered, Memory-Mapped\n");
    printf("    Runtime polymorphism for I/O strategies\n\n");

    printf("✓ Phase 11 Part 3 (Macros):\n");
    printf("    SYS_CALL() - Error handling macro\n");
    printf("    HANDLE_*_ERROR() - Wrapper macros\n");
    printf("    Preprocessor safety checks\n\n");

    printf("✓ Phase 1-9 (Database System):\n");
    printf("    File persistence for database records\n");
    printf("    Direct system calls for file I/O\n\n");

    printf("✓ Phase 10 (SQL):\n");
    printf("    SQL queries executed on persistent database\n");
    printf("    Results written to disk via Phase 12 I/O\n\n");

    // ============= TEST 7: REAL-WORLD APPLICATION =============
    printf("═══════════════════════════════════════════════════════\n");
    printf("TEST 7: Real-World Application: Save Database to Disk\n");
    printf("═══════════════════════════════════════════════════════\n\n");

    printf("Use case: Persist in-memory database to disk\n");
    printf("Architecture:\n");
    printf("  1. Load database from disk (Phase 1-9)\n");
    printf("  2. Process SQL queries (Phase 10)\n");
    printf("  3. Register callbacks for save operations (Phase 11 Part 1)\n");
    printf("  4. Choose I/O strategy at runtime (Phase 11 Part 2)\n");
    printf("  5. Use system calls with error handling (Phase 12)\n\n");

    printf("Example flow:\n");
    printf("  Database db = db_load_from_file(\"people.db\");\n");
    printf("  IOStrategy strategy = io_buffered_create(callbacks);\n");
    printf("  strategy->open(strategy, \"people_backup.db\", ...);\n");
    printf("  for each record: strategy->write(strategy, fd, record, ...);\n");
    printf("  strategy->close(strategy, fd);\n\n");

    printf("Benefits:\n");
    printf("  • Transparent I/O (switch strategies without code changes)\n");
    printf("  • Event-driven (callbacks log/audit I/O operations)\n");
    printf("  • Safe (error macros catch failures)\n");
    printf("  • Portable (Windows/Linux compatibility)\n\n");

    // ============= TEST 8: NEXT STEPS =============
    printf("═══════════════════════════════════════════════════════\n");
    printf("TEST 8: Phase 12 Extensions (Not implemented)\n");
    printf("═══════════════════════════════════════════════════════\n\n");

    printf("What could be added to Phase 12:\n\n");

    printf("1. Multi-Process Database (fork, IPC):\n");
    printf("   • Use fork() to create reader/writer processes\n");
    printf("   • Implement file locking for concurrent access\n");
    printf("   • Use pipes for inter-process communication\n\n");

    printf("2. Signal Handling:\n");
    printf("   • SIGTERM for graceful shutdown\n");
    printf("   • SIGUSR1 for cache flush signals\n");
    printf("   • Signal handlers with callbacks\n\n");

    printf("3. Advanced Features:\n");
    printf("   • fcntl() for file locking\n");
    printf("   • stat() for file metadata\n");
    printf("   • dirfd() for directory operations\n");
    printf("   • Full mmap() implementation for large files\n\n");

    // Cleanup
    standard->free_strategy(standard);
    buffered->free_strategy(buffered);
    mmap->free_strategy(mmap);

    printf("✅ Phase 12 Complete!\n\n");
}

// =============== PHASE 13: NETWORK PROGRAMMING & DISTRIBUTED DATABASES ==============

// Network callbacks
typedef struct
{
    int clients_connected;
    int queries_received;
    int responses_sent;
} NetworkStats;

void on_client_connected(int client_id, const char *client_addr, void *user_data)
{
    NetworkStats *stats = (NetworkStats *)user_data;
    stats->clients_connected++;
    printf("     Callback: Client connected (id=%d, addr=%s)\n", client_id, client_addr);
}

void on_query_received(int client_id, const char *query, void *user_data)
{
    NetworkStats *stats = (NetworkStats *)user_data;
    stats->queries_received++;
    printf("     Callback: Query received from client %d: \"%s\"\n", client_id, query);
}

void on_response_sent(int client_id, int record_count, void *user_data)
{
    NetworkStats *stats = (NetworkStats *)user_data;
    stats->responses_sent++;
    printf("     Callback: Response sent to client %d (records=%d)\n", client_id, record_count);
}

void on_client_disconnected(int client_id, void *user_data)
{
    printf("     Callback: Client disconnected (id=%d)\n", client_id);
}

void test_network_programming()
{
    printf("\n=== Phase 13: Network Programming & Distributed Databases ===\n\n");

    // ============= TEST 1: PROTOCOL SERIALIZATION =============
    printf("═══════════════════════════════════════════════════════\n");
    printf("TEST 1: Message Serialization\n");
    printf("═══════════════════════════════════════════════════════\n\n");

    Person test_person = {42, "Alice Johnson", 28, 75000.50};

    printf("Original Person:\n");
    printf("  ID:     %d\n", test_person.id);
    printf("  Name:   %s\n", test_person.name);
    printf("  Age:    %d\n", test_person.age);
    printf("  Salary: %.2f\n\n", test_person.salary);

    // Serialize
    char buffer[256];
    int bytes = serialize_person(&test_person, buffer, sizeof(buffer));
    printf("Serialized Person (bytes=%d):\n", bytes);

    // Deserialize
    Person deserialized;
    deserialize_person(buffer, bytes, &deserialized);
    printf("\nDeserialized Person:\n");
    printf("  ID:     %d\n", deserialized.id);
    printf("  Name:   %s\n", deserialized.name);
    printf("  Age:    %d\n", deserialized.age);
    printf("  Salary: %.2f\n\n", deserialized.salary);

    // ============= TEST 2: QUERY REQUEST/RESPONSE PROTOCOL =============
    printf("═══════════════════════════════════════════════════════\n");
    printf("TEST 2: Query Request/Response Protocol\n");
    printf("═══════════════════════════════════════════════════════\n\n");

    QueryRequest req;
    req.header.version = PROTOCOL_VERSION;
    req.header.type = MSG_QUERY_REQUEST;
    req.header.payload_size = strlen("SELECT * FROM people WHERE age > 30");
    strcpy(req.query, "SELECT * FROM people WHERE age > 30");

    printf("Query Request:\n");
    printf("  Type:    MSG_QUERY_REQUEST (%d)\n", MSG_QUERY_REQUEST);
    printf("  Query:   %s\n\n", req.query);

    QueryResponse resp;
    resp.header.version = PROTOCOL_VERSION;
    resp.header.type = MSG_QUERY_RESPONSE;
    resp.header.record_count = 2;
    resp.records[0] = (Person){1, "Alice", 32, 80000};
    resp.records[1] = (Person){2, "Bob", 35, 85000};

    printf("Query Response:\n");
    printf("  Type:          MSG_QUERY_RESPONSE (%d)\n", MSG_QUERY_RESPONSE);
    printf("  Record count:  %d\n", resp.header.record_count);
    for (int i = 0; i < resp.header.record_count; i++)
    {
        printf("    [%d] %s (Age %d)\n", i + 1, resp.records[i].name, resp.records[i].age);
    }
    printf("\n");

    // ============= TEST 3: TCP SERVER (VTables) =============
    printf("═══════════════════════════════════════════════════════\n");
    printf("TEST 3: TCP Network Server (Vtables from Phase 11)\n");
    printf("═══════════════════════════════════════════════════════\n\n");

    // Load database
    Database *db = db_load_from_file("people.db");
    if (db == NULL)
    {
        printf("Failed to load database\n");
        return;
    }

    // Insert sample records
    Person records[] = {
        {1, "Alice", 28, 60000},
        {2, "Bob", 35, 75000},
        {3, "Charlie", 32, 70000}};
    for (int i = 0; i < 3; i++)
        db_add_record(db, records[i]);

    // Create network callbacks
    NetworkStats stats = {0, 0, 0};
    NetworkCallbacks callbacks = {
        .on_connected = on_client_connected,
        .on_query_received = on_query_received,
        .on_response_sent = on_response_sent,
        .on_disconnected = on_client_disconnected,
        .user_data = &stats};

    // Create TCP server (Vtable pattern)
    NetworkServer *server = net_server_tcp_create(db, 8080, callbacks);
    printf("Created TCP Server (Vtable polymorphism)\n");
    printf("  Strategy: %s\n", server->name);
    printf("  Port:     %d\n\n", server->port);

    // Start server
    net_server_start(server);

    // Simulate client connections
    printf("Simulating client connections...\n");
    printf("--- Client 1 ---\n");
    int client1 = server->accept_client(server);
    server->handle_client_query(server, client1, "SELECT * FROM people WHERE age > 30");

    QueryResponse qr1;
    qr1.header.record_count = 2;
    qr1.records[0] = records[1]; // Bob
    qr1.records[1] = records[2]; // Charlie
    server->send_response(server, client1, &qr1);

    printf("\n--- Client 2 ---\n");
    int client2 = server->accept_client(server);
    server->handle_client_query(server, client2, "SELECT * FROM people WHERE age > 30");

    QueryResponse qr2;
    qr2.header.record_count = 3;
    for (int i = 0; i < 3; i++)
        qr2.records[i] = records[i];
    server->send_response(server, client2, &qr2);

    printf("\n");

    // ============= TEST 4: TCP CLIENT =============
    printf("═══════════════════════════════════════════════════════\n");
    printf("TEST 4: TCP Network Client (Error Macros from Phase 11)\n");
    printf("═══════════════════════════════════════════════════════\n\n");

    NetworkClient *client = net_client_create("127.0.0.1", 8080);
    printf("Created TCP Client\n");
    printf("   Server: %s:%d\n\n", client->server_host, client->server_port);

    net_client_connect(client);
    net_client_send_query(client, "SELECT * FROM people WHERE age > 30");

    QueryResponse client_resp;
    client_resp.header.record_count = 1;
    client_resp.records[0] = (Person){2, "Bob", 35, 75000};
    net_client_receive_response(client, &client_resp);

    net_client_disconnect(client);

    // ============= TEST 5: CALLBACK STATISTICS =============
    printf("═══════════════════════════════════════════════════════\n");
    printf("TEST 5: Network Callbacks (Events from Phase 11)\n");
    printf("═══════════════════════════════════════════════════════\n\n");

    printf("Network Events Triggered:\n");
    printf("  Clients connected:   %d\n", stats.clients_connected);
    printf("  Queries received:    %d\n", stats.queries_received);
    printf("  Responses sent:      %d\n\n", stats.responses_sent);

    // ============= TEST 6: STATISTICS =============
    printf("═══════════════════════════════════════════════════════\n");
    printf("TEST 6: Server and Client Statistics\n");
    printf("═══════════════════════════════════════════════════════\n");

    server->display_stats(server);
    net_client_display_stats(client);

    // ============= TEST 7: PHASE 13 ARCHITECTURE =============
    printf("═══════════════════════════════════════════════════════\n");
    printf("TEST 7: Phase 13 Architecture & Integration\n");
    printf("═══════════════════════════════════════════════════════\n\n");

    printf("Complete Database System Flow:\n\n");

    printf("1. CLIENT LAYER:\n");
    printf("   • User enters SQL query\n");
    printf("   • Client serializes QueryRequest\n");
    printf("   • Sends over TCP socket\n\n");

    printf("2. NETWORK LAYER (Phase 13):\n");
    printf("   • TCP packets transmitted\n");
    printf("   • Error handling with macros\n");
    printf("   • Network callbacks fired\n\n");

    printf("3. SERVER LAYER:\n");
    printf("   • Receives and deserializes request\n");
    printf("   • Routes to database engine\n\n");

    printf("4. DATABASE LAYER (Phases 1-9):\n");
    printf("   • Phase 1-9: In-memory database\n");
    printf("   • B-trees, Hash tables, Cache\n");
    printf("   • Threading, Batch operations\n\n");

    printf("5. SQL LAYER (Phase 10):\n");
    printf("   • Tokenize SQL query\n");
    printf("   • Parse into AST\n");
    printf("   • Execute against database\n\n");

    printf("6. ADVANCED FEATURES (Phase 11):\n");
    printf("   • Callbacks: Events on query completion\n");
    printf("   • Vtables: Multiple server strategies\n");
    printf("   • Macros: Error handling & logging\n\n");

    printf("7. PERSISTENCE (Phase 12):\n");
    printf("   • Save results to disk\n");
    printf("   • Buffered or memory-mapped I/O\n");
    printf("   • File locking for concurrency\n\n");

    printf("8. RESPONSE:\n");
    printf("   • Server serializes response\n");
    printf("   • Sends results over TCP\n");
    printf("   • Client receives and deserializes\n");
    printf("   • User sees results\n\n");

    // ============= TEST 8: DISTRIBUTED SYSTEMS CONCEPT =============
    printf("═══════════════════════════════════════════════════════\n");
    printf("TEST 8: Distributed System Capabilities\n");
    printf("═══════════════════════════════════════════════════════\n\n");

    printf("With Phase 13, your database now supports:\n\n");

    printf("✓ CLIENT-SERVER ARCHITECTURE:\n");
    printf("  • Multiple clients query same database server\n");
    printf("  • Server maintains state\n");
    printf("  • Centralized data\n\n");

    printf("✓ REMOTE QUERY EXECUTION:\n");
    printf("  • Execute SQL queries over network\n");
    printf("  • Full Phase 10 SQL support\n");
    printf("  • Distributed computation\n\n");

    printf("✓ NETWORK REPLICATION:\n");
    printf("  • Write to primary server\n");
    printf("  • Replicate to backup servers\n");
    printf("  • High availability\n\n");

    printf("✓ LOAD BALANCING:\n");
    printf("  • Multiple servers, same database\n");
    printf("  • Distribute queries\n");
    printf("  • Scalability\n\n");

    printf("✓ REAL-TIME SYNCHRONIZATION:\n");
    printf("  • Callbacks trigger on events\n");
    printf("  • Clients notified of changes\n");
    printf("  • Event-driven architecture\n\n");

    // Cleanup
    net_server_stop(server);
    server->free_server(server);
    net_client_free(client);
    db_free(db);

    printf("✅ Phase 13 Complete!\n");
    printf("✅ All 13 Phases Complete!\n\n");

    printf("═══════════════════════════════════════════════════════\n");
    printf("CONGRATULATIONS: Full C Database System Built!\n");
    printf("═══════════════════════════════════════════════════════\n\n");

    printf("You've mastered:\n");
    printf("  • Low-level C (Phases 1-9)\n");
    printf("  • SQL Processing (Phase 10)\n");
    printf("  • Advanced C Patterns (Phase 11)\n");
    printf("  • Systems Programming (Phase 12)\n");
    printf("  • Network Programming (Phase 13)\n\n");

    printf("Next steps:\n");
    printf("  • Implement full socket programming\n");
    printf("  • Add persistent replication\n");
    printf("  • Build multi-node clusters\n");
    printf("  • Production optimization\n\n");
}

int main(int argc, char *argv[])
{
    // Load database from file
    Database *db = db_load_from_file("people.db");

    if (db == NULL)
    {
        printf("Failed to create database!\n");
        return 1;
    }

    // Phase 8: Route by argc
    // If argc == 1 (no arguments), run interactive mode
    // If argc > 1 (arguments provided), run CLI mode
    if (argc > 1)
    {
        // CLI mode: Parse command and dispatch
        run_cli_mode(argc, argv, db);
        db_free(db);
        return 0;
    }

    // Interactive mode: Run menu loop
    int choice;

    while (1)
    {
        show_menu();

        if (scanf("%d", &choice) != 1)
        {
            printf("Invalid input. Try again.\n");
            while (getchar() != '\n')
                ;
            continue;
        }
        getchar();

        if (choice == 1)
        {
            // Display all records
            db_display(db);
        }
        else if (choice == 2)
        {
            // Sort by age
            db_sort_by_age(db);
            db_display(db);
        }
        else if (choice == 3)
        {
            // Sort by name
            db_sort_by_name(db);
            db_display(db);
        }
        else if (choice == 4)
        {
            // Sort by salary (descending)
            db_sort_by_salary_desc(db);
            db_display(db);
        }
        else if (choice == 5)
        {
            // Search by name
            char name[100];
            printf("Enter name to search: ");
            fgets(name, sizeof(name), stdin);
            name[strcspn(name, "\n")] = '\0';

            int idx = db_search_by_name(db, name);
            if (idx >= 0)
            {
                printf("Details: Age=%d, Salary=%.2f\n",
                       db->records[idx].age, db->records[idx].salary);
            }
        }
        else if (choice == 6)
        {
            // Filter by age
            int min_age;
            printf("Enter minimum age: ");
            scanf("%d", &min_age);
            getchar();

            Person *filtered = malloc(sizeof(Person) * db->count);
            if (filtered == NULL)
            {
                printf("Memory allocation failed!\n");
                continue;
            }

            int filtered_count = db_filter_by_age_gt(db, min_age, filtered);
            printf("\nFound %d people older than %d:\n", filtered_count, min_age);
            for (int i = 0; i < filtered_count; i++)
            {
                printf("  %s, Age=%d, Salary=%.2f\n",
                       filtered[i].name, filtered[i].age, filtered[i].salary);
            }
            free(filtered);
        }
        else if (choice == 7)
        {
            // Binary search by ID
            int target_id;
            printf("Enter ID to search: ");
            scanf("%d", &target_id);
            getchar();

            int idx = db_binary_search_by_id(db, target_id);
            if (idx >= 0)
            {
                printf("Found: %s, Age=%d, Salary=%.2f\n",
                       db->records[idx].name, db->records[idx].age, db->records[idx].salary);
            }
        }
        else if (choice == 8)
        {
            // Manual comparison of two records
            if (db->count < 2)
            {
                printf("Need at least 2 records to compare.\n");
                continue;
            }

            int idx1, idx2;
            printf("Enter first record index (0-%d): ", db->count - 1);
            scanf("%d", &idx1);
            printf("Enter second record index (0-%d): ", db->count - 1);
            scanf("%d", &idx2);
            getchar();

            if (idx1 < 0 || idx1 >= db->count || idx2 < 0 || idx2 >= db->count)
            {
                printf("Invalid indices!\n");
                continue;
            }

            int cmp = compare_by_age(&db->records[idx1], &db->records[idx2]);
            printf("\nCompare %s (age %d) vs %s (age %d):\n",
                   db->records[idx1].name, db->records[idx1].age,
                   db->records[idx2].name, db->records[idx2].age);
            printf("Result: %s\n",
                   cmp < 0 ? "First is younger" : cmp > 0 ? "First is older"
                                                          : "Same age");
        }
        else if (choice == 9)
        {
            db_sort_by_id(db);
            db_display(db);
        }
        else if (choice == 10)
        {
            compare_search_performance(db);
        }
        else if (choice == 11)
        {
            // Add new record
            Person new_person = {0};
            new_person.id = db->count + 1;

            printf("Enter name: ");
            fgets(new_person.name, sizeof(new_person.name), stdin);
            new_person.name[strcspn(new_person.name, "\n")] = '\0';

            printf("Enter age: ");
            scanf("%d", &new_person.age);

            printf("Enter salary: ");
            scanf("%lf", &new_person.salary);
            getchar();

            if (db_add_record(db, new_person))
            {
                printf("Record added successfully!\n");
                db_memory_stats(db);
            }
        }
        else if (choice == 12)
        {
            // Show memory stats
            db_memory_stats(db);
        }
        else if (choice == 13)
        {
            test_hash_table(db);
        }
        else if (choice == 14)
        {
            test_btree(db);
        }
        else if (choice == 15)
        {
            test_query_cache(db);
        }
        else if (choice == 16)
        {
            test_batch_operations(db);
        }
        else if (choice == 17)
        {
            test_threading(db);
        }
        else if (choice == 18)
        {
            test_threadpool();
        }
        else if (choice == 19)
        {
            benchmark_run_all(db);
        }
        else if (choice == 20)
        {
            test_replication(db);
        }
        else if (choice == 21)
        {
            test_tokenizer();
        }
        else if (choice == 22)
        {
            test_parser();
        }
        else if (choice == 23)
        {
            test_callbacks();
        }
        else if (choice == 24)
        {
            test_vtable_executors();
        }
        else if (choice == 25)
        {
            test_macros();
        }
        else if (choice == 26)
        {
            test_systems_programming();
        }
        else if (choice == 27)
        {
            test_network_programming();
        }
        else if (choice == 28)
        {
            printf("Goodbye!\n");
            break;
        }
        else
        {
            printf("Invalid choice. Try again.\n");
        }
    }

    // Cleanup
    db_free(db);
    return 0;
}
