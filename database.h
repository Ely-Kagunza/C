#ifndef DATABASE_H
#define DATABASE_H

#include "person.h"
#include "hash.h"
#include "cache.h"

// Database structure for in-memory storage
typedef struct {
  Person *records;
  int count;
  int capacity;
  HashTable *id_index;
  QueryCache *query_cache;
} Database;

// Core database operations
Database *db_create(int initial_capacity);
void db_free(Database *db);
int db_add_record(Database *db, Person record);
int resize_database(Database *db, int new_capacity);
void db_display(Database *db);
void db_memory_stats(Database *db);

// Hash table lookup (Phase 7)
Person *db_get_by_id(Database *db, int id);

#endif