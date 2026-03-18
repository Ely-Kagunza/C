#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
  printf("25. Quit\n");
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

    BTree *bt = btree_create(3);  // Order 3: max 5 keys per node
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
        {6, "Frank", 26, 52000},  // This will trigger first split
        {7, "Grace", 34, 65000},
        {8, "Henry", 27, 54000},
        {9, "Iris", 33, 63000},
        {10, "Jack", 30, 59000}
    };

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
        {105, "Eve", 31, 62000}
    };

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
    HANDLE threads[5];  // Windows thread handles
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
            NULL,                      // Default security attributes
            0,                         // Default stack size
            worker_search_thread,      // Thread function
            &tasks[i],                 // Thread argument
            0,                         // Creation flags
            NULL                       // Thread ID (not needed)
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
    Sleep(500);  // Simulate work
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
        Sleep(100);  // Stagger submissions
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
        {203, "Repl3", 45, 75000}
    };

    for (int i = 0; i < 3; i++)
    {
        db_add_record(db, new_records[i]);
    }

    replication_display_stats(rm);

    printf("\nTEST 3: Detect changes (replication system reads primary)\n");
    replication_detect_changes(rm);  // System notices changes
    
    replication_display_stats(rm);

    printf("TEST 4: Apply changes to replica\n");
    replication_sync(rm);  // Replica replays log

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
        "SELECT * FROM people ORDER BY salary DESC LIMIT 10"
    };

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
        {20, "Tina", 37, 72000}
    };

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
typedef struct {
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
        {102, "Charlie", 16, 40000},  // Invalid age!
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
        {15, "Olivia", 36, 68000}
    };

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
