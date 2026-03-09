#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int id;
    char name[100];
    int age;
    double salary;
} Person;

// Function declarations
void save_database(const char *filename, Person *people, int count);
int load_database(const char *filename, Person *people);
void display_database(Person *people, int count);
void show_menu(void);
void add_person(Person *people, int *count);

// Main menu and orchestration
int main(void) {
    Person database[100];
    int count = 0;
    int choice;

    // Load existing database
    count = load_database("people.txt", database);

    while (1) {
        show_menu();

        if (scanf("%d", &choice) != 1) {
            printf("Invalid input. Try again.\n");
            while (getchar() != '\n');  // Clear buffer
            continue;
        }

        getchar();  // Clear newline from buffer after scanf

        if (choice == 1) {
            add_person(database, &count);
        } else if (choice == 2) {
            display_database(database, count);
        } else if (choice == 3) {
            save_database("people.txt", database, count);
        } else if (choice == 4) {
            printf("Goodbye!\n");
            break;
        } else {
            printf("Invalid choice. Try again.\n");
        }
    }

    return 0;
}

void show_menu(void) {
    printf("\n=== Database Menu ===\n");
    printf("1. Add a person\n");
    printf("2. View all records\n");
    printf("3. Save database\n");
    printf("4. Quit\n");
    printf("Selection: ");
}

void add_person(Person *people, int *count) {
    if (*count >= 100) {
        printf("Database is full!\n");
        return;
    }

    Person new_person;
    new_person.id = *count + 1;  // Next ID is count + 1

    // Get name
    printf("Enter name: ");
    fgets(new_person.name, sizeof(new_person.name), stdin);
    // Remove newline from fgets (it includes it)
    new_person.name[strcspn(new_person.name, "\n")] = '\0';

    // Get age
    printf("Enter age: ");
    scanf("%d", &new_person.age);

    // Get salary
    printf("Enter salary: ");
    scanf("%lf", &new_person.salary);
    getchar(); // Clear newline from scanf

    // Add to database
    people[*count] = new_person;
    (*count)++;

    printf("Person added successfully!\n");
}

void display_database(Person *people, int count) {
    if (count == 0) {
        printf("\nDatabase is empty.\n");
        return;
    }

    printf("\n");
    printf("┌────┬──────────────┬─────┬──────────┐\n");
    printf("│ ID │ Name         │ Age │  Salary  │\n");
    printf("├────┼──────────────┼─────┼──────────┤\n");

    for (int i = 0; i < count; i++) {
        printf("│ %2d │ %-12s │ %3d │ %8.2f │\n",
            people[i].id,
            people[i].name,
            people[i].age,
            people[i].salary);
    }

    printf("└────┴──────────────┴─────┴──────────┘\n");
    printf("Total records: %d\n\n", count);
}

void save_database(const char *filename, Person *people, int count) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        perror("Error opening file for writing");
        return;
    }

    // Write header
    fprintf(file, "ID,Name,Age,Salary\n");

    // Write each record
    for (int i = 0; i < count; i++) {
        fprintf(file, "%d,%s,%d,%.2f\n",
            people[i].id,
            people[i].name,
            people[i].age,
            people[i].salary);
    }

    fclose(file);
    printf("Database saved to %s\n", filename);
}

int load_database(const char *filename, Person *people) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("No existing database file. Starting fresh.\n");
        return 0;
    }

    char line[1024];
    int count = 0;

    // Skip header line
    fgets(line, sizeof(line), file);

    // Read each line
    while (fgets(line, sizeof(line), file) != NULL && count < 100) {
        // Parse: "1, Alice, 30, 50000.00"
        sscanf(line, "%d,%99[^,],%d,%lf",
            &people[count].id,
            people[count].name,
            &people[count].age,
            &people[count].salary);
        
        count++;
    }

    fclose(file);
    printf("Loaded %d records from %s\n", count, filename);
    return count;
}