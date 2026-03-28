#ifndef JSON_UTILS_H
#define JSON_UTILS_H

#define _CRT_SECURE_NO_WARNINGS
#include "database.h"

#define JSON_BUFFER_SIZE 65536

// ================== PERSON JSON ==================

// Convert single Person to JSON string
// Returns static buffer, caller should not free
char *person_to_json(const Person *p);

// Parse JSON string into Person struct
// Returns 1 on success, 0 on failure
int person_from_json(const char *json, Person *p);

// Convert array of Person pointers to JSON array
// Returns static buffer
char *person_array_to_json(const Person *people, int count);

// ================== ERROR RESPONSES ==================

// Build error JSON response
char *json_error(const char *message);

// Build success JSON response
char *json_success(const char *data);

// ================== GENERIC JSON ==================

// Build status response: {"status": "message"}
char *json_status(const char *status);

// Build key-value response: {"key": "value"}
char *json_key_value(const char *key, const char *value);

// Escape special characters in string for JSON
char *json_escape_string(const char *str);

#endif // JSON_UTILS_H