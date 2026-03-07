/** diary.c - Phase 1: basic Structure and File Writing
 * We are learning how to open a file, write to it, and close it manually.
 */

# include <stdio.h> // Standard Input/Output (for printf, fopen, etc.)
# include <stdlib.h> // Standard Library (for exit codes)
# include <string.h> // For strcmp
# include <time.h> // For timestamp functions
# include <ctype.h> // For tolower()

// === STRUCT DEFINITIONS ===
typedef struct {
    char timestamp[200]; // The timestamp line (e.g., "--- ENTRY: Fri Mar 6 21:37:16 2026")
    char *content; // All the entry text (dynamically allocated)
    int content_size; // How much memory we allocated for content
} DiaryEntry;

typedef struct DiaryNode {
    DiaryEntry data;
    struct DiaryNode *next;
} DiaryNode;

// === FUNCTION PROTOTYPES (declarations) ===
// Core helpers (file operations)
DiaryEntry *load_entries(const char *filename, int *count);
void display_entries(DiaryEntry *entries, int count);
void save_entries(const char *filename, DiaryEntry *entries, int count);
void free_entries(DiaryEntry *entries, int count);

// Linked list helpers
DiaryNode *create_node(DiaryEntry entry);
DiaryNode *add_node_to_list(DiaryNode *head, DiaryEntry entry);
void display_entries_linked(DiaryNode *head);
DiaryNode *remove_node_at_position(DiaryNode *head, int position);
void free_list(DiaryNode *head);
DiaryNode *load_entries_linked_list(const char *filename);
void save_linked_list(const char *filename, DiaryNode *head);

// Menu handlers
void show_menu(void);
void handle_write_entry(void);
void handle_view_entries(void);
void handle_search_entries(void);
void handle_delete_entry(void);

// === HELPER FUNCTIONS (Load, Display, Save, Free) ===

/**
* Load all diary entries from file into memory.
* Returns an array of DiaryEntry structs.
* Sets *count to the number of entries loaded.
*/
DiaryEntry *load_entries(const char *filename, int *count) {
FILE *file = fopen(filename, "r");
if (file == NULL) {
    *count = 0;
    return NULL;
}

DiaryEntry *entries = malloc(sizeof(DiaryEntry) * 10); // Start with capacity for 10 entries
if (entries == NULL) {
    perror("malloc failed");
    fclose(file);
    return NULL;
}

int capacity = 10;
int num_entries = 0;
char line[1024];
int in_entry = 0;

while (fgets(line, sizeof(line), file) != NULL) {
    // Check if this line starts an entry
    if (strncmp(line, "--- ENTRY:", 10) == 0) {
        // If we already have an entry, save it
        if (in_entry && num_entries > 0) {
            // Previous entry's content is already in entries[num_entries - 1].content
        }

        // Grow array if needed
        if (num_entries >= capacity) {
            capacity *= 2;
            DiaryEntry *temp = realloc(entries, sizeof(DiaryEntry) * capacity);
            if (temp == NULL) {
                perror("realloc failed");
                fclose(file);
                return entries;  // Return what we have so far
            }
            entries = temp;
        }

        // Start a new entry
        strcpy(entries[num_entries].timestamp, line);
        entries[num_entries].content = malloc(1024);
        if (entries[num_entries].content == NULL) {
            perror("malloc failed");
            fclose(file);
            return entries;
        }
        entries[num_entries].content_size = 1024;
        entries[num_entries].content[0] = '\0';  // empty string
        in_entry = 1;
        num_entries++;
    } else if (in_entry && num_entries > 0) {
        // Add line to current entry's content
        DiaryEntry *current = &entries[num_entries - 1];
        int current_len = strlen(current->content);
        int line_len = strlen(line);

        // Grow content if needed
        while (current_len + line_len >= current->content_size) {
            current->content_size *= 2;
            char *temp = realloc(current->content, current->content_size);
            if (temp == NULL) {
                perror("realloc failed");
                fclose(file);
                return entries;  // Return what we have so far
            }
            current->content = temp;
        }

        strcat(current->content, line);
    }
}

fclose(file);
*count = num_entries;
return entries;
}

/**
 * Display all entries with numbers.
 */

void display_entries(DiaryEntry *entries, int count) {
    if (count == 0) {
        printf("No entries yet.\n");
        return;
    }

    for (int i = 0; i < count; i++) {
        printf("\n--- ENTRY #%d ---\n", i + 1);
        printf("%s", entries[i].timestamp);
        printf("%s", entries[i].content);
    }
}

/**
 * Save all entries to file, overwriting existing content.
 */

void save_entries(const char *filename, DiaryEntry *entries, int count) {
    FILE *file = fopen(filename, "w");  // Open in WRITE mode to clear file
    if (file == NULL) {
        perror("Error opening diary.txt");
        return;
    }

    for (int i = 0; i < count; i++) {
        fprintf(file, "%s", entries[i].timestamp);
        fprintf(file, "%s", entries[i].content);
    }

    fclose(file);
}

/**
 * Free all dynamically allocated memory for entries.
 */

void free_entries(DiaryEntry *entries, int count) {
    for (int i = 0; i < count; i++) {
        free(entries[i].content);  // free each entry's content
    }
    free(entries);  // free the array of entries
}

/**
 * Create a new node with the given entry data.
 * Returns a pointer to the new node, or NULL if mallaoc fails.
 */
DiaryNode *create_node(DiaryEntry entry) {
    DiaryNode *new_node = (DiaryNode *)malloc(sizeof(DiaryNode));
    if (new_node == NULL) {
        perror("malloc failed for new node");
        return NULL;
    }

    new_node->data = entry;  // Copy the entry into the node
    new_node->next = NULL;   // New node initially points to nothing

    return new_node;
}

/**
 * Add a new node to the end of the linked list.
 * To add to an empty list: head = add_node_to_list(head, entry);
 */
DiaryNode *add_node_to_list(DiaryNode *head, DiaryEntry entry) {
    DiaryNode *new_node = create_node(entry);
    if (new_node == NULL) return head;  // malloc failed, return unchanged list

    // Case 1: Empty list
    if (head == NULL) {
        return new_node;  // New node becomes the head
    }

    // Case 2: Non-empty list - traverse to the end
    DiaryNode *current = head;
    while (current->next != NULL) {
        current = current->next;  // Move to the next node
    }

    // Now current points to the last node
    current->next = new_node;  // Link the new node to the end

    return head;  // Return the (possibly changed) head of the list
}

/**
 * Traverse the linked list and display all entries with numbers.
 */
 void display_entries_linked(DiaryNode *head) {
    if (head == NULL) {
        printf("No entries yet.\n");
        return;
    }

    DiaryNode *current = head;
    int count = 1;

    while (current != NULL) {
        printf("\n--- ENTRY #%d ---\n", count);
        printf("%s", current->data.timestamp);
        printf("%s", current->data.content);

        current = current->next;  // Move to the next node
        count++;
    }
}

/**
 * Remove the node at position (1-based).
 * Position 1 = first node, Position 2 = second node, etc.
 * Returns the (possibly new) head of the list.
 */
DiaryNode *remove_node_at_position(DiaryNode *head, int position) {
    if (head == NULL || position < 1) {
        printf("Invalid position.\n");
        return head;
    }

    // Case 1: Delete the HEAD node
    if (position == 1) {
        DiaryNode *new_head = head->next;  // The next node becomes the new head
        free(head->data.content);  // Free the entry's content
        free(head);  // Free the node itself
        return new_head;
    }

    // Case 2: Delete a middle or last node
    DiaryNode *current = head;
    int count = 1;

    // Traverse to the node BEFORE the one we want to delete
    while (current != NULL && count < position - 1) {
        current = current->next;
        count++;
    }

    // Check if we found a valid position
    if (current == NULL || current->next == NULL) {
        printf("Invalid position.\n");
        return head;
    }

    // Now current->next is the node to delete
    DiaryNode *node_to_delete = current->next;
    current->next = node_to_delete->next;  // Skip the node to delete

    free(node_to_delete->data.content);  // Free the entry's content
    free(node_to_delete);  // Free the node itself

    return head;  // Return the (possibly changed) head of the list
}

/**
 * Free all nodes in the linked list.
 */
void free_list(DiaryNode *head) {
    DiaryNode *current = head;

    while (current != NULL) {
        DiaryNode *next_node = current->next;  // Save the pointer to the next node
        free(current->data.content);  // Free the entry's content
        free(current);  // Free the node itself
        current = next_node;  // Move to the next node
    }
}

/**
 * load all diary entries from file into a linked list.
 * Returns a pointer to the head of the list
 */
DiaryNode *load_entries_linked_list(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        return NULL;  // No file = empty list
    }

    DiaryNode *head = NULL;
    char line[1024];
    int in_entry = 0;
    DiaryEntry current_entry;

    // Initialize current_entry
    strcpy(current_entry.timestamp, "");
    current_entry.content = malloc(1024);
    if (current_entry.content == NULL) {
        perror("malloc failed");
        fclose(file);
        return head;
    }
    current_entry.content_size = 1024;
    current_entry.content[0] = '\0';  // empty string

    while (fgets(line, sizeof(line), file) != NULL) {
        // Check if this line starts a new entry
        if (strncmp(line, "--- ENTRY:", 10) == 0) {
            // If we have a previous entry, add it to the list
            if (in_entry && strlen(current_entry.timestamp) > 0) {
                head = add_node_to_list(head, current_entry);
            }

            // Start a new entry
            strcpy(current_entry.timestamp, line);
            current_entry.content = malloc(1024);
            if (current_entry.content == NULL) {
                perror("malloc failed");
                fclose(file);
                return head;
            }
            current_entry.content_size = 1024;
            current_entry.content[0] = '\0';  // empty string
            in_entry = 1;
        } else if (in_entry) {
            // Add line to current entry's content
            int current_len = strlen(current_entry.content);
            int line_len = strlen(line);

            // Grow content if needed
            while (current_len + line_len >= current_entry.content_size) {
                current_entry.content_size *= 2;
                char *temp = realloc(current_entry.content, current_entry.content_size);
                if (temp == NULL) {
                    perror("realloc failed");
                    fclose(file);
                    return head;
                }
                current_entry.content = temp;
            }

            strcat(current_entry.content, line);
            current_len = strlen(current_entry.content);
        }
    }

    // Don't forget to add the last entry
    if (in_entry && strlen(current_entry.timestamp) > 0) {
        head = add_node_to_list(head, current_entry);
    }

    fclose(file);
    return head;
}

/**
 * Save all entries in the linked list to file.
 */
void save_linked_list(const char *filename, DiaryNode *head) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        perror("Error opening diary.txt");
        return;
    }

    DiaryNode *current = head;
    while (current != NULL) {
        fprintf(file, "%s", current->data.timestamp);
        fprintf(file, "%s", current->data.content);
        current = current->next;
    }

    fclose(file);
}

// === MENU HANDLER FUNCTIONS ===

/**
 * Display the menu options.
 */

void show_menu(void) {
    printf("--- Personal Diary ---\n");
    printf("1. Write Entry (Type '.' on a new line to save)\n");
    printf("2. View Entries\n");
    printf("3. Search Entries\n");
    printf("4. Delete Entry\n");
    printf("5. Edit Entry\n");
    printf("6. Quit\n");
    printf("Selection: ");
}

/**
 * Handle writing a new entry.
 */

 void handle_write_entry(void) {
    FILE *file = fopen("diary.txt", "a");
    if (file == NULL) {
        perror("Error opening diary.txt");
        return;
    }

    time_t now = time(NULL);
    fprintf(file, "\n--- ENTRY: %s", ctime(&now));

    printf("\nEnter your thoughts (type '.' on its own line to finish):\n");

    char buffer[1024];
    while (1) {
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) break;

        if (strcmp(buffer, ".\n") == 0) {
            break;
        }

        fprintf(file, "%s", buffer);
    }

    fclose(file);
    printf("Entry saved!\n");
}

/**
 * Handle viewing all entries.
 */
 void handle_view_entries(void) {
    DiaryNode *head = load_entries_linked_list("diary.txt");

    if (head == NULL) {
        printf("No entries yet.\n");
        return;
    }

    display_entries_linked(head);
    free_list(head);
}

/**
 * Handle editing an existing entry
 */
 void handle_edit_entry(void) {
    DiaryNode *head = load_entries_linked_list("diary.txt");

    if (head == NULL) {
        printf("No entries to edit.\n");
        return;
    }

    display_entries_linked(head);

    printf("\nEnter entry number to edit (1-...): ");
    int entry_to_edit;

    if (scanf("%d", &entry_to_edit) != 1) {
        printf("Invalid input.\n");
        while (getchar() != '\n');
        free_list(head);
        return;
    }

    while (getchar() != '\n');  // Clear the buffer

    // Find the node to edit
    DiaryNode *current = head;
    int position = 1;
    while (current != NULL && position < entry_to_edit) {
        current = current->next;
        position++;
    }

    if (current == NULL) {
        printf("Invalid entry number.\n");
        free_list(head);
        return;
    }

    // Show current entry
    printf("\n--- CURRENT ENTRY #%d ---\n", entry_to_edit);
    printf("%s", current->data.timestamp);
    printf("%s\n", current->data.content);

    // Ask for new content
    printf("Enter new content (type '.' on its own line to finish):\n");

    char new_content[4096];
    new_content[0] = '\0';
    int total_length = 0;

    while (1) {
        char line[1024];
        if (fgets(line, sizeof(line), stdin) == NULL) break;

        if (strcmp(line, ".\n") == 0) break;

        strcat(new_content, line);
        total_length += strlen(line);

        if (total_length >= 4000) {
            printf("Content too long! Keeping previous content.\n");
            free_list(head);
            return;
        }
    }

    // Update the entry
    free(current->data.content);
    current->data.content = malloc(strlen(new_content) + 1);
    if (current->data.content == NULL) {
        perror("malloc failed");
        free_list(head);
        return;
    }

    strcpy(current->data.content, new_content);
    current->data.content_size = strlen(new_content) + 1;

    // Update timestamp
    time_t now = time(NULL);
    snprintf(current->data.timestamp, sizeof(current->data.timestamp), 
             "--- ENTRY: %s", ctime(&now));

    // Save all entries
    save_linked_list("diary.txt", head);
    printf("Entry edited!\n");

    free_list(head);
}

/**
 * Convert a string to lowercase (modifies the string in-place)
 * Returns the input so you can chain calls
 */
 char *to_lowercase(char *str) {
    for (int i = 0; str[i] != '\0'; i++) {
        str[i] = tolower((unsigned char) str[i]);
    }
    return str;
}

/**
 * Handle searching entries by keyword.
 */
void handle_search_entries(void) {
    printf("\nEnter search keyword: ");

    char buffer[1024];
    fgets(buffer, sizeof(buffer), stdin);
    buffer[strcspn(buffer, "\n")] = '\0';
    to_lowercase(buffer);

    DiaryNode *head = load_entries_linked_list("diary.txt");

    if (head == NULL) {
        printf("No entries to search.\n");
        return;
    }

    int position = 1;
    int matches = 0;
    DiaryNode *current = head;

    printf("\n--- SEARCH RESULTS FOR '%s' ---\n", buffer);

    while (current != NULL) {
        char line_copy[4096];
        strcpy(line_copy, current->data.content);
        to_lowercase(line_copy);

        if (strstr(line_copy, buffer) != NULL) {
            printf("\n--- ENTRY #%d ---\n", position);
            printf("%s", current->data.timestamp);
            printf("%s", current->data.content);
            matches++;
        }
        current = current->next;
        position++;
    }

    printf("\nFound %d match(es).\n", matches);
    free_list(head);
}

/**
 * Handle deleting an entry.
 */
 void handle_delete_entry(void) {
    DiaryNode *head = load_entries_linked_list("diary.txt");

    if (head == NULL) {
        printf("No entries to delete.\n");
        return;
    }

    display_entries_linked(head);

    printf("\nEnter entry number to delete (1-...): ");
    int entry_to_delete;

    if (scanf("%d", &entry_to_delete) != 1) {
        printf("Invalid input.\n");
        while (getchar() != '\n'); // Clear the buffer
        free_list(head);
        return;
    }

    while (getchar() != '\n'); // Clear the buffer

    head = remove_node_at_position(head, entry_to_delete);

    save_linked_list("diary.txt", head);
    printf("Entry deleted!\n");

    free_list(head);
    
}


// === MAIN PROGRAM ===

/**
 * Main Program: orchastrate all features.
 */
int main() {
    int choice;

    while (1) {
        show_menu();

        if (scanf("%d", &choice) != 1) {
            printf("That's not a number. Try again.\n");
            while (getchar() != '\n'); // Clear the buffer
            continue;
        }

        getchar(); // Eat the newline left by scanf

        if (choice == 1) {
            handle_write_entry();
        } else if (choice == 2) {
            handle_view_entries();
        } else if (choice == 3) {
            handle_search_entries();
        } else if (choice == 4) {
            handle_delete_entry();
        } else if (choice == 5) {
            handle_edit_entry();
        } else if (choice == 6) {
            printf("Goodbye!\n");
            break;
        } else {
            printf("Invalid choice. Try again.\n");
        }
    }

    return 0;
}
