#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "database.h"
#include "io.h"
#include "query.h"

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
  printf("13. Quit\n");
  printf("Selection: ");
}

int main(void)
{
  // Load database from file
  Database *db = db_load_from_file("people.db");

  if (db == NULL)
  {
    printf("Failed to create database!\n");
    return 1;
  }

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
