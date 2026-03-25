#define _CRT_SECURE_NO_WARNINGS
#include "../../include/query.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>

static void to_lower_copy(char *dest, const char *src, size_t size) {
    size_t i = 0;
    for (; i + 1 < size && src[i] != '\0'; i++) {
        dest[i] = (char)tolower((unsigned char)src[i]);
    }
    dest[i] = '\0';
}

static int contains_ignore_case(const char *text, const char *pattern) {
    char lower_text[NAME_SIZE];
    char lower_pattern[NAME_SIZE];

    if (!text || !pattern) {
        return 0;
    }

    to_lower_copy(lower_text, text, sizeof(lower_text));
    to_lower_copy(lower_pattern, pattern, sizeof(lower_pattern));

    return strstr(lower_text, lower_pattern) != NULL;
}

Person *database_find_by_name(const Database *db, const char *name) {
    if (!db || !name) {
        return NULL;
    }

    for (int i = 0; i < db->count; i++) {
        if (_stricmp(db->records[i].name, name) == 0) {
            return &db->records[i];
        }
    }

    return NULL;
}

void database_find_all_by_name(const Database *db, const char *name) {
    if (!db || !name) {
        return;
    }

    int found_any = 0;

    printf("\n%-6s %-20s %-6s %-12s\n", "ID", "Name", "Age", "Salary");
    printf("------------------------------------------------------\n");

    for (int i = 0; i < db->count; i++) {
        if (contains_ignore_case(db->records[i].name, name)) {
            const Person *p = &db->records[i];
            printf("%-6d %-20s %-6d %-12.2f\n", p->id, p->name, p->age, p->salary);
            found_any = 1;
        }
    }

    if (!found_any) {
        printf("No matching people found for name fragment '%s'\n", name);
    }
}

void database_find_by_age_range(const Database *db, int min_age, int max_age) {
    if (!db) {
        return;
    }

    if (min_age > max_age) {
        int temp = min_age;
        min_age = max_age;
        max_age = temp;
    }

    int found_any = 0;

    printf("\nPeople with age between %d and %d:\n", min_age, max_age);
    printf("\n%-6s %-20s %-6s %-12s\n", "ID", "Name", "Age", "Salary");
    printf("------------------------------------------------------\n");

    for (int i = 0; i < db->count; i++) {
        const Person *person = &db->records[i];
        if (person->age >= min_age && person->age <= max_age) {
            printf("%-6d %-20s %-6d %-12.2f\n",
                   person->id, person->name, person->age, person->salary);
            found_any = 1;
        }
    }

    if (!found_any) {
        printf("No matching people found in that age range\n");
    }
}

void database_find_by_salary_range(const Database *db, double min_salary, double max_salary) {
    if (!db) {
        return;
    }

    if (min_salary > max_salary) {
        double temp = min_salary;
        min_salary = max_salary;
        max_salary = temp;
    }

    int found_any = 0;

    printf("\nPeople with salary between %.2f and %.2f:\n", min_salary, max_salary);
    printf("\n%-6s %-20s %-6s %-12s\n", "ID", "Name", "Age", "Salary");
    printf("------------------------------------------------------\n");

    for (int i = 0; i < db->count; i++) {
        const Person *person = &db->records[i];
        if (person->salary >= min_salary && person->salary <= max_salary) {
            printf("%-6d %-20s %-6d %-12.2f\n",
                   person->id, person->name, person->age, person->salary);
            found_any = 1;
        }
    }

    if (!found_any) {
        printf("No matching people found in that salary range\n");
    }
}

static int compare_by_age(const void *a, const void *b) {
    const Person *pa = (const Person *)a;
    const Person *pb = (const Person *)b;

    if (pa->age < pb->age) return -1;
    if (pa->age > pb->age) return 1;
    return 0;
}

static int compare_by_salary(const void *a, const void *b) {
    const Person *pa = (const Person *)a;
    const Person *pb = (const Person *)b;

    if (pa->salary < pb->salary) return -1;
    if (pa->salary > pb->salary) return 1;
    return 0;
}

static int compare_by_name(const void *a, const void *b) {
    const Person *pa = (const Person *)a;
    const Person *pb = (const Person *)b;

    return _stricmp(pa->name, pb->name);
}

void database_sort_by_age(Database *db) {
    if (!db || db->count <= 1) {
        return;
    }

    qsort(db->records, db->count, sizeof(Person), compare_by_age);

}

void database_sort_by_salary(Database *db) {
    if (!db || db->count <= 1) {
        return;
    }

    qsort(db->records, db->count, sizeof(Person), compare_by_salary);
}

void database_sort_by_name(Database *db) {
    if (!db || db->count <= 1) {
        return;
    }

    qsort(db->records, db->count, sizeof(Person), compare_by_name);
}

Person *database_find_by_age(const Database *db, int age) {
    if (!db) {
        return NULL;
    }

    for (int i = 0; i < db->count; i++) {
        if (db->records[i].age == age) {
            return &db->records[i];
        }
    }

    return NULL;
}

Person *database_binary_search_by_age(const Database *db, int age) {
    if (!db || db->count <= 0) {
        return NULL;
    }

    int left = 0;
    int right = db->count - 1;

    while (left <= right) {
        int mid = left + (right - left) / 2;
        int mid_age = db->records[mid].age;

        if (mid_age == age) {
            while (mid > 0 && db->records[mid - 1].age == age) {
                mid--;
            }
            return &db->records[mid];
        }

        if (mid_age < age) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }

    return NULL;
}

void database_compare_search_by_age(const Database *db, int age) {
    if (!db || db->count <= 0) {
        printf("Database is empty\n");
        return;
    }

    clock_t linear_start = clock();
    Person *linear_result = database_find_by_age(db, age);
    clock_t linear_end = clock();

    Database temp = *db;
    temp.records = malloc((size_t)db->count * sizeof(Person));
    if (!temp.records) {
        printf("Failed to allocate memory for comparison copy\n");
        return;
    }

    memcpy(temp.records, db->records, (size_t)db->count * sizeof(Person));

    clock_t sort_start = clock();
    qsort(temp.records, db->count, sizeof(Person), compare_by_age);
    clock_t sort_end = clock();

    clock_t binary_start = clock();
    Person *binary_result = database_binary_search_by_age(&temp, age);
    clock_t binary_end = clock();

    double linear_time_ms = (double)(linear_end - linear_start) * 1000.0 / CLOCKS_PER_SEC;
    double sort_time_ms = (double)(sort_end - sort_start) * 1000.0 / CLOCKS_PER_SEC;
    double binary_time_ms = (double)(binary_end - binary_start) * 1000.0 / CLOCKS_PER_SEC;

    printf("\nSearch comparison for age %d\n", age);
    printf("Linear search: %.3f ms\n", linear_time_ms);
    if (linear_result) {
        printf("  | found ID %d, Name %s\n", linear_result->id, linear_result->name);
    } else {
        printf("  | not found\n");
    }

    printf("Sort copy: %.3f ms\n", sort_time_ms);

    printf("Binary search: %.3f ms\n", binary_time_ms);
    if (binary_result) {
        printf("  | found ID %d, Name %s\n", binary_result->id, binary_result->name);
    } else {
        printf("  | not found\n");
    }

    free(temp.records);
}