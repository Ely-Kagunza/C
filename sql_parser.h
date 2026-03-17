#ifndef SQL_PARSER_H
#define SQL_PARSER_H

#include "sql_tokenizer.h"

// ============ QUERY STRUCTURES ============

// One WHERE condition: "age > 30" or "name = 'John'"
typedef struct {
    char *column;      // "age", "salary", etc.
    char *op;    // ">", "<", "=", "!=", "<=", ">=", "LIKE"
    char *value;       // "30", "100000", "'John'", etc.
} WhereCondition;

// ORDER BY clause: column and direction
typedef struct {
    char *column;      // "salary"
    int descending;    // 1 for DESC, 0 for ASC
} OrderByClause;

// SELECT query structure
typedef struct {
    // SELECT clause
    char **columns;           // ["id", "name", "age"] or ["*"]
    int column_count;
    int select_all;           // 1 if SELECT *, 0 otherwise
    
    // FROM clause
    char *table_name;         // "people"
    
    // WHERE clause
    WhereCondition *conditions;
    int condition_count;
    int where_and;            // 1 if AND, 0 if OR between conditions
    
    // ORDER BY clause
    OrderByClause order_by;
    int has_order_by;
    
    // LIMIT clause
    int limit;                // -1 means no limit
    int offset;               // -1 means no offset
} SelectQuery;

// INSERT query structure
typedef struct {
    char *table_name;         // "people"
    char **columns;           // ["id", "name", "age"]
    int column_count;
    char **values;            // ["1", "'John'", "30"]
    int value_count;
} InsertQuery;

// UPDATE query structure
typedef struct {
    char *table_name;         // "people"
    char **set_columns;       // ["salary", "age"]
    char **set_values;        // ["60000", "32"]
    int set_count;
    WhereCondition *conditions;
    int condition_count;
} UpdateQuery;

// DELETE query structure
typedef struct {
    char *table_name;         // "people"
    WhereCondition *conditions;
    int condition_count;
} DeleteQuery;

// Union of all query types
typedef enum {
    QUERY_SELECT,
    QUERY_INSERT,
    QUERY_UPDATE,
    QUERY_DELETE,
    QUERY_UNKNOWN
} QueryType;

typedef struct {
    QueryType type;
    union {
        SelectQuery select;
        InsertQuery insert;
        UpdateQuery update;
        DeleteQuery del;
    } query;
} ParsedQuery;

// Parser result: success or failure
typedef struct {
    int success;              // 1 = success, 0 = error
    char *error_message;      // Error description if failed
    ParsedQuery parsed_query;
} ParserResult;

// ============ PUBLIC API ============

// Main parsing function - converts tokens to query structure
ParserResult *parse(TokenList *tokens);

// Free memory allocated for parsed query
void parser_result_free(ParserResult *result);

// Debug function: print parsed query
void print_parsed_query(ParserResult *result);

#endif