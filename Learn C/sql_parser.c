#include "sql_parser.h"
#include "sql_tokenizer.h"
#include <stdlib.h>
#include <string.h>

// ============ PARSER STATE ===============
typedef struct {
    TokenList *tokens;
    int position;                // Current token position
} ParserState;

// ============ HELPER FUNCTIONS ============

// Get current token without consuming it
static Token *current_token(ParserState *state)
{
    if (state->position >= state->tokens->count)
        return &state->tokens->tokens[state->tokens->count - 1];  // Return last token (EOF)
    return &state->tokens->tokens[state->position];
}

// Get current token type
static SQLTokenType current_type(ParserState *state)
{
    return current_token(state)->type;
}

// Get current token value
static const char *current_value(ParserState *state)
{
    return current_token(state)->value;
}

// Advance to next token
static void consume_token(ParserState *state)
{
    if (state->position < state->tokens->count)
        state->position++;
}

// Check if current token is a keyword with specific value
static int is_keyword(ParserState *state, const char *keyword)
{
    if (current_type(state) != TOKEN_KEYWORD)
        return 0;
    return _stricmp(current_value(state), keyword) == 0;
}

// Check if current token is an identifier with specific value
static int is_identifier(ParserState *state, const char *name)
{
    if (current_type(state) != TOKEN_IDENTIFIER)
        return 0;
    return _stricmp(current_value(state), name) == 0;
}

// Expect a keyword, consume it, or error
static int expect_keyword(ParserState *state, const char *keyword, char **error_msg)
{
    if (!is_keyword(state, keyword))
    {
        char buffer[256];
        sprintf(buffer, "Expected keyword '%s', got '%s'", keyword, current_value(state));
        *error_msg = malloc(strlen(buffer) + 1);
        strcpy(*error_msg, buffer);
        return 0;
    }
    consume_token(state);
    return 1;
}

// Duplicate a string
static char *string_dup(const char *str)
{
    if (str == NULL)
        return NULL;
    char *copy = malloc(strlen(str) + 1);
    if (copy == NULL)
    {
        printf("ERROR: Memory allocation failed\n");
        exit(1);
    }
    strcpy(copy, str);
    return copy;
}

// ============ PARSER FUNCTIONS ============

// Parse column list: "id, name, age" or "*"
static int parse_column_list(ParserState *state, SelectQuery *query, char **error_msg)
{
    // Check for SELECT *
    if (current_type(state) == TOKEN_STAR)
    {
        query->select_all = 1;
        query->columns = malloc(sizeof(char *));
        query->columns[0] = string_dup("*");
        query->column_count = 1;
        consume_token(state);
        return 1;
    }

    // Parse column names
    query->columns = malloc(sizeof(char *) * 10);   // Start with 10 columns
    query->column_count = 0;
    int capacity = 10;

    while (1)
    {
        if (current_type(state) != TOKEN_IDENTIFIER)
        {
            *error_msg = string_dup("Expected column name");
            return 0;
        }

        // Resize if needed
        if (query->column_count >= capacity)
        {
            capacity *= 2;
            query->columns = realloc(query->columns, sizeof(char *) * capacity);
        }
        
        query->columns[query->column_count++] = string_dup(current_value(state));
        consume_token(state);

        // Check for comma (more columns)
        if (current_type(state) == TOKEN_COMMA)
        {
            consume_token(state);
            continue;
        }
        break;
    }

    return 1;
}

// Parse WHERE conditions: "age > 30 AND salary < 100000"
static int parse_where_clause(ParserState *state, SelectQuery *query, char **error_msg)
{
    if (!expect_keyword(state, "WHERE", error_msg))
        return 0;

    query->conditions = malloc(sizeof(WhereCondition) * 10);   // Start with 10 conditions
    query->condition_count = 0;
    int capacity = 10;
    query->where_and = 1;   // Default to AND between conditions

    while (1)
    {
        // Parse one condition: column operator value
        if (current_type(state) != TOKEN_IDENTIFIER)
        {
            *error_msg = string_dup("Expected column name in WHERE clause");
            return 0;
        }

        // Resize if needed
        if (query->condition_count >= capacity)
        {
            capacity *= 2;
            query->conditions = realloc(query->conditions, sizeof(WhereCondition) * capacity);
        }

        query->conditions[query->condition_count].column = string_dup(current_value(state));
        consume_token(state);

        // Parse operator
        if (current_type(state) != TOKEN_OPERATOR)
        {
            *error_msg = string_dup("Expected operator in WHERE clause");
            return 0;
        }
        query->conditions[query->condition_count].op = string_dup(current_value(state));
        consume_token(state);

        // Parse value (number or string)
        if (current_type(state) != TOKEN_NUMBER &&
            current_type(state) != TOKEN_STRING &&
            current_type(state) != TOKEN_IDENTIFIER)
        {
            *error_msg = string_dup("Expected number or string value in WHERE clause");
            return 0;
        }
        query->conditions[query->condition_count].value = string_dup(current_value(state));
        consume_token(state);

        query->condition_count++;

        // Check for AND/OR separator
        if (is_keyword(state, "AND"))
        {
            query->where_and = 1;
            consume_token(state);
            continue;
        }
        else if (is_keyword(state, "OR"))
        {
            query->where_and = 0;
            consume_token(state);
            continue;
        }
        break;
    }

    return 1;
}

// Parse ORDER BY clause: "age DESC" or "salary ASC"
static int parse_order_by(ParserState *state, SelectQuery *query, char **error_msg)
{
    if (!expect_keyword(state, "ORDER", error_msg))
        return 0;
    if (!expect_keyword(state, "BY", error_msg))
        return 0;

    if (current_type(state) != TOKEN_IDENTIFIER)
    {
        *error_msg = string_dup("Expected column name in ORDER BY clause");
        return 0;
    }

    query->order_by.column = string_dup(current_value(state));
    consume_token(state);

    query->order_by.descending = 0;
    if (is_keyword(state, "DESC"))
    {
        query->order_by.descending = 1;
        consume_token(state);
    }
    else if (is_keyword(state, "ASC"))
    {
        query->order_by.descending = 0;
        consume_token(state);
    }

    query->has_order_by = 1;
    return 1;
}

// Parse LIMIT clause: "LIMIT 10" or "LIMIT 10 OFFSET 5"
static int parse_limit(ParserState *state, SelectQuery *query, char **error_msg)
{
    if (!expect_keyword(state, "LIMIT", error_msg))
        return 0;

    if (current_type(state) != TOKEN_NUMBER)
    {
        *error_msg = string_dup("Expected number in LIMIT clause");
        return 0;
    }

    query->limit = atoi(current_value(state));
    consume_token(state);

    if (is_keyword(state, "OFFSET"))
    {
        consume_token(state);
        if (current_type(state) != TOKEN_NUMBER)
        {
            *error_msg = string_dup("Expected number in OFFSET clause");
            return 0;
        }
        query->offset = atoi(current_value(state));
        consume_token(state);
    }

    return 1;
}

// Parse SELECT query: "SELECT ... FROM ... WHERE ... ORDER BY ... LIMIT ..."
static int parse_select_query(ParserState *state, SelectQuery *query, char **error_msg)
{
    // Expect SELECT keyword
    if (!expect_keyword(state, "SELECT", error_msg))
        return 0;
    
    // Parse column list
    if (!parse_column_list(state, query, error_msg))
        return 0;

    // Expect FROM keyword
    if (!expect_keyword(state, "FROM", error_msg))
        return 0;

    // Parse table name
    if (current_type(state) != TOKEN_IDENTIFIER)
    {
        *error_msg = string_dup("Expected table name");
        return 0;
    }
    query->table_name = string_dup(current_value(state));
    consume_token(state);

    // Initialize defaults
    query->condition_count = 0;
    query->has_order_by = 0;
    query->limit = -1;
    query->offset = -1;

    // Optional WHERE clause
    if (is_keyword(state, "WHERE"))
    {
        if (!parse_where_clause(state, query, error_msg))
            return 0;
    }

    // Optional ORDER BY clause
    if (is_keyword(state, "ORDER"))
    {
        if (!parse_order_by(state, query, error_msg))
            return 0;
    }

    // Optional LIMIT clause
    if (is_keyword(state, "LIMIT"))
    {
        if (!parse_limit(state, query, error_msg))
            return 0;
    }

    // Expect end of query EOF
    if (current_type(state) != TOKEN_EOF)
    {
        *error_msg = string_dup("Expected end of query");
        return 0;
    }

    return 1;
}

// ============ PUBLIC API FUNCTIONS ============

// Main parse function - entry point
ParserResult *parse(TokenList *tokens)
{
    if (tokens == NULL || tokens->count == 0)
    {
        ParserResult *result = malloc(sizeof(ParserResult));
        result->success = 0;
        result->error_message = string_dup("No tokens to parse");
        return result;
    }

    ParserState state;
    state.tokens = tokens;
    state.position = 0;

    ParserResult *result = malloc(sizeof(ParserResult));
    if (result == NULL)
    {
        printf("ERROR: Memory allocation failed\n");
        exit(1);
    }

    result->error_message = NULL;

    // Determine query type from first token
    if (is_keyword(&state, "SELECT"))
    {
        result->parsed_query.type = QUERY_SELECT;
        memset(&result->parsed_query.query.select, 0, sizeof(SelectQuery));

        if (parse_select_query(&state, &result->parsed_query.query.select, &result->error_message))
        {
            result->success = 1;
        }
        else
        {
            result->success = 0;
        }
    }
    else if (is_keyword(&state, "INSERT"))
    {
        result->parsed_query.type = QUERY_INSERT;
        result->success = 0;
        result->error_message = string_dup("INSERT not yet implemented");
    }
    else if (is_keyword(&state, "UPDATE"))
    {
        result->parsed_query.type = QUERY_UPDATE;
        result->success = 0;
        result->error_message = string_dup("UPDATE not yet implemented");
    }
    else if (is_keyword(&state, "DELETE"))
    {
        result->parsed_query.type = QUERY_DELETE;
        result->success = 0;
        result->error_message = string_dup("DELETE not yet implemented");
    }
    else
    {
        result->parsed_query.type = QUERY_UNKNOWN;
        result->success = 0;
        result->error_message = string_dup("Unknown query type");
    }

    return result;
}

// Free memory allocated for parsed query
void parser_result_free(ParserResult *result)
{
    if (result == NULL)
        return;

    if (result->error_message != NULL)
        free(result->error_message);

    // Free query-specific memory
    if (result->parsed_query.type == QUERY_SELECT)
    {
        SelectQuery *q = &result->parsed_query.query.select;

        if (q->columns != NULL)
        {
            for (int i = 0; i < q->column_count; i++)
                free(q->columns[i]);
            free(q->columns);
        }

        free(q->table_name);

        if (q->conditions != NULL)
        {
            for (int i = 0; i < q->condition_count; i++)
            {
                free(q->conditions[i].column);
                free(q->conditions[i].op);
                free(q->conditions[i].value);
            }
            free(q->conditions);
        }

        free(q->order_by.column);
    }

    free(result);
}

// Debug function: print parsed query
void print_parsed_query(ParserResult *result)
{
    if (result == NULL)
    {
        printf("Result is NULL\n");
        return;
    }

    if (!result->success)
    {
        printf("Error: %s\n", result->error_message);
        return;
    }

    printf("\n PARSE SUCCESS\n");
    printf("Query type: %d\n", result->parsed_query.type);

    if (result->parsed_query.type == QUERY_SELECT)
    {
        SelectQuery *q = &result->parsed_query.query.select;
        printf("SELECT\n");
        printf("  Columns: ");
        for (int i = 0; i < q->column_count; i++)
            printf("%s%s", q->columns[i], (i < q->column_count - 1) ? ", " : "");
        printf("\n");
        printf("  From: %s\n", q->table_name);

        if (q->condition_count > 0)
        {
            printf("  Where: ");
            for (int i = 0; i < q->condition_count; i++)
            {
                printf("%s %s %s", q->conditions[i].column, q->conditions[i].op, q->conditions[i].value);
                if (i < q->condition_count - 1)
                    printf(" %s ", q->where_and ? "AND" : "OR");
            }
            printf("\n");
        }

        if (q->has_order_by)
            printf("  Order By: %s %s\n", q->order_by.column, q->order_by.descending ? "DESC" : "ASC");

        if (q->limit >= 0)
            printf("  Limit: %d\n", q->limit);
    }
    else if (result->parsed_query.type == QUERY_INSERT)
        printf("INSERT\n");
    else if (result->parsed_query.type == QUERY_UPDATE)
        printf("UPDATE\n");
    else if (result->parsed_query.type == QUERY_DELETE)
        printf("DELETE\n");
    else
        printf("UNKNOWN\n");

    printf("\n");
}