#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "query.h"

// Linear search: find first person with matching name
int db_search_by_name(Database *db, const char *name)
{
  for (int i = 0; i < db->count; i++)
  {
    if (strcmp(db->records[i].name, name) == 0)
    {
      printf("Found '%s' at index %d.\n", name, i);
      return i;
    }
  }
  printf("'%s' not found.\n", name);
  return -1;
}

// Linear search: find all persons with age greater than threshold
int db_filter_by_age_gt(Database *db, int min_age, Person *results)
{
  int count = 0;
  for (int i = 0; i < db->count; i++)
  {
    if (db->records[i].age > min_age)
    {
      results[count++] = db->records[i];
    }
  }
  return count;
}

// Binary search: find person ID (array MUST be sorted by ID!)
int db_binary_search_by_id(Database *db, int target_id)
{
  int left = 0;
  int right = db->count - 1;

  while (left <= right)
  {
    int mid = left + (right - left) / 2;
    int mid_id = db->records[mid].id;

    if (mid_id == target_id)
    {
      printf("Found ID %d at index %d.\n", target_id, mid);
      return mid;
    }
    else if (mid_id < target_id)
    {
      left = mid + 1;
    }
    else
    {
      right = mid - 1;
    }
  }

  printf("ID %d not found.\n", target_id);
  return -1;
}

// Compare two Person structs by age (ascending)
int compare_by_age(const void *a, const void *b)
{
  const Person *person_a = (const Person *)a;
  const Person *person_b = (const Person *)b;
  return person_a->age - person_b->age;
}

// Compare two Person structs by name (alphabetical)
int compare_by_name(const void *a, const void *b)
{
  const Person *person_a = (const Person *)a;
  const Person *person_b = (const Person *)b;
  return strcmp(person_a->name, person_b->name);
}

// Compare by salary (descending)
int compare_by_salary_desc(const void *a, const void *b)
{
  const Person *person_a = (const Person *)a;
  const Person *person_b = (const Person *)b;

  if (person_a->salary < person_b->salary)
    return 1;
  if (person_a->salary > person_b->salary)
    return -1;
  return 0;
}

// Compare by ID (ascending)
int compare_by_id(const void *a, const void *b)
{
  const Person *person_a = (const Person *)a;
  const Person *person_b = (const Person *)b;
  return person_a->id - person_b->id;
}

// Sort database by age (ascending)
void db_sort_by_age(Database *db)
{
  qsort(db->records, db->count, sizeof(Person), compare_by_age);
  printf("Database sorted by age.\n");
}

// Sort database by name (alphabetical)
void db_sort_by_name(Database *db)
{
  qsort(db->records, db->count, sizeof(Person), compare_by_name);
  printf("Database sorted by name.\n");
}

// Sort database by ID (ascending)
void db_sort_by_id(Database *db)
{
  qsort(db->records, db->count, sizeof(Person), compare_by_id);
  printf("Database sorted by ID.\n");
}

// Sort database by salary (descending)
void db_sort_by_salary_desc(Database *db)
{
  qsort(db->records, db->count, sizeof(Person), compare_by_salary_desc);
  printf("Database sorted by salary (descending).\n");
}

// Compare search performance: linear vs binary
void compare_search_performance(Database *db)
{
  if (db->count < 2)
  {
    printf("Need at least 2 records to compare.\n");
    return;
  }

  int target_id = db->records[db->count / 2].id;
  printf("Searching for ID %d...\n", target_id);
  printf("Database size: %d records\n", db->count);

  // Manual LINEAR search loop
  clock_t start_linear = clock();
  int linear_result = -1;
  for (int i = 0; i < db->count; i++)
  {
    if (db->records[i].id == target_id)
    {
      linear_result = i;
      break;
    }
  }
  clock_t end_linear = clock();
  double linear_time = ((double)(end_linear - start_linear)) / CLOCKS_PER_SEC * 1000000;

  // Binary search
  db_sort_by_id(db);
  clock_t start_binary = clock();
  int binary_result = db_binary_search_by_id(db, target_id);
  clock_t end_binary = clock();
  double binary_time = ((double)(end_binary - start_binary)) / CLOCKS_PER_SEC * 1000000;

  printf("\n=== Search Performance ===\n");
  printf("Linear Search (manual loop):  %.2f microseconds\n", linear_time);
  printf("Binary Search (sorted data):  %.2f microseconds\n", binary_time);
  if (binary_time > 0)
  {
    printf("Speed ratio: %.1fx faster\n", linear_time / binary_time);
  }
}
