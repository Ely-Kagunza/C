/** diary.c - Phase 1: basic Structure and File Writing
 * We are learning how to open a file, write to it, and close it manually.
 */

# include <stdio.h> // Standard Input/Output (for printf, fopen, etc.)
# include <stdlib.h> // Standard Library (for exit codes)
# include <string.h> // For strcmp
# include <time.h> // For timestamp functions

// === STRUCT DEFINITIONS ===
typedef struct {
    char timestamp[200]; // The timestamp line (e.g., "--- ENTRY: Fri Mar 6 21:37:16 2026")
    char *content; // All the entry text (dynamically allocated)
    int content_size; // How much memory we allocated for content
} DiaryEntry;

// === FUNCTION PROTOTYPES (declarations) ===
// Core helpers (file operations)
DiaryEntry *load_entries(const char *filename, int *count);
void display_entries(DiaryEntry *entries, int count);
void save_entries(const char *filename, DiaryEntry *entries, int count);
void free_entries(DiaryEntry *entries, int count);

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
    printf("5. Quit\n");
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
    FILE *file = fopen("diary.txt", "r");

    if (file == NULL) {
        printf("No entries yet.\n");
        return;
    }

    int line_no = 1;
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        printf("%d: %s", line_no, buffer);
        line_no++;
    }
    fclose(file);
}

/**
 * Handle searching entries by keyword.
 */
void handle_search_entries(void) {
    printf("\nEnter search keyword: ");

    char buffer[1024];
    fgets(buffer, sizeof(buffer), stdin);
    buffer[strcspn(buffer, "\n")] = '\0';

    FILE *file = fopen("diary.txt", "r");
    if (file == NULL) {
        printf("No entries to search.\n");
        return;
    }

    int line_no = 1;
    int matches = 0;
    char line[1024];

    printf("\n--- SEARCH RESULTS FOR '%s' ---\n", buffer);

    while (fgets(line, sizeof(line), file) != NULL) {
        if (strstr(line, buffer) != NULL) {
            printf("%d: %s", line_no, line);
            matches++;
        }
        line_no++;
    }

    printf("\nFound %d match(es).\n", matches);
    fclose(file);
}

/**
 * Handle deleting an entry.
 */
 void handle_delete_entry(void) {
    int num_entries = 0;
    DiaryEntry *entries = load_entries("diary.txt", &num_entries);

    if (num_entries == 0) {
        printf("No entries to delete.\n");
        return;
    }

    display_entries(entries, num_entries);

    printf("\nEnter entry number to delete (1-%d): ", num_entries);
    int entry_to_delete;

    if (scanf("%d", &entry_to_delete) != 1) {
        printf("Invalid input.\n");
        while (getchar() != '\n'); // Clear the buffer
    } else if (entry_to_delete < 1 || entry_to_delete > num_entries) {
        printf("Invalid entry number.\n");
    } else {
        // Remove the entry by shifting all later entries down
        for (int i = entry_to_delete - 1; i < num_entries - 1; i++) {
            // Copy struct (timestamp and content pointer)
            entries[i] = entries[i + 1];
        }
        num_entries--;

        // Save the modified entries
        save_entries("diary.txt", entries, num_entries);
        printf("Entry deleted!\n");
    }

    // Always free memory, whether delete succeeded or not
    free_entries(entries, num_entries);
    
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
            printf("Goodbye!\n");
            break;
        } else {
            printf("Invalid choice. Try again.\n");
        }
    }

    return 0;
}