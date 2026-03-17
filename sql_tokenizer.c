#include "sql_tokenizer.h"
#include <string.h>
#include <ctype.h>

// Helper: Check if a word is a SQL keyword
static int is_keyword(const char *word)
{
    static const char *keywords[] = {
        "SELECT", "FROM", "WHERE", "AND", "OR", "NOT",
        "INSERT", "INTO", "VALUES", "UPDATE", "SET", "DELETE",
        "ORDER", "BY", "ASC", "DESC", "GROUP", "HAVING",
        "JOIN", "INNER", "LEFT", "RIGHT", "ON",
        "LIMIT", "OFFSET",
        NULL  // Null terminator to mark end of list
    };

    for (int i = 0; keywords[i] != NULL; i++)
    {
        if (string_compare_ignore_case(word, keywords[i]) == 0)  // Case-insensitive comparison
            return 1;
    }
    return 0;
}

// Helper: Check if a word is an operator
static int is_operator(const char *word)
{
    static const char *operators[] = {
        ">", "<", "=", "!=", "<=", ">=", "+", "-", "*", "/", "%",
        NULL
    };

    for (int i = 0; operators[i] != NULL; i++)
    {
        if (strcmp(word, operators[i]) == 0)
            return 1;
    }
    return 0;
}

// Helper: Check if character is a digit
static int is_digit(char c)
{
    return c >= '0' && c <= '9';
}

// Helper: Check if character can be part of an identifier
static int is_identifier_char(char c)
{
    return isalnum(c) || c == '_';
}

// Helper: Add a token to the TokenList (with dynamic resizing)
static void token_list_add(TokenList *list, SQLTokenType type, const char *value)
{
    // Resize if needed (same pattern as the database growth)
    if (list->count >= list->capacity)
    {
        list->capacity *= 2;
        list->tokens = realloc(list->tokens, list->capacity * sizeof(Token));
        if (list->tokens == NULL)
        {
            printf("ERROR: Failed to allocate tokens!\n");
            exit(1);
        }
    }

    // Allocate memory for the token value
    list->tokens[list->count].value = malloc(strlen(value) + 1);
    if (list->tokens[list->count].value == NULL)
    {
        printf("ERROR: Failed to allocate token value!\n");
        exit(1);
    }

    strcpy(list->tokens[list->count].value, value);
    list->tokens[list->count].type = type;
    list->count++;
}

// Main tokenizer function - this is what the parser calls
TokenList *tokenize(const char *sql)
{
    if (sql == NULL)
    {
        printf("ERROR: SQL query is NULL!\n");
        return NULL;
    }

    // Create TokenList structure
    TokenList *list = malloc(sizeof(TokenList));
    if (list == NULL)
    {
        printf("ERROR: Failed to allocate TokenList!\n");
        return NULL;
    }

    list->tokens = malloc(sizeof(Token) * 10);  // Start with 10 tokens
    list->count = 0;
    list->capacity = 10;

    // Walk through the SQL string character by character
    int i = 0;
    int len = strlen(sql);

    while (i < len)
    {
        // Skip whitespace
        while (i < len && isspace(sql[i]))
            i++;

        if (i >= len)
            break;

        // Case 1: Comma
        if (sql[i] == ',')
        {
            token_list_add(list, TOKEN_COMMA, ",");
            i++;
        }
        // Case 2: Left parenthesis
        else if (sql[i] == '(')
        {
            token_list_add(list, TOKEN_LPAREN, "(");
            i++;
        }
        // Case 3: Right parenthesis
        else if (sql[i] == ')')
        {
            token_list_add(list, TOKEN_RPAREN, ")");
            i++;
        }
        // Case 4: Star (for SELECT *)
        else if (sql[i] == '*')
        {
            token_list_add(list, TOKEN_STAR, "*");
            i++;
        }
        // Case 5: String literal (single or double quotes)
        else if (sql[i] == '\'' || sql[i] == '"')
        {
            char quote = sql[i];
            i++;  // Skip opening quote
            int start = i;

            // Find the closing quote
            while (i < len && sql[i] != quote)
                i++;

            // Extract string content (without quotes)
            int length = i - start;
            char *string_value = malloc(length + 1);
            if (string_value == NULL)
            {
                printf("ERROR: Failed to allocate string value!\n");
                return NULL;
            }
            strncpy(string_value, &sql[start], length);
            string_value[length] = '\0';

            token_list_add(list, TOKEN_STRING, string_value);
            free(string_value);

            i++;  // Skip closing quote
        }

        // Case 6: Number (integer or decimal)
        else if (is_digit(sql[i]))
        {
            int start = i;
            while (i < len && (is_digit(sql[i]) || sql[i] == '.'))
                i++;

            int length = i - start;
            char *number_value = malloc(length + 1);
            if (number_value == NULL)
            {
                printf("ERROR: Failed to allocate number value!\n");
                return NULL;
            }
            strncpy(number_value, &sql[start], length);
            number_value[length] = '\0';

            token_list_add(list, TOKEN_NUMBER, number_value);
            free(number_value);
        }

        // Case 7: Operator
        else if (strchr("!><=+-*/%", sql[i]))
        {
            int start = i;
            i++;

            // Check for two-character operators (!=, <=, >=)
            if (i < len && ((sql[start] == '!' && sql[i] == '=') ||
                           (sql[start] == '<' && sql[i] == '=') ||
                           (sql[start] == '>' && sql[i] == '=')))
            {
                i++;
            }
            
            int length = i - start;
            char *operator_value = malloc(length + 1);
            if (operator_value == NULL)
            {
                printf("ERROR: Failed to allocate operator value!\n");
                return NULL;
            }
            strncpy(operator_value, &sql[start], length);
            operator_value[length] = '\0';

            token_list_add(list, TOKEN_OPERATOR, operator_value);
            free(operator_value);
        }

        // Case 8: Keyword and identifier (words)
        else if (isalpha(sql[i]) || sql[i] == '_')
        {
            int start = i;
            while (i < len && is_identifier_char(sql[i]))
                i++;

            int length = i - start;
            char *word = malloc(length + 1);
            if (word == NULL)
            {
                printf("ERROR: Failed to allocate word value!\n");
                return NULL;
            }
            strncpy(word, &sql[start], length);
            word[length] = '\0';

            // Convert to uppercase for keyword matching
            char *upper_word = malloc(length + 1);
            strcpy(upper_word, word);
            for (int j = 0; j < length; j++)
                upper_word[j] = toupper(word[j]);

            // Determine if keyword or identifier
            if (is_keyword(upper_word))
                token_list_add(list, TOKEN_KEYWORD, upper_word);
            else
                token_list_add(list, TOKEN_IDENTIFIER, word);

            free(word);
            free(upper_word);
        }
        
        // Unknown character - return error
        else
        {
            printf("ERROR: Unknown character '%c' at position %d!\n", sql[i], i);
            i++;
        }
    }

    // Add EOF token to mark end
    token_list_add(list, TOKEN_EOF, "");

    return list;
}

// Free all memory allocated for tokens
void token_list_free(TokenList *list)
{
    if (list == NULL)
        return;

    for (int i = 0; i < list->count; i++)
    {
        free(list->tokens[i].value);
    }
    free(list->tokens);
    free(list);
}

// Debug function: print all tokens
void token_list_print(TokenList *list)
{
    if (list == NULL)
    {
        printf("TokenList is NULL!\n");
        return;
    }

    printf("\n=== Tokens ===\n");
    for (int i = 0; i < list->count; i++)
    {
        const char *type_name;
        switch (list->tokens[i].type)
        {
            case TOKEN_KEYWORD:    type_name = "KEYWORD"; break;
            case TOKEN_IDENTIFIER: type_name = "IDENT"; break;
            case TOKEN_NUMBER:     type_name = "NUMBER"; break;
            case TOKEN_OPERATOR:   type_name = "OP"; break;
            case TOKEN_STRING:     type_name = "STRING"; break;
            case TOKEN_COMMA:      type_name = "COMMA"; break;
            case TOKEN_LPAREN:     type_name = "LPAREN"; break;
            case TOKEN_RPAREN:     type_name = "RPAREN"; break;
            case TOKEN_STAR:       type_name = "STAR"; break;
            case TOKEN_EOF:        type_name = "EOF"; break;
            default:               type_name = "???"; break;
        }
        printf("[%2d] %-10s | %s\n", i, type_name, list->tokens[i].value);
    }
    printf("==================\n\n");
}

