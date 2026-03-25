#define _CRT_SECURE_NO_WARNINGS
#include "../../include/database.h"

#include <stdlib.h>
#include <string.h>

Person person_create(int id, const char *name, int age, double salary) {
    Person person;
    person.id = id;
    person.age = age;
    person.salary = salary;

    if (name) {
        strncpy(person.name, name, NAME_SIZE - 1);
        person.name[NAME_SIZE - 1] = '\0';
    } else {
        person.name[0] = '\0';
    }

    return person;
}

static int database_reserve(Database *db, int min_capacity) {
    if (!db) {
        return 0;
    }

    if (min_capacity <= db->capacity) {
        return 1;
    }

    int new_capacity = db->capacity > 0 ? db->capacity : 4;
    while (new_capacity < min_capacity) {
        new_capacity *= 2;
    }

    printf("Reallocating from %d to %d\n", db->capacity, new_capacity);

    Person *new_records = realloc(db->records, (size_t)new_capacity * sizeof(Person));
    if (!new_records) {
        return 0;
    }

    db->records = new_records;
    db->capacity = new_capacity;
    return 1;
}

Database *database_create(int initial_capacity) {
    if (initial_capacity <= 0) {
        initial_capacity = 4;
    }

    Database *db = malloc(sizeof(Database));
    if (!db) {
        return NULL;
    }

    db->records = malloc((size_t)initial_capacity * sizeof(Person));
    if (!db->records) {
        free(db);
        return NULL;
    }

    db->count = 0;
    db->capacity = initial_capacity;

    return db;
}

void database_free(Database *db) {
    if (!db) {
        return;
    }

    free(db->records);
    free(db);
}

int database_id_exists(Database *db, int id) {
    if (!db) {
        return 0;
    }

    for (int i = 0; i < db->count; i++) {
        if (db->records[i].id == id) {
            return 1;
        }
    }

    return 0;
}

int database_add_person(Database *db, Person person) {
    if (!db) {
        return 0;
    }

    if (database_id_exists(db, person.id)) {
        printf("Person with ID %d already exists\n", person.id);
        return 0;
    }

    if (!database_reserve(db, db->count + 1)) {
        return 0;
    }

    db->records[db->count++] = person;
    return 1;
}

int database_compact(Database *db) {
    if (!db) {
        return 0;
    }

    int target_capacity = db->capacity;

    if (db->count == 0) {
        target_capacity = 4;
    } else if (db->capacity > 4 && db->count * 4 < db->capacity) {
        target_capacity = db->count * 2;
        if (target_capacity < 4) {
            target_capacity = 4;
        }
    } else {
        return 1;
    }

    Person *new_records = realloc(db->records, (size_t)target_capacity * sizeof(Person));
    if (!new_records) {
        return 0;
    }

    db->records = new_records;
    db->capacity = target_capacity;
    return 1;
}

int database_update_person(Database *db, int id, Person updated_person) {
    if (!db) {
        return 0;
    }

    for (int i = 0; i < db->count; i++) {
        if (db->records[i].id == id) {
            db->records[i] = updated_person;
            return 1;
        }
    }

    return 0;
}

int database_delete_person(Database *db, int id) {
    if (!db) {
        return 0;
    }

    for (int i = 0; i < db->count; i++) {
        if (db->records[i].id == id) {
            for (int j = i; j < db->count - 1; j++) {
                db->records[j] = db->records[j + 1];
            }

            db->count--;
            database_compact(db);
            return 1;
        }
    }

    return 0;
}

Person *database_find_by_id(Database *db, int id) {
    if (!db) {
        return NULL;
    }

    for (int i = 0; i < db->count; i++) {
        if (db->records[i].id == id) {
            return &db->records[i];
        }
    }

    return NULL;
}

void database_display(const Database *db) {
    if (!db) {
        return;
    }

    printf("\n%-6s %-20s %-6s %-12s\n", "ID", "Name", "Age", "Salary");
    printf("------------------------------------------------------\n");

    for (int i = 0; i < db->count; i++) {
        const Person *p = &db->records[i];
        printf("%-6d %-20s %-6d %-12.2f\n", p->id, p->name, p->age, p->salary);
    }

    printf("\nTotal records: %d\n", db->count);
}

int database_save_text(const Database *db, const char *filename) {
    if (!db || !filename) {
        return 0;
    }

    FILE *file = fopen(filename, "w");
    if (!file) {
        return 0;
    }

    if (fprintf(file, "%d\n", db->count) < 0) {
        fclose(file);
        return 0;
    }

    for (int i = 0; i < db->count; i++) {
        const Person *p = &db->records[i];
        if (fprintf(file, "%d|%s|%d|%.2f\n", p->id, p->name, p->age, p->salary) < 0) {
            fclose(file);
            return 0;
        }
    }

    fclose(file);
    return 1;
}

Database *database_load_text(const char *filename) {
    if (!filename) {
        return NULL;
    }

    FILE *file = fopen(filename, "r");
    if (!file) {
        return NULL;
    }

    char line[256];
    if (!fgets(line, sizeof(line), file)) {
        fclose(file);
        return NULL;
    }

    int count = 0;
    if (sscanf(line, "%d", &count) != 1 || count < 0) {
        fclose(file);
        return NULL;
    }

    Database *db = database_create(count > 0 ? count : 4);
    if (!db) {
        fclose(file);
        return NULL;
    }

    for (int i = 0; i < count; i++) {
        if (!fgets(line, sizeof(line), file)) {
            database_free(db);
            fclose(file);
            return NULL;
        }

        Person person;
        if (sscanf(line, "%d|%63[^|]|%d|%lf",
                   &person.id,
                   person.name,
                   &person.age,
                   &person.salary) != 4) {
            database_free(db);
            fclose(file);
            return NULL;
        }

        if (!database_add_person(db, person)) {
            database_free(db);
            fclose(file);
            return NULL;
        }
    }

    fclose(file);
    return db;
}


size_t database_memory_usage_bytes(const Database *db) {
    if (!db) {
        return 0;
    }

    return sizeof(Database) + (size_t)db->capacity * sizeof(Person);
}

void database_print_stats(const Database *db) {
    if (!db) {
        printf("Database is NULL\n");
        return;
    }

    printf("\nDatabase stats\n");
    printf("Count: %d\n", db->count);
    printf("Capacity: %d\n", db->capacity);
    printf("Approx memory: %zu bytes\n", database_memory_usage_bytes(db));
}