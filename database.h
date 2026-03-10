#ifndef DATABASE_H
#define DATABASE_H

#include "person.h"

// Core database operations
Database *db_create(int initial_capacity);
void db_free(Database *db);
int db_add_record(Database *db, Person record);
int resize_database(Database *db, int new_capacity);
void db_display(Database *db);
void db_memory_stats(Database *db);

#endif