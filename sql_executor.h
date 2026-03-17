#ifndef SQL_EXECUTOR_H
#define SQL_EXECUTOR_H

#include "sql_parser.h"
#include "database.h"

// Result of executing a query
typedef struct {
    Person **records;                 // Array of Person pointers matched
    int count;                        // Number of records matched
    int success;                      // 1 if successful, 0 if error
    char *error_message;              // Error message if failed
} QueryResult;

// Main execution function - executes parsed query aganist database
QueryResult *execute_query(Database *db, ParserResult *parsed_query);

// Free memory allocated for query result
void query_result_free(QueryResult *result);

// Display query results
void query_result_display(QueryResult *result, SelectQuery *query);

#endif