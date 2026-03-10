#ifndef QUERY_H
#define QUERY_H

#include "database.h"

// Search functions
int db_search_by_name(Database *db, const char *name);
int db_binary_search_by_id(Database *db, int target_id);

// Filter function
int db_filter_by_age_gt(Database *db, int min_age, Person *results);

// Comparison functions
int compare_by_age(const void *a, const void *b);
int compare_by_name(const void *a, const void *b);
int compare_by_salary_desc(const void *a, const void *b);
int compare_by_id(const void *a, const void *b);

// Sort functions
void db_sort_by_age(Database *db);
void db_sort_by_name(Database *db);
void db_sort_by_id(Database *db);
void db_sort_by_salary_desc(Database *db);

// Performance testing
void compare_search_performance(Database *db);

#endif
