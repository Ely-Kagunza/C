#include "../include/database.h"
#include "../include/query.h"
#include "../include/hash_index.h"
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
    char *end = NULL;

    if (!read_line(prompt, line, sizeof(line))) {
        return 0;
    }

    *value = (int)strtol(line, &end, 10);
    if (end == line) {
        return 0;
    }

    return 1;
}

static int read_double(const char *prompt, double *value) {
    char line[64];
    char *end = NULL;

    if (!read_line(prompt, line, sizeof(line))) {
        return 0;
    }

    *value = strtod(line, &end);
    if (end == line) {
        return 0;
    }

    return 1;
}

static int read_person_details_without_id(Person *person, int id) {
    char name[NAME_SIZE];
    int age = 0;
    double salary = 0.0;

    if (!read_line("Enter Name: ", name, sizeof(name))) {
        return 0;
    }

    if (!read_int("Enter Age: ", &age)) {
        return 0;
    }

    if (!read_double("Enter Salary: ", &salary)) {
        return 0;
    }

    *person = person_create(id, name, age, salary);
    return 1;
}

static int create_person_from_input(Person *person) {
    int id = 0;
    int age = 0;
    double salary = 0.0;
    char name[NAME_SIZE];

    if (!read_int("Enter ID: ", &id)) {
        printf("Invalid ID input\n");
        return 0;
    }

    if (!read_line("Enter Name: ", name, sizeof(name))) {
        printf("Invalid name input\n");
        return 0;
    }

    if (!read_int("Enter Age: ", &age)) {
        printf("Invalid age input\n");
        return 0;
    }

    if (!read_double("Enter Salary: ", &salary)) {
        printf("Invalid salary input\n");
        return 0;
    }

    *person = person_create(id, name, age, salary);
    return 1;
}

static int read_age_range(int *min_age, int *max_age) {
    if (!read_int("Enter minimum age: ", min_age)) {
        return 0;
    }

    if (!read_int("Enter maximum age: ", max_age)) {
        return 0;
    }

    return 1;
}

static int read_salary_range(double *min_salary, double *max_salary) {
    if (!read_double("Enter minimum salary: ", min_salary)) {
        return 0;
    }

    if (!read_double("Enter maximum salary: ", max_salary)) {
        return 0;
    }

    return 1;
}

static void seed_sample_data(Database *db) {
    if (!database_add_person(db, person_create(1, "Alice", 30, 50000.0)) ||
        !database_add_person(db, person_create(2, "Bob", 24, 42000.0)) ||
        !database_add_person(db, person_create(3, "Charlie", 41, 78000.0))) {
        printf("Failed to seed sample data\n");
    } else {
        printf("Sample data seeded successfully\n");
    }
}

static void show_menu(void) {
    printf("\n");
    printf("1. Add person\n");
    printf("2. Display all people\n");
    printf("3. Find person by ID\n");
    printf("4. Find person by Name\n");
    printf("5. Find all people by Name fragment\n");
    printf("6. Find people by Age range\n");
    printf("7. Find people by Salary range\n");
    printf("8. Update person by ID\n");
    printf("9. Delete person by ID\n");
    printf("10. Save database\n");
    printf("11. Load database\n");
    printf("12. Seed sample data\n");
    printf("13. Sort by age\n");
    printf("14. Sort by salary\n");
    printf("15. Sort by name\n");
    printf("16. Compare age search performance\n");
    printf("17. Print hash index statistics\n");
    printf("0. Exit\n");
}

int main(void) {
    Database *db = database_load_text("people.txt");
    if (!db) {
        db = database_create(4);
    }

    if (!db) {
        printf("Failed to initialize database\n");
        return 1;
    }

    HashIndex *index = hash_index_create(8);
    if (index && db->count > 0) {
        hash_index_build(index, db);
    }

    int running = 1;

    while (running) {
        int choice = -1;

        show_menu();

        if (!read_int("Choose an option: ", &choice)) {
            printf("Input closed or invalid. Exiting.\n");
            break;
        }

        switch (choice) {
            case 1: {
                Person person;
                if (!create_person_from_input(&person)) {
                    printf("Failed to create person\n");
                } else if (database_id_exists(db, person.id)) {
                    printf("Person with ID %d already exists\n", person.id);
                } else if (!database_add_person(db, person)) {
                    printf("Failed to add person\n");
                } else {
                    printf("Person added successfully\n");
                    if (index) {
                        hash_index_insert(index, &db->records[db->count - 1]);
                    }
                }
                break;
            }

            case 2:
                database_display(db);
                database_print_stats(db);
                break;

            case 3: {
                int id = 0;
                if (!read_int("Enter ID to find: ", &id)) {
                    printf("Invalid ID input\n");
                    break;
                }

                Person *found = index ? hash_index_find_by_id(index, id) : database_find_by_id(db, id);
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

            case 4: {
                char name[NAME_SIZE];
                if (!read_line("Enter Name to find: ", name, sizeof(name))) {
                    printf("Invalid name input\n");
                    break;
                }
                
                Person *found = database_find_by_name(db, name);
                if (found) {
                    printf("\nFound person:\n");
                    printf("ID: %d\n", found->id);
                    printf("Name: %s\n", found->name);
                    printf("Age: %d\n", found->age);
                    printf("Salary: %.2f\n", found->salary);
                } else {
                    printf("No person found with name %s\n", name);
                }
                break;
            }

            case 5: {
                char name[NAME_SIZE];
                if (!read_line("Enter Name fragment to find: ", name, sizeof(name))) {
                    printf("Invalid name input\n");
                    break;
                }
                
                printf("\nMatching people:\n");
                database_find_all_by_name(db, name);
                printf("\n");
                break;
            }

            case 6: {
                int min_age = 0;
                int max_age = 0;

                if (!read_age_range(&min_age, &max_age)) {
                    printf("Invalid age range input\n");
                    break;
                }

                printf("\nMatching people:\n");
                database_find_by_age_range(db, min_age, max_age);
                printf("\n");
                break;   
            }

            case 7: {
                double min_salary = 0.0;
                double max_salary = 0.0;
                
                if (!read_salary_range(&min_salary, &max_salary)) {
                    printf("Invalid salary range input\n");
                    break;
                }

                printf("\nMatching people:\n");
                database_find_by_salary_range(db, min_salary, max_salary);
                printf("\n");
                break;
            }

            case 8: {
                int id = 0;
                Person updated;

                if (!read_int("Enter ID to update: ", &id)) {
                    printf("Invalid ID input\n");
                    break;
                }

                if (!database_find_by_id(db, id)) {
                    printf("No person found with ID %d\n", id);
                    break;
                }

                printf("Enter new details:\n");
                if (!read_person_details_without_id(&updated, id)) {
                    printf("Failed to create updated person\n");
                    break;
                }

                if (database_update_person(db, id, updated)) {
                    printf("Person updated successfully\n");
                } else {
                    printf("Failed to update person\n");
                }
                break;
            }

            case 9: {
                int id = 0;

                if (!read_int("Enter ID to delete: ", &id)) {
                    printf("Invalid ID input\n");
                    break;
                }

                if (database_delete_person(db, id)) {
                    printf("Person deleted successfully\n");
                    if (index) {
                        hash_index_remove(index, id);
                    }
                } else {
                    printf("No person found with ID %d\n", id);
                }
                break;
            }


            case 10:
                if (database_save_text(db, "people.txt")) {
                    printf("Database saved successfully\n");
                } else {
                    printf("Failed to save database\n");
                }
                database_print_stats(db);
                break;

            case 11: {
                Database *loaded = database_load_text("people.txt");
                if (!loaded) {
                    printf("Failed to load database from file\n");
                } else {
                    database_free(db);
                    db = loaded;
                    printf("Database loaded successfully\n");
                    if (index) {
                        hash_index_build(index, db);
                    }
                }
                database_print_stats(db);
                break;
            }

            case 12:
                seed_sample_data(db);
                break;

            case 13:
                database_sort_by_age(db);
                printf("Sorted by age\n");
                break;

            case 14:
                database_sort_by_salary(db);
                printf("Sorted by salary\n");
                break;

            case 15:
                database_sort_by_name(db);
                printf("Sorted by name\n");
                break;

            case 16: {
                int age = 0;

                if (!read_int("Enter age to compare search performance: ", &age)) {
                    printf("Invalid age input\n");
                    break;
                }

                database_compare_search_by_age(db, age);
                break;
            }

            case 17:
                hash_index_print_stats(index);
                hash_index_print_buckets(index);
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
    if (index) {
        hash_index_free(index);
    }
    database_free(db);
    return 0;
}