#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int id;
    char name[100];
    int age;
    double salary;
} Person;

typedef struct PersonNode {
    Person data;
    struct PersonNode *next;
} PersonNode;

// Header for binary file
typedef struct {
    char magic[4]; // "DB01"
    int version; // 1
    int record_count; // Number of records in the file
} DatabaseHeader;

// Function declarations
PersonNode *create_node(Person data);
PersonNode *add_node_to_list(PersonNode *head, Person data);
void display_list(PersonNode *head);
void free_list(PersonNode *head);
void save_binary_with_header(const char *filename, PersonNode *head);
PersonNode *load_binary_with_header(const char *filename);
void show_menu(void);
PersonNode *add_person_interactive(PersonNode *head, int *next_id);

int main(void) {
    PersonNode *head = NULL;
    int choice;
    int next_id = 1;

    head = load_binary_with_header("people.db");

    if (head == NULL) {
        PersonNode *current = head;
        while (current != NULL) {
            if (current->data.id >= next_id) {
                next_id = current->data.id + 1;
            }
            current = current->next;
        }
    }

    while (1) {
        show_menu();

        if (scanf("%d", &choice) != 1) {
            printf("Invalid input. Try again.\n");
            while (getchar() != '\n'); // Clear invalid input
            continue;
        }

        getchar(); // Clear the newline character from the buffer

        if (choice == 1) {
            head = add_person_interactive(head, &next_id);
        } else if (choice == 2) {
            display_list(head);
        } else if (choice == 3) {
            save_binary_with_header("people.db", head);
        } else if (choice == 4) {
            printf("Goodbye!\n");
            break;
        } else {
            printf("Invalid choice. Try again.\n");
        }
    }

    free_list(head);
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

PersonNode *add_person_interactive(PersonNode *head, int *next_id) {
    Person new_person;
    new_person.id = (*next_id)++;

    printf("Enter name: ");
    fgets(new_person.name, sizeof(new_person.name), stdin);
    new_person.name[strcspn(new_person.name, "\n")] = '\0';

    printf("Enter age: ");
    scanf("%d", &new_person.age);

    printf("Enter salary: ");
    scanf("%lf", &new_person.salary);
    getchar(); // Clear the newline character from the buffer

    head = add_node_to_list(head, new_person);
    printf("Person added successfully.\n");

    return head;
}

PersonNode *create_node(Person data) {
    PersonNode *new_node = malloc(sizeof(PersonNode));
    if (new_node == NULL) {
        printf("Memory allocation failed.\n");
        return NULL;
    }
    new_node->data = data;
    new_node->next = NULL;
    return new_node;
}

PersonNode *add_node_to_list(PersonNode *head, Person data) {
    PersonNode *new_node = create_node(data);
    if (new_node == NULL) return head;

    if (head == NULL) {
        return new_node;
    }

    PersonNode *current = head;
    while (current->next != NULL) {
        current = current->next;
    }
    current->next = new_node;
    return head;
}

void display_list(PersonNode *head) {
    if (head == NULL) {
        printf("\nDatabase is empty.\n");
        return;
    }

    printf("\n");
    printf("┌────┬──────────────┬─────┬──────────┐\n");
    printf("│ ID │ Name         │ Age │ Salary   │\n");
    printf("├────┼──────────────┼─────┼──────────┤\n");

    int count = 0;
    PersonNode *current = head;
    while (current != NULL) {
        printf("│ %-2d │ %-12s │ %3d │ %8.2f │\n",
            current->data.id,
            current->data.name,
            current->data.age,
            current->data.salary);

        current = current->next;
        count++;
    }
    printf("└────┴──────────────┴─────┴──────────┘\n");
    printf("Total records: %d\n\n", count);
}

void free_list(PersonNode *head) {
    PersonNode *current = head;
    while (current != NULL) {
        PersonNode *next = current->next; // Save next pointer first!
        free(current); // Free the current node
        current = next; // Move to next node
    }
}

void save_binary_with_header(const char *filename, PersonNode *head) {
    FILE *file = fopen(filename, "wb");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }

    // Count records
    int count = 0;
    PersonNode *current = head;
    while (current != NULL) {
        count++;
        current = current->next;
    }

    // Write header
    DatabaseHeader header;
    strcpy(header.magic, "PEDB");
    header.version = 1;
    header.record_count = count;
    fwrite(&header, sizeof(DatabaseHeader), 1, file);

    // Write all records
    current = head;
    while (current != NULL) {
        fwrite(&current->data, sizeof(Person), 1, file);current = current->next;
    }

    fclose(file);
    printf("Database saved to %s\n", filename);
}

PersonNode *load_binary_with_header(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        printf("No binary file found. Starting fresh.\n");
        return NULL;
    }

    DatabaseHeader header;
    if (fread(&header, sizeof(DatabaseHeader), 1, file) != 1) {
        printf("Invalid database file!\n");
        fclose(file);
        return NULL;
    }

    if (strcmp(header.magic, "PEDB") != 0) {
        printf("This is not a valid database file!\n");
        fclose(file);
        return NULL;
    }

    printf("Loading database version %d with %d records...\n",
        header.version, header.record_count);

    PersonNode *head = NULL;
    for (int i = 0; i < header.record_count; i++) {
        Person person;
        if (fread(&person, sizeof(Person), 1, file) != 1) {
            printf("Error reading record %d!\n", i + 1);
            break;
        }
        head = add_node_to_list(head, person);
    }

    fclose(file);
    return head;
}