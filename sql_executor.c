#include "sql_executor.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Build a query key for caching
static char *build_query_key(SelectQuery *query)
{
    char *key = malloc(1024);
    if (key == NULL) return NULL;
    
    sprintf(key, "SELECT:");
    for (int i = 0; i < query->column_count; i++)
        strcat(key, query->columns[i]);
    strcat(key, ":FROM:");
    strcat(key, query->table_name);
    
    if (query->condition_count > 0)
    {
        strcat(key, ":WHERE:");
        for (int i = 0; i < query->condition_count; i++)
        {
            strcat(key, query->conditions[i].column);
            strcat(key, query->conditions[i].op);
            strcat(key, query->conditions[i].value);
        }
    }
    
    return key;
}

// Evaluate a single WHERE condition
static int evaluate_condition(Person *person, WhereCondition *condition)
{
    int person_value = 0;
    double person_float = 0;
    int is_float = 0;

    // Get person's field value
    if (strcmp(condition->column, "age") == 0)
    {
        person_value = person->age;
    }
    else if (strcmp(condition->column, "id") == 0)
    {
        person_value = person->id;
    }
    else if (strcmp(condition->column, "salary") == 0)
    {
        person_float = person->salary;
        is_float = 1;
    }
    else
    {
        return 1;  // Unknown column = match
    }

    // Parse comparison value
    int compare_value = 0;
    double compare_float = 0;
    if (is_float)
        compare_float = atof(condition->value);
    else
        compare_value = atoi(condition->value);

    // Evaluate operator
    if (strcmp(condition->op, ">") == 0)
    {
        return is_float ? (person_float > compare_float) : (person_value > compare_value);
    }
    else if (strcmp(condition->op, "<") == 0)
    {
        return is_float ? (person_float < compare_float) : (person_value < compare_value);
    }
    else if (strcmp(condition->op, "=") == 0)
    {
        return is_float ? (person_float == compare_float) : (person_value == compare_value);
    }
    else if (strcmp(condition->op, "!=") == 0)
    {
        return is_float ? (person_float != compare_float) : (person_value != compare_value);
    }
    else if (strcmp(condition->op, "<=") == 0)
    {
        return is_float ? (person_float <= compare_float) : (person_value <= compare_value);
    }
    else if (strcmp(condition->op, ">=") == 0)
    {
        return is_float ? (person_float >= compare_float) : (person_value >= compare_value);
    }

    return 1;  // Default match
}

// Evaluate entire WHERE clause (handle AND/OR)
static int evaluate_where(Person *person, WhereCondition *conditions, int condition_count, int where_and)
{
    if (condition_count == 0)
        return 1;  // No WHERE = match all

    if (where_and)
    {
        // ALL conditions must be true
        for (int i = 0; i < condition_count; i++)
        {
            if (!evaluate_condition(person, &conditions[i]))
                return 0;
        }
        return 1;
    }
    else
    {
        // ANY condition must be true
        for (int i = 0; i < condition_count; i++)
        {
            if (evaluate_condition(person, &conditions[i]))
                return 1;
        }
        return 0;
    }
}

// Comparators for sorting Person* arrays
static int cmp_by_salary_asc(const void *a, const void *b)
{
    Person *p1 = *(Person**)a;
    Person *p2 = *(Person**)b;
    return (p1->salary > p2->salary) - (p1->salary < p2->salary);
}

static int cmp_by_salary_desc(const void *a, const void *b)
{
    Person *p1 = *(Person**)a;
    Person *p2 = *(Person**)b;
    return (p1->salary < p2->salary) - (p1->salary > p2->salary);
}

static int cmp_by_age_asc(const void *a, const void *b)
{
    Person *p1 = *(Person**)a;
    Person *p2 = *(Person**)b;
    return p1->age - p2->age;
}

static int cmp_by_age_desc(const void *a, const void *b)
{
    Person *p1 = *(Person**)a;
    Person *p2 = *(Person**)b;
    return p2->age - p1->age;
}

static int cmp_by_name_asc(const void *a, const void *b)
{
    Person *p1 = *(Person**)a;
    Person *p2 = *(Person**)b;
    return strcmp(p1->name, p2->name);
}

static int cmp_by_name_desc(const void *a, const void *b)
{
    Person *p1 = *(Person**)a;
    Person *p2 = *(Person**)b;
    return strcmp(p2->name, p1->name);
}

// Get comparator based on column and direction
typedef int (*ComparatorFunc)(const void*, const void*);

static ComparatorFunc get_comparator(const char *column, int descending)
{
    if (strcmp(column, "salary") == 0)
        return descending ? cmp_by_salary_desc : cmp_by_salary_asc;
    else if (strcmp(column, "age") == 0)
        return descending ? cmp_by_age_desc : cmp_by_age_asc;
    else if (strcmp(column, "name") == 0)
        return descending ? cmp_by_name_desc : cmp_by_name_asc;
    else
        return cmp_by_age_asc;  // Default
}

QueryResult *execute_query(Database *db, ParserResult *parsed_query)
{
    QueryResult *result = malloc(sizeof(QueryResult));
    result->records = NULL;
    result->count = 0;
    result->success = 0;
    result->error_message = NULL;

    // Only SELECT supported for Phase 10
    if (parsed_query->parsed_query.type != QUERY_SELECT)
    {
        result->error_message = malloc(256);
        strcpy(result->error_message, "Only SELECT queries supported in Phase 10");
        return result;
    }

    SelectQuery *query = &parsed_query->parsed_query.query.select;

    // =========== STEP 1: Check Cache ===========
    char *query_key = build_query_key(query);
    CachedQuery *cached = cache_lookup(db->query_cache, query_key);
    
    if (cached != NULL)
    {
        printf("[Cache HIT] Results retrieved from cache\n");
        result->records = malloc(sizeof(Person*) * cached->result_count);
        for (int i = 0; i < cached->result_count; i++)
            result->records[i] = cached->results[i];
        result->count = cached->result_count;
        result->success = 1;
        free(query_key);
        return result;
    }

    printf("[Cache MISS] Executing query...\n");

    // =========== STEP 2: Filter with WHERE ===========
    Person **filtered = malloc(sizeof(Person*) * db->count);
    int filtered_count = 0;

    for (int i = 0; i < db->count; i++)
    {
        if (evaluate_where(&db->records[i], query->conditions, query->condition_count, query->where_and))
        {
            filtered[filtered_count++] = &db->records[i];
        }
    }

    printf("WHERE clause: %d records matched (out of %d)\n", filtered_count, db->count);

    // =========== STEP 3: Sort with ORDER BY ===========
    if (query->has_order_by && filtered_count > 0)
    {
        ComparatorFunc comparator = get_comparator(query->order_by.column, query->order_by.descending);
        qsort(filtered, filtered_count, sizeof(Person*), comparator);
        
        printf("ORDER BY: sorted by %s %s\n", query->order_by.column,
               query->order_by.descending ? "DESC" : "ASC");
    }

    // =========== STEP 4: Apply LIMIT/OFFSET ===========
    int start = (query->offset >= 0) ? query->offset : 0;
    int end = filtered_count;
    if (query->limit >= 0)
        end = (start + query->limit < filtered_count) ? start + query->limit : filtered_count;

    int final_count = end - start;
    if (final_count < 0) final_count = 0;

    printf("LIMIT/OFFSET: returning records %d to %d (count: %d)\n", start, end - 1, final_count);

    // =========== STEP 5: Build Result ===========
    result->records = malloc(sizeof(Person*) * final_count);
    for (int i = 0; i < final_count; i++)
        result->records[i] = filtered[start + i];

    result->count = final_count;
    result->success = 1;

    // =========== STEP 6: Cache Results ===========
    cache_insert(db->query_cache, query_key, result->records, result->count);
    printf("[Cache] Results cached for future queries\n");

    free(filtered);
    free(query_key);

    return result;
}

void query_result_free(QueryResult *result)
{
    if (result == NULL) return;
    if (result->records != NULL) free(result->records);
    if (result->error_message != NULL) free(result->error_message);
    free(result);
}

void query_result_display(QueryResult *result, SelectQuery *query)
{
    if (result == NULL) return;

    if (!result->success)
    {
        printf("Query execution failed: %s\n\n", result->error_message);
        return;
    }

    printf("\nQuery executed successfully - Returned %d records\n\n", result->count);

    if (result->count == 0)
    {
        printf("(No results)\n\n");
        return;
    }

    // Display full table
    printf("┌────┬──────────────┬─────┬──────────┐\n");
    printf("│ ID │ Name         │ Age │ Salary   │\n");
    printf("├────┼──────────────┼─────┼──────────┤\n");

    for (int i = 0; i < result->count; i++)
    {
        printf("│ %-2d │ %-12s │ %3d │ %8.2f │\n",
               result->records[i]->id,
               result->records[i]->name,
               result->records[i]->age,
               result->records[i]->salary);
    }

    printf("└────┴──────────────┴─────┴──────────┘\n\n");
}