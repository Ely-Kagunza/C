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
  printf("17. Quit\n");
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
    printf("\n=== Phase 9: B-Tree Testing ===\n\n");

    // Create B-tree indexed by age
    BTree *age_index = btree_create(3);
    if (age_index == NULL)
    {
        printf("Failed to create B-tree\n");
        return;
    }

    // Insert all records into B-tree (indexed by age)
    printf("Building B-tree index by age...\n");
    for (int i = 0; i < db->count; i++)
    {
        btree_insert(age_index, db->records[i].age, &db->records[i]);
    }
    btree_display_stats(age_index);

    // TEST 1: Exact search
    printf("TEST 1: Exact Age Search\n");
    Person *found = btree_search(age_index, 30);
    if (found != NULL)
        printf("✓ Found person age 30: %s\n\n", found->name);
    else
        printf("✗ No person with age 30\n\n");

    // TEST 2: Range search
    printf("TEST 2: Range Search (Age 25-35)\n");
    Person **range_results = malloc(sizeof(Person *) * db->count);
    if (range_results == NULL)
    {
        printf("Memory allocation failed\n");
        btree_free(age_index);
        return;
    }

    int range_count = btree_range_search(age_index, 25, 35, range_results);
    printf("Found %d people between age 25 and 35:\n", range_count);
    for (int i = 0; i < range_count; i++)
    {
        printf("  - %s, Age %d\n", range_results[i]->name, range_results[i]->age);
    }
    printf("\n");

    // TEST 3: Different range
    printf("TEST 3: Range Search (Age 20-28)\n");
    range_count = btree_range_search(age_index, 20, 28, range_results);
    printf("Found %d people between age 20 and 28:\n", range_count);
    for (int i = 0; i < range_count; i++)
    {
        printf("  - %s, Age %d\n", range_results[i]->name, range_results[i]->age);
    }
    printf("\n");

    free(range_results);
    btree_free(age_index);
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
