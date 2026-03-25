#define _CRT_SECURE_NO_WARNINGS
#include "../../include/database.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static void to_lower_copy(char *dest, const char *src, size_t size) {
    size_t i = 0;
    for (; i + 1 < size && src[i] != '\0'; i++) {
        dest[i] = (char)tolower((unsigned char)src[i]);
    }
    dest[i] = '\0';
}

static int contains_ignore_case(const char *text, const char *pattern) {
    char lower_text[NAME_SIZE];
    char lower_pattern[NAME_SIZE];

    if (!text || !pattern) {
        return 0;
    }

    to_lower_copy(lower_text, text, sizeof(lower_text));
    to_lower_copy(lower_pattern, pattern, sizeof(lower_pattern));

    return strstr(lower_text, lower_pattern) != NULL;
}

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

static int database_resize(Database *db, int new_capacity) {
    Person *new_records = realloc(db->records, new_capacity * sizeof(Person));
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

Person *database_find_by_name(const Database *db, const char *name) {
    if (!db || !name) {
        return NULL;
    }

    for (int i = 0; i < db->count; i++) {
        if (_stricmp(db->records[i].name, name) == 0) {
            return &db->records[i];
        }
    }

    return NULL;
}

void database_find_all_by_name(const Database *db, const char *name) {
    if (!db || !name) {
        return;
    }

    int found_any = 0;

    printf("\n%-6s %-20s %-6s %-12s\n", "ID", "Name", "Age", "Salary");
    printf("------------------------------------------------------\n");

    for (int i = 0; i < db->count; i++) {
        if (contains_ignore_case(db->records[i].name, name)) {
            const Person *p = &db->records[i];
            printf("%-6d %-20s %-6d %-12.2f\n", p->id, p->name, p->age, p->salary);
            found_any = 1;
        }
    }

    if (!found_any) {
        printf("No matching people found for name fragment '%s'\n", name);
    }
}

int database_add_person(Database *db, Person person) {
    if (!db) {
        return 0;
    }

    if (database_id_exists(db, person.id)) {
        printf("Person with ID %d already exists\n", person.id);
        return 0;
    }

    if (db->count >= db->capacity) {
        int new_capacity = db->capacity * 2;
        if (!database_resize(db, new_capacity)) {
            return 0;
        }
    }

    db->records[db->count++] = person;
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