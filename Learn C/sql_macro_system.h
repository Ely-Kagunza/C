#ifndef SQL_MACRO_SYSTEM_H
#define SQL_MACRO_SYSTEM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "database.h"

// ============== PART 1: BASIC MACROS ==============

// Simple function-like macro
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

// Variadic macros (variable number of arguments)
#define PRINT_DEBUG(fmt, ...) \
    printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)

#define PRINT_ERROR(fmt, ...) \
    printf("[ERROR] " fmt "\n", ##__VA_ARGS__)

#define PRINT_INFO(fmt, ...) \
    printf("[INFO] " fmt "\n", ##__VA_ARGS__)

// ============== PART 2: STRINGIFICATION ==============

// Convert macro argument to string
#define STRINGIFY(x) #x

// Useful for field names
#define FIELD_NAME(field) STRINGIFY(field)

// ============== PART 3: TOKEN PASTING ==============

// Concatenate two tokens
#define CONCAT(a, b) a##b

// Generate function names from field names
#define COMPARE_FUNC(type) compare_by_##type
#define SORT_FUNC(type) sort_by_##type

// ============== PART 4: X-MACRO PATTERN ==============
// The most powerful metaprogramming technique in C

// Define all Person fields ONCE
// Format: FIELD(field_name, field_type, field_string)
#define PERSON_FIELDS         \
    FIELD(id, int, "%d")      \
    FIELD(name, char *, "%s") \
    FIELD(age, int, "%d")     \
    FIELD(salary, double, "%.2f")

// =============== PART 5: GENERATE CODE USING X-MACRO ===============

// First, define FIELD to count occurences
#define FIELD(name, type, fmt) +1

// Then expand PERSON_FIELDS with the counting version of FIELD
enum {
    PERSON_FIELD_COUNT_VALUE = (0 PERSON_FIELDS)
};

// Clean up the temporary FIELD macro
#undef FIELD

#define PERSON_FIELD_COUNT PERSON_FIELD_COUNT_VALUE

// Generate field info array
typedef struct
{
    const char *name;
    const char *format;
    int offset;
} FieldInfo;

extern FieldInfo person_fields[PERSON_FIELD_COUNT];

// Functions that work with any struct (generated from X-macros)
void macro_print_person(Person *p);
int macro_compare_persons(Person *a, Person *b);
void macro_serialize_person(Person *p, char *buffer, int buffer_size);
void macro_deserialize_person(const char *buffer, Person *p);
void macro_display_operators(void);
void macro_display_field_info(void);

// Demonstrate code generation
void macro_generate_comparison_table(void);

#endif