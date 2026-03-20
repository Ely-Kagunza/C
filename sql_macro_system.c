#include "sql_macro_system.h"
#include <stddef.h>

// ============== GENERATED FIELD INFO TABLE ==============
// This is generated from the PERSON_FIELDS X-macro

FieldInfo person_fields[] = {
// For each FIELD in PERSON_FIELDS, generate a FieldInfo entry
#define FIELD(name, type, fmt) {STRINGIFY(name), fmt, offsetof(Person, name)},
    PERSON_FIELDS
#undef FIELD
};

// ============== X-MACRO: GENERATED PRINT FUNCTION ==============
// Generates code to print all fields of a Person

void macro_print_person(Person *p) {
  if (p == NULL) {
    printf("NULL Person\n");
    return;
  }

  printf("Person Record:\n");
  printf("─────────────────────────────────────\n");

  // For each field, generate a printf call
#define FIELD(name, type, fmt)                                                 \
  printf("  %-10s: " fmt "\n", STRINGIFY(name), p->name);

  // This expands to:
  // printf("  %-10s: %d\n", "id", p->id);
  // printf("  %-10s: %s\n", "name", p->name);
  // printf("  %-10s: %d\n", "age", p->age);
  // printf("  %-10s: %.2f\n", "salary", p->salary);

  PERSON_FIELDS

#undef FIELD
}

// ============== X-MACRO: GENERATED COMPARISON FUNCTION ==============

int macro_compare_persons(Person *a, Person *b) {
  if (a == NULL || b == NULL)
    return 0;

  printf("Comparing persons:\n");

  // For each field, generate a comparison
#define FIELD(name, type, fmt)                                                 \
  printf("  %s: %s ", STRINGIFY(name),                                         \
         (a->name == b->name) ? "SAME" : "DIFF");                              \
  printf("(" fmt " vs " fmt ")\n", a->name, b->name);

  PERSON_FIELDS

#undef FIELD

  return 1;
}

// ============== X-MACRO: GENERATED SERIALIZATION FUNCTION ==============

void macro_serialize_person(Person *p, char *buffer, int buffer_size) {
  if (p == NULL || buffer == NULL)
    return;

  int offset = 0;

  // For each field, generate serialization code
#define FIELD(name, type, fmt)                                                 \
  offset += snprintf(buffer + offset, buffer_size - offset,                    \
                     STRINGIFY(name) "=" fmt "|", p->name);

  PERSON_FIELDS

#undef FIELD

  printf("Serialized: %s\n", buffer);
}

// =============== X-MACRO: GENERATED DESERIALIZATION ===============

void macro_deserialize_person(const char *buffer, Person *p) {
  if (buffer == NULL || p == NULL)
    return;

  printf("Deserializinf: %s\n", buffer);

  // This is simplified - just scan the string
  sscanf(buffer, "id=%d|name=%99[^|]|age=%d|salary=%lf|", &p->id, p->name,
         &p->age, &p->salary);
}

void macro_display_field_info(void) {
  printf("\n=== Person Structure Field Information ===\n\n");
  printf("Total fields: %d\n\n", PERSON_FIELD_COUNT);

  printf("Field Details:\n");
  printf("─────────────────────────────────────────────┐\n");
  printf("  %-12s | %-10s | %-10s | Offset\n", "Name", "Format", "Type");
  printf("─────────────────────────────────────────────┤\n");

  // Generate table rows
#define FIELD(name, type, fmt)                                                 \
  printf("  %-12s | %-10s | %-10s | %zu\n", STRINGIFY(name), fmt,              \
         STRINGIFY(type), offsetof(Person, name));

  PERSON_FIELDS

#undef FIELD

  printf("─────────────────────────────────────────────┘\n");
}

// =============== DEMONSTRATION: MACRO-GENERATED COMPARISON TABLE
// ===============

void macro_generate_comparison_table(void) {
  printf("\n=== Macro Code Generation Example ===\n\n");

  printf("Original X-macro definition:\n");
  printf("───────────────────────────────────────────────\n");
  printf("#define PERSON_FIELDS \\\n");
  printf("    FIELD(id, int, \"%%d\") \\\n");
  printf("    FIELD(name, char*, \"%%s\") \\\n");
  printf("    FIELD(age, int, \"%%d\") \\\n");
  printf("    FIELD(salary, double, \"%%.2f\")\n\n");

  printf("Generated macro_print_person() code:\n");
  printf("───────────────────────────────────────────────\n");
  printf("void macro_print_person(Person *p) {\n");
  printf("    printf(\"  %%s: %%d\\\\n\", \"id\", p->id);\n");
  printf("    printf(\"  %%s: %%s\\\\n\", \"name\", p->name);\n");
  printf("    printf(\"  %%s: %%d\\\\n\", \"age\", p->age);\n");
  printf("    printf(\"  %%s: %%.2f\\\\n\", \"salary\", p->salary);\n");
  printf("}\n\n");

  printf("Generated macro_compare_persons() code:\n");
  printf("───────────────────────────────────────────────\n");
  printf("int macro_compare_persons(Person *a, Person *b) {\n");
  printf("    printf(\"  id: %%s\", (a->id == b->id) ? \"✓\" : \"✗\");\n");
  printf(
      "    printf(\"  name: %%s\", (a->name == b->name) ? \"✓\" : \"✗\");\n");
  printf("    printf(\"  age: %%s\", (a->age == b->age) ? \"✓\" : \"✗\");\n");
  printf("    printf(\"  salary: %%s\", (a->salary == b->salary) ? \"✓\" : "
         "\"✗\");\n");
  printf("}\n\n");

  printf("Key insight:\n");
  printf("───────────────────────────────────────────────\n");
  printf("• Define fields ONCE in PERSON_FIELDS\n");
  printf("• Redefine FIELD macro to generate different code\n");
  printf("• Eliminates duplication and sync errors\n");
  printf("• Add new field? Update only PERSON_FIELDS!\n\n");
}

// =============== PRACTICAL EXAMPLE: OPERATOR CODES TABLE ===============

// Using X-macros to generate database operator codes
#define SQL_OPERATORS                                                          \
  OP(EQ, ==, "equals")                                                         \
  OP(GT, >, "greater than")                                                    \
  OP(LT, <, "less than")                                                       \
  OP(GTE, >=, "greater or equal")                                              \
  OP(LTE, <=, "less or equal")                                                 \
  OP(NEQ, !=, "not equal")

typedef enum {
#define OP(name, symbol, desc) OP_##name,
  SQL_OPERATORS
#undef OP
} OperatorCode;

const char *operator_names[] = {
#define OP(name, symbol, desc) desc,
    SQL_OPERATORS
#undef OP
};

const char *operator_symbols[] = {
#define OP(name, symbol, desc) STRINGIFY(symbol),
    SQL_OPERATORS
#undef OP
};

void macro_display_operators(void) {
  printf("\n=== Generated Operator Table ===\n\n");
  printf("Operators generated from SQL_OPERATORS X-macro:\n\n");

  int num_ops = sizeof(operator_names) / sizeof(operator_names[0]);
  for (int i = 0; i < num_ops; i++) {
    printf("  %-3d | %-20s | %s\n", i, operator_names[i], operator_symbols[i]);
  }
  printf("\n");
}

// =============== VARIADIC MACRO LOGGER ===============

typedef struct {
  int debug_count;
  int info_count;
  int error_count;
} MacroStats;

static MacroStats macro_stats = {0, 0, 0};

void reset_macro_stats(void) {
  macro_stats.debug_count = 0;
  macro_stats.info_count = 0;
  macro_stats.error_count = 0;
}

void display_macro_stats(void) {
  printf("\n=== Macro System Statistics ===\n");
  printf("Debug messages:  %d\n", macro_stats.debug_count);
  printf("Info messages:   %d\n", macro_stats.info_count);
  printf("Error messages:  %d\n\n", macro_stats.error_count);
}

// Note: In real usage, these would be called through the macros
// which increment counters, but for demo purposes we'll track them manually