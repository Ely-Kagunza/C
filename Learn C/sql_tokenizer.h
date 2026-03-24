#ifndef SQL_TOKENIZER_H
#define SQL_TOKENIZER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Token types - what kind of thing is this token?
typedef enum {
    TOKEN_KEYWORD,               // SELECT, FROM, WHERE, INSERT, UPDATE, DELETE, etc.
    TOKEN_IDENTIFIER,            // table name, column name, aliases (people, age, name)
    TOKEN_NUMBER,                // 30, 100.5, 42 (integers and decimals)
    TOKEN_OPERATOR,              // +, -, *, /, =, <, >, <=, >=, !=, AND, OR, NOT, etc.
    TOKEN_STRING,                // 'John', "Smith", '123 Main St', etc. (string literals with quotes)
    TOKEN_COMMA,                 // , (comma separator)
    TOKEN_LPAREN,                // ( (left parenthesis)
    TOKEN_RPAREN,                // ) (right parenthesis)
    TOKEN_STAR,                  // * (special case for SELECT *)
    TOKEN_EOF,                   // end of file
} SQLTokenType;

// Single token from the SQL query
typedef struct {
    SQLTokenType type;
    char *value;                // The actual text ("SELECT", "age", "30", ">", etc.)
} Token;

// Collection of all tokens from one SQL query
typedef struct {
    Token *tokens;              // Array of tokens structs
    int count;                  // How many tokens we've parsed
    int capacity;               // How much space we allocated
} TokenList;

// Public API - what the parser will call
TokenList *tokenize(const char *sql);
void token_list_free(TokenList *list);
void token_list_print(TokenList *list);    // Debugging function

// Portable case-insensitive string comparison
// MSVC uses _stricmp, POSIX uses strcasecmp
static int string_compare_ignore_case(const char *s1, const char *s2)
{
    #ifdef _WIN32
        return _stricmp(s1, s2);
    #else
        return strcasecmp(s1, s2);
    #endif
}

#endif // SQL_TOKENIZER_H