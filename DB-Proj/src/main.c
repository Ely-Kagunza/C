#include "../include/database.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int read_line(const char *prompt, char *buffer, size_t size) {
    printf("%s", prompt);

    if (fgets(buffer, (int)size, stdin) == NULL) {
        return 0;
    }

    buffer[strcspn(buffer, "\n")] = '\0';
    return 1;
}

static int read_int(const char *prompt, int *value) {
    char line[64];

    if (!read_line(prompt, line, sizeof(line))) {
        return 0;
    };
    
    *value = (int)strtol(line, NULL, 10);
    return 1;
}

static double read_double(const char *prompt, int *ok) {
    char line[64];
    char *end = NULL;

    if (!read_line(prompt, line, sizeof(line))) {
        *ok = 0;
        return 0.0;
    }

    double value = strtod(line, &end);
    if (end == line) {
        *ok = 0;
        return 0.0;
    }

    *ok = 1;
    return value;
}

static int create_person_from_input(Person *person) {
    char name[NAME_SIZE];
    int id = 0;
    int age = 0;
    int ok = 0;
    double salary = 0.0;

    if (!read_int("Enter ID: ", &id)) return 0;
    if (!read_line("Enter Name: ", name, sizeof(name))) return 0;
    if (!read_int("Enter Age: ", &age)) return 0;

    salary = read_double("Enter Salary: ", &ok);
    if (!ok) return 0;

    *person = person_create(id, name, age, salary);
    return 1;
}

static void seed_sample_data(Database *db) {
    database_add_person(db, person_create(1, "Alice", 30, 50000.0));
    database_add_person(db, person_create(2, "Bob", 24, 42000.0));
    database_add_person(db, person_create(3, "Charlie", 41, 78000.0));
}

static void show_menu(void) {
    printf("\n");
    printf("1. Add person\n");
    printf("2. Display all people\n");
    printf("3. Find person by ID\n");
    printf("4. Save database\n");
    printf("5. Load database\n");
    printf("6. Seed sample data\n");
    printf("0. Exit\n");
}

int main(void) {
    Database *db = database_create(4);
    if (!db) {
        printf("Failed to create database\n");
        return 1;
    }

    int running = 1;

    while (running) {
        show_menu();

        int choice;
        if (!read_int("Choose an option: ", &choice)) {
            printf("Invalid input\n");
            break;
        }

        switch (choice) {
            case 1: {
                Person person;
                if (!create_person_from_input(&person)) {
                    printf("Failed to read person data\n");
                } else if (!database_add_person(db, person)) {
                    printf("Failed to add person\n");
                } else {
                    printf("Person added successfully\n");
                }
                break;
            }

            case 2:
                database_display(db);
                break;

            case 3: {
                int id = 0;
                if (!read_int("Enter ID to find: ", &id)) {
                    printf("Invalid input\n");
                    break;
                }

                Person *found = database_find_by_id(db, id);

                if (found) {
                    printf("\nFound person:\n");
                    printf("ID: %d\n", found->id);
                    printf("Name: %s\n", found->name);
                    printf("Age: %d\n", found->age);
                    printf("Salary: %.2f\n", found->salary);
                } else {
                    printf("No person found with ID %d\n", id);
                }
                break;
            }

            case 4:
                if (database_save_text(db, "people.txt")) {
                    printf("Database saved successfully\n");
                } else {
                    printf("Failed to save database\n");
                }
                break;

            case 5: {
                Database *loaded = database_load_text("people.txt");
                if (!loaded) {
                    printf("Failed to load database from file\n");
                } else {
                    database_free(db);
                    db = loaded;
                    printf("Database loaded successfully\n");
                }
                break;
            }

            case 6:
                seed_sample_data(db);
                printf("Sample data seeded successfully\n");
                break;

            case 0:
                running = 0;
                break;

            default:
                printf("Invalid option\n");
                break;
        }
    }
    
    database_save_text(db, "people.txt");
    database_free(db);
    return 0;
}