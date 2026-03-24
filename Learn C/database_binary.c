#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct {
    int id;
    char name[100];
    int age;
    double salary;
} Person;

typedef struct {
    Person *records;  // Pointer to array of Person structs
    int count;        // Current number of records
    int capacity;     // Space allocated for records
} Database;

typedef struct PersonNode {
    Person data;
    struct PersonNode *next;
} PersonNode;

// Header for binary file
typedef struct {
    char magic[5]; // "DB01"
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
int append_record_to_file(const char *filename, Person record);
Person get_record_by_position(const char *filename, int position);
Database* db_create(int initial_capacity);
Database* db_load_from_file(const char *filename);
int db_search_by_name(Database *db, const char *name);
int db_filter_by_age_gt(Database *db, int min_age, Person *results);
int db_binary_search_by_id(Database *db, int target_id);
int compare_by_age(const void *a, const void *b);
int compare_by_name(const void *a, const void *b);
int compare_by_salary_desc(const void *a, const void *b);
void db_sort_by_age(Database *db);
void db_sort_by_name(Database *db);
void db_sort_by_salary_desc(Database *db);
void db_sort_by_id(Database *db);
void compare_search_performance(Database *db);
void db_display(Database *db);
void db_memory_stats(Database *db);
int db_add_record(Database *db, Person record);

int main(void) {
    // Load database into array (Phase 4 approach)
    Database *db = db_load_from_file("people.db");
    
    if (db == NULL) {
        printf("Failed to create database!\n");
        return 1;
    }

    int choice;

    while (1) {
        show_menu();

        if (scanf("%d", &choice) != 1) {
            printf("Invalid input. Try again.\n");
            while (getchar() != '\n');
            continue;
        }
        getchar();

        if (choice == 1) {
            // Display all records
            db_display(db);

        } else if (choice == 2) {
            // Sort by age
            db_sort_by_age(db);
            db_display(db);

        } else if (choice == 3) {
            // Sort by name
            db_sort_by_name(db);
            db_display(db);

        } else if (choice == 4) {
            // Sort by salary (descending)
            db_sort_by_salary_desc(db);
            db_display(db);

        } else if (choice == 5) {
            // Search by name
            char name[100];
            printf("Enter name to search: ");
            fgets(name, sizeof(name), stdin);
            name[strcspn(name, "\n")] = '\0';
            
            int idx = db_search_by_name(db, name);
            if (idx >= 0) {
                printf("Details: Age=%d, Salary=%.2f\n",
                       db->records[idx].age, db->records[idx].salary);
            }

        } else if (choice == 6) {
            // Filter by age
            int min_age;
            printf("Enter minimum age: ");
            scanf("%d", &min_age);
            getchar();
            
            Person *filtered = malloc(sizeof(Person) * db->count);
            if (filtered == NULL) {
                printf("Memory allocation failed!\n");
                continue;
            }
            
            int filtered_count = db_filter_by_age_gt(db, min_age, filtered);
            printf("\nFound %d people older than %d:\n", filtered_count, min_age);
            for (int i = 0; i < filtered_count; i++) {
                printf("  %s, Age=%d, Salary=%.2f\n",
                       filtered[i].name, filtered[i].age, filtered[i].salary);
            }
            free(filtered);

        } else if (choice == 7) {
            // Binary search by ID
            int target_id;
            printf("Enter ID to search: ");
            scanf("%d", &target_id);
            getchar();
            
            int idx = db_binary_search_by_id(db, target_id);
            if (idx >= 0) {
                printf("Found: %s, Age=%d, Salary=%.2f\n",
                       db->records[idx].name, db->records[idx].age, db->records[idx].salary);
            }

        } else if (choice == 8) {
            // Manual comparison of two records
            if (db->count < 2) {
                printf("Need at least 2 records to compare.\n");
                continue;
            }
            
            int idx1, idx2;
            printf("Enter first record index (0-%d): ", db->count - 1);
            scanf("%d", &idx1);
            printf("Enter second record index (0-%d): ", db->count - 1);
            scanf("%d", &idx2);
            getchar();
            
            if (idx1 < 0 || idx1 >= db->count || idx2 < 0 || idx2 >= db->count) {
                printf("Invalid indices!\n");
                continue;
            }
            
            int cmp = compare_by_age(&db->records[idx1], &db->records[idx2]);
            printf("\nCompare %s (age %d) vs %s (age %d):\n",
                   db->records[idx1].name, db->records[idx1].age,
                   db->records[idx2].name, db->records[idx2].age);
            printf("Result: %s\n",
                   cmp < 0 ? "First is younger" : 
                   cmp > 0 ? "First is older" : 
                   "Same age");

        } else if (choice == 9) {
            db_sort_by_id(db);
            db_display(db);

        } else if (choice == 10) {
            compare_search_performance(db);
        } else if (choice == 11) {
            // Add new record
            Person new_person = {0};
            new_person.id = db->count + 1;
            
            printf("Enter name: ");
            fgets(new_person.name, sizeof(new_person.name), stdin);
            new_person.name[strcspn(new_person.name, "\n")] = '\0';
            
            printf("Enter age: ");
            scanf("%d", &new_person.age);
            
            printf("Enter salary: ");
            scanf("%lf", &new_person.salary);
            getchar();
            
            if (db_add_record(db, new_person)) {
                printf("Record added successfully!\n");
                db_memory_stats(db);
            }
        } else if (choice == 12) {
            // Show memory stats
            db_memory_stats(db);
            
        } else if (choice == 13) {
            printf("Goodbye!\n");
            break;
         
        } else {
            printf("Invalid choice. Try again.\n");
        }
    }

    // Cleanup
    free(db->records);
    free(db);
    return 0;
}

void show_menu(void) {
    printf("\n=== Database Menu ===\n");
    printf("1. Display all records\n");
    printf("2. Sort by age\n");
    printf("3. Sort by name\n");
    printf("4. Sort by salary (descending)\n");
    printf("5. Search by name\n");
    printf("6. Filter by age\n");
    printf("7. Binary search by ID\n");
    printf("8. Compare two records\n");
    printf("9. Sort by ID\n");
    printf("10. Compare search performance\n");
    printf("11. Add new record\n");
    printf("12. Show memory stats\n");
    printf("13. Quit\n");
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
    append_record_to_file("people.db", new_person);
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

// Append a single record to the database file
// Returns 1 on success, 0 on failure
int append_record_to_file(const char *filename, Person record) {
    FILE *file = fopen(filename, "r+b");  // Open for reading AND writing

    if (file == NULL) {
        printf("Creating new database file...\n");
        file = fopen(filename, "w+b"); // Create new file
        if (file == NULL) {
            perror("Error creating new file");
            return 0;
        }
        
        // Write initial header
        DatabaseHeader header = {"PEDB", 1, 0};
        fwrite(&header, sizeof(DatabaseHeader), 1, file);

        // Seek back to beginning before reading
        fseek(file, 0, SEEK_SET);
    }

    // Step 1: Read and validate the header
    DatabaseHeader header;
    if (fread(&header, sizeof(DatabaseHeader), 1, file) != 1) {
        printf("Error reading header!\n");
        fclose(file);
        return 0;
    }

    // Step 2: Check magic number
    if (strcmp(header.magic, "PEDB") != 0) {
        printf("Invalid database file!\n");
        fclose(file);
        return 0;
    }

    // Step 3: Seek to the end of the file (past all existing records)
    if (fseek(file, 0, SEEK_END) != 0) {
        printf("Error seeking to end!\n");
        fclose(file);
        return 0;
    }

    // Step 4: Write the new record at the end
    if (fwrite(&record, sizeof(Person), 1, file) != 1) {
        printf("Error writing record!\n");
        fclose(file);
        return 0;
    }

    // Step 5: Update the header with the new record count
    header.record_count++;

    // Seek back to the beginning to overwrite the header
    if (fseek(file, 0, SEEK_SET) != 0) {
        perror("Error seeking to beginning!");
        fclose(file);
        return 0;
    }

    // Write the updated header
    if (fwrite(&header, sizeof(DatabaseHeader), 1, file) != 1) {
        perror("Error updating header!");
        fclose(file);
        return 0;
    }

    fclose(file);
    printf("Record appended successfully. File now has %d records.\n", header.record_count);
    return 1;
}

// Retrieve a single record by its position (1-indexed)
// Returns a Person struct; check record, id to see if it's valid
// If position is invalid, returns a zeroed Person struct
Person get_record_by_position(const char *filename, int position) {
    Person empty_record = {0};  // Initialize to zeros (invalid)

    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Error opening file");
        return empty_record;
    }

    // Step 1: Read and validate the header
    DatabaseHeader header;
    if (fread(&header, sizeof(DatabaseHeader), 1, file) != 1) {
        printf("Error reading header!\n");
        fclose(file);
        return empty_record;
    }

    // Step 2: Validate position (1-indexed)
    if (position < 1 || position > header.record_count) {
        printf("Invalid position %d. File has %d records.\n", position, header.record_count);
        fclose(file);
        return empty_record;
    }

    // Step 3: Calculate the byte offset for this record
    // Header is at position 0
    // Record 1 starts at: sizeof(DatabaseHeader)
    // Record N starts at: sizeof(DatabaseHeader) + (N-1) * sizeof(Person)
    long record_offset = sizeof(DatabaseHeader) + (position - 1) * sizeof(Person);

    // Step 4: Seek to that position
    if (fseek(file, record_offset, SEEK_SET) != 0) {
        perror("Error seeking to record!");
        fclose(file);
        return empty_record;
    }

    // Step 5: Read the record
    Person record;
    if (fread(&record, sizeof(Person), 1, file) != 1) {
        printf("Error reading record %d!\n", position);
        fclose(file);
        return empty_record;
    }

    fclose(file);
    printf("Retrieved record at position %d: ID=%d, Name=%s\n", position, record.id, record.name);

    return record;
}

// Create a new empty database with initial capacity
Database* db_create(int initial_capacity) {
    Database* db = malloc(sizeof(Database));
    if (db == NULL) {
        printf("Failed to allocate database struct!\n");
        return NULL;
    }

    db->records = malloc(sizeof(Person) * initial_capacity);
    if (db->records == NULL) {
        printf("Failed to allocate memory for records!\n");
        free(db);  // Free the database atruct if array fails
        return NULL;
    }

    db->count = 0;
    db->capacity = initial_capacity;

    return db;
}

// Resize database: grow capacity when needed
int resize_database(Database *db, int new_capacity) {
    if (new_capacity <= db->capacity) {
        printf("Error: cannot shrink below %d records!\n", db->capacity);
        return 0;
    }

    Person *new_records = realloc(db->records, sizeof(Person) * new_capacity);
    if (new_records == NULL) {
        printf("Memory allocation failed! Keeping old size.\n");
        return 0;
    }

    db->records = new_records;
    db->capacity = new_capacity;
    printf("Database resized to %d records.\n", new_capacity);
    return 1;
}

// Add a record to the database
int db_add_record(Database *db, Person record) {
    // Check if we need to resize
    if (db->count >= db->capacity) {
        int new_capacity = db->capacity * 2;  // Grow by 2x
        if (!resize_database(db, new_capacity)) {
            printf("failed to add record: out of memory\n");
            return 0;
        }
    }

    // Add the record
    db->records[db->count++] = record;
    printf("Record added successfully. File now has %d/%d records.\n", db->count, db->capacity);
    return 1;
}

// Load database from binary file into memory array
Database* db_load_from_file(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        printf("No database file found. Starting fresh.\n");
        return db_create(10);  //Create empty database with capacity 10
    }

    // Read header
    DatabaseHeader header;
    if (fread(&header, sizeof(DatabaseHeader), 1, file) != 1) {
        printf("Invalid database file!\n");
        fclose(file);
        return db_create(10);
    }

    // Validate magic number
    if (strcmp(header.magic, "PEDB") != 0) {
        printf("Corrupted database file!\n");
        fclose(file);
        return db_create(10);
    }

    // Create database with capacity = record count + 10% buffer
    int capacity = header.record_count + (header.record_count / 10) + 1;
    Database *db = db_create(capacity);

    // Read all records into array
    for (int i = 0; i < header.record_count; i++) {
        Person person;
        if (fread(&person, sizeof(Person), 1, file) != 1) {
            printf("Error reading record %d!\n", i + 1);
            break;
        }
        db->records[db->count++] = person;  // Add to array
    }

    fclose(file);
    printf("Database loaded from %s with %d records.\n", filename, db->count);
    return db;
}

// Linear search: find first person with matching name
// Returns index if found, -1 if not found
int db_search_by_name(Database *db, const char *name) {
    for (int i = 0; i < db->count; i++) {
        if (strcmp(db->records[i].name, name) == 0) {
            printf("Found '%s' at index %d.\n", name, i);
            return i;
        }
    }
    printf("'%s' not found.\n", name);
    return -1;
}

// Linear search: find all persons with age greater than threshold
// Returns count of matching records
int db_filter_by_age_gt(Database *db, int min_age, Person *results) {
    int count = 0;
    for (int i = 0; i < db->count; i++) {
        if (db->records[i].age > min_age) {
            results[count++] = db->records[i];
        }
    }
    return count;
}

// Binary search: find person ID (array MUST be sorted by ID!)
// Returns index if found, -1 if not found
int db_binary_search_by_id(Database *db, int target_id) {
    int left = 0;
    int right = db->count - 1;

    while (left <= right) {
        // Calculate middle, avoiding integer overflow
        int mid = left + (right - left) / 2;
        int mid_id = db->records[mid].id;

        if (mid_id == target_id) {
            printf("Found ID %d at index %d.\n", target_id, mid);
            return mid;
        } else if (mid_id < target_id) {
            // Target is in right half
            left = mid + 1;
        } else {
            // Target is in left half
            right = mid - 1;
        }
    }

    printf("ID %d not found.\n", target_id);
    return -1;
}

// Compare two Person structs by age (ascending)
int compare_by_age(const void *a, const void *b) {
    // Cast void pointers to Person pointers
    const Person *person_a = (const Person *)a;
    const Person *person_b = (const Person *)b;

    // Return age difference
    // If age_a < age_b, returns negative
    // If age_a > age_b, returns positive
    return person_a->age - person_b->age;
}

// Compare two Person structs by name (alphabetical)
int compare_by_name(const void *a, const void *b) {
    const Person *person_a = (const Person *)a;
    const Person *person_b = (const Person *)b;

    // strcmp returns negative, zero, or positive
    return strcmp(person_a->name, person_b->name);
}

// Compare by salary (descending - notice the swap!)
int compare_by_salary_desc(const void *a, const void *b) {
    const Person *person_a = (const Person *)a;
    const Person *person_b = (const Person *)b;

    // Swap a and b to get descending order
    if (person_a->salary < person_b->salary) return 1;
    if (person_a->salary > person_b->salary) return -1;
    return 0;
}

// Compare by ID (ascending) - for binary search to work properly
int compare_by_id(const void *a, const void *b) {
    const Person *person_a = (const Person *)a;
    const Person *person_b = (const Person *)b;
    return person_a->id - person_b->id;
}

// Sort database by age (ascending)
void db_sort_by_age(Database *db) {
    qsort(db->records, db->count, sizeof(Person), compare_by_age);
    printf("Database sorted by age.\n");
}

// Sort database by name (alphabetical)
void db_sort_by_name(Database *db) {
    qsort(db->records, db->count, sizeof(Person), compare_by_name);
    printf("Database sorted by name.\n");
}

void db_sort_by_salary_desc(Database *db) {
    qsort(db->records, db->count, sizeof(Person), compare_by_salary_desc);
    printf("Database sorted by salary (descending).\n");
}

void db_sort_by_id(Database *db) {
    qsort(db->records, db->count, sizeof(Person), compare_by_id);
    printf("Database sorted by ID.\n");
}

void compare_search_performance(Database *db) {
    if (db->count <2) {
        printf("Need at least 2 records to compare.\n");
        return;
    }

    // Pick a record to search for
    int target_id = db->records[db->count / 2].id; // Middle record
    printf("Searching for ID %d...\n", target_id);
    printf("Database size: %d records\n", db->count);

    // Manual LINEAR search loop
    clock_t start_linear = clock();
    int linear_result = -1;
    for (int i = 0; i < db->count; i++) {
        if (db->records[i].id == target_id) {
            linear_result = i;
            break;
        }
    }
    clock_t end_linear = clock();
    double linear_time = ((double)(end_linear - start_linear)) / CLOCKS_PER_SEC * 1000000; // Microseconds

    // Binary search
    db_sort_by_id(db);  // Must be sorted by ID for binary search to work
    clock_t start_binary = clock();
    int binary_result = db_binary_search_by_id(db, target_id);
    clock_t end_binary = clock();
    double binary_time = ((double)(end_binary - start_binary)) / CLOCKS_PER_SEC * 1000000; // Microseconds

    printf("\n=== Search Performance ===\n");
    printf("Linear Search (manual loop):  %.2f microseconds\n", linear_time);
    printf("Binary Search (sorted data):  %.2f microseconds\n", binary_time);
    printf("Speed ratio: %.1fx faster\n", linear_time / binary_time);
}

void db_display(Database *db) {
    if (db->count == 0) {
        printf("Database is empty.\n");
        return;
    }
    
    printf("\n");
    printf("┌────┬──────────────┬─────┬──────────┐\n");
    printf("│ ID │ Name         │ Age │ Salary   │\n");
    printf("├────┼──────────────┼─────┼──────────┤\n");
    
    for (int i = 0; i < db->count; i++) {
        printf("│ %-2d │ %-12s │ %3d │ %8.2f │\n",
            db->records[i].id,
            db->records[i].name,
            db->records[i].age,
            db->records[i].salary);
    }
    printf("└────┴──────────────┴─────┴──────────┘\n");
}

// Report memory usage
void db_memory_stats(Database *db) {
    printf("\n=== Database Memory Stats ===\n");
    printf("Records in use (count):    %d\n", db->count);
    printf("Space allocated (capacity): %d\n", db->capacity);
    printf("Bytes per record:          %zu\n", sizeof(Person));
    printf("Total allocated:           %ld bytes (%.2f KB)\n", 
           (long)(db->capacity * sizeof(Person)),
           (db->capacity * sizeof(Person)) / 1024.0);
    printf("Actual data:               %ld bytes (%.2f KB)\n",
           (long)(db->count * sizeof(Person)),
           (db->count * sizeof(Person)) / 1024.0);
    printf("Unused space:              %ld bytes (%.1f%%)\n",
           (long)((db->capacity - db->count) * sizeof(Person)),
           100.0 * (db->capacity - db->count) / db->capacity);
    printf("Load factor:               %.1f%%\n", 
           100.0 * db->count / db->capacity);
    printf("==============================\n\n");
}