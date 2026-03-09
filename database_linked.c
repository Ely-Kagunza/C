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

// Function prototypes
PersonNode *create_node(Person data);
PersonNode *add_node_to_list(PersonNode *head, Person data);
void display_list(PersonNode *head);
void free_list(PersonNode *head);
void save_linked_list(const char *filename, PersonNode *head);
PersonNode *load_linked_list(const char *filename);
void show_menu(void);
PersonNode *add_person_interactive(PersonNode *head, int *next_id);

int main(void) {
    PersonNode *head = NULL;
    int choice;
    int next_id = 1;

    head = load_linked_list("people.txt");

    // Calculate next_id based on highest ID in loaded records
    if (head == NULL) {
      int max_id = 0;
        // Count existing records ro set next_id correctly
        PersonNode *current = head;
        while (current != NULL) {
            if (current->data.id > max_id) {
                max_id = current->data.id;
            }
            current = current->next;
        }
        next_id = max_id + 1;
    }

    while (1) {
        show_menu();

        if (scanf("%d", &choice) != 1) {
            printf("Invalis input. Try again.\n");
            while (getchar() != '\n'); // Clear invalid input
            continue;
        }

        getchar(); // Clear the newline character from the buffer

        if (choice == 1) {
            head = add_person_interactive(head, &next_id);
        } else if (choice == 2) {
            display_list(head);
        } else if (choice == 3) {
            save_linked_list("people.txt", head);
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
        printf("Memory allocation failed\n");
        return NULL;
    }

    new_node->data = data;
    new_node->next = NULL;

    return new_node;
}

PersonNode *add_node_to_list(PersonNode *head, Person data) {
    PersonNode *new_node = create_node(data);
    if (new_node == NULL) return head;  // Memory allocation failed

    // Case 1: Empty list
    if (head == NULL) {
        return new_node;
    }

    // Case 2: Find the end and link
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
    printf("│ ID │ Name         │ Age │  Salary  │\n");
    printf("├────┼──────────────┼─────┼──────────┤\n");

    int count = 0;
    PersonNode *current = head;

    while (current != NULL) {
        printf("│ %2d │ %-12s │ %3d │ %8.2f │\n",
            current->data.id,
            current->data.name,
            current->data.age,
            current->data.salary);

        current = current->next;  // Move to next node
        count++;
    }

    printf("└────┴──────────────┴─────┴──────────┘\n");
    printf("Total records: %d\n\n", count);
}

void free_list(PersonNode *head) {
    PersonNode *current = head;

    while (current != NULL) {
        PersonNode *next = current->next;  // SAVE the next pointer first!
        free(current);                     // FREE the current node
        current = next;                    // MOVE to the next node
    }
}

void save_linked_list(const char *filename, PersonNode *head) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        perror("Error opening file for writing");
        return;
    }

    fprintf(file, "ID,Name,Age,Salary\n");

    PersonNode *current = head;
    while (current != NULL) {
        fprintf(file, "%d,%s,%d,%.2f\n",
            current->data.id,
            current->data.name,
            current->data.age,
            current->data.salary);

        current = current->next;
    }

    fclose(file);
    printf("Database saved to %s\n", filename);
}

PersonNode *load_linked_list(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("No existing database file. Starting fresh.\n");
        return NULL;
    }

    PersonNode *head = NULL;
    char line[1024];

    fgets(line, sizeof(line), file);  // Skip header line

    while (fgets(line, sizeof(line), file) != NULL) {
        Person person;

        // Parse the CSV line
        sscanf(line, "%d,%99[^,],%d,%lf",
            &person.id,
            person.name,
            &person.age,
            &person.salary);

        // Add to the linked list
        head = add_node_to_list(head, person);
    }

    fclose(file);
    printf("Loaded records from %s\n", filename);
    return head;
}