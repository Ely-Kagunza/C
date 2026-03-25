#ifndef QUERY_H
#define QUERY_H

#include "database.h"

Person *database_find_by_name(const Database *db, const char *name);
void database_find_all_by_name(const Database *db, const char *name);
void database_find_by_age_range(const Database *db, int min_age, int max_age);
void database_find_by_salary_range(const Database *db, double min_salary, double max_salary);

Person *database_find_by_age(const Database *db, int age);
Person *database_binary_search_by_age(const Database *db, int age);
void database_compare_search_by_age(const Database *db, int age);

void database_sort_by_age(Database *db);
void database_sort_by_salary(Database *db);
void database_sort_by_name(Database *db);

#endif