#ifndef DATABASE_H
#define DATABASE_H

#include <stdio.h>

#define NAME_SIZE 64

typedef struct {
    int id;
    char name[NAME_SIZE];
    int age;
    double salary;
} Person;

typedef struct {
    Person *records;
    int count;
    int capacity;
} Database;

Person person_create(int id, const char *name, int age, double salary);

Database *database_create(int initial_capacity);
void database_free(Database *db);

int database_id_exists(Database *db, int id);
int database_add_person(Database *db, Person person);
int database_update_person(Database *db, int id, Person updated_person);
int database_delete_person(Database *db, int id);
Person *database_find_by_id(Database *db, int id);

void database_display(const Database *db);

int database_save_text(const Database *db, const char *filename);
Database *database_load_text(const char *filename);

#endif