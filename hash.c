#include <stdio.h>
#include <stdlib.h>
#include "hash.h"


// ============================================================================
// HASH FUNCTION - Converts an integer key into a bucket index
// ============================================================================
// 
// WHY THIS MATTERS:
// A good hash function distributes keys evenly across buckets.
// We use FNV (Fowler-Noll-Vo) hash constant, a well-known technique.
//
// PYTHON COMPARISON:
// Python's hash() function does the same thing internally for dict keys.
// We're implementing the core idea that Python hides from us.
//
// Example:
//   hash_function(42, 100) might return 42
//   hash_function(142, 100) might return 42 (collision!)
//   hash_function(2, 100) might return 2 (no collision)
// ============================================================================
static int hash_function(int key, int table_size)
{
    // FVN hashconstant: proven to distribute integers well
    return (key * 2654435761) % table_size;
}

// ============================================================================
// CREATE - Allocate and initialize an empty hash table
// ============================================================================
//
// WHAT HAPPENS:
// 1. Allocate the HashTable struct itself
// 2. Allocate the buckets array (initially all NULL)
// 3. Initialize metadata (size, count, load factor)
//
// PYTHON COMPARISON:
// Similar to creating an empty dict: my_dict = {}
// Python allocates ~8 buckets initially; we allocate 'initial_size'
// ============================================================================
HashTable *hash_create(int initial_size)
{
    // Sanity check
    if (initial_size <= 0)
    {
        printf("Error: Hash table size must be > 0\n");
        return NULL;
    }

    // Allocate the hash table struct itself
    HashTable *ht = malloc(sizeof(HashTable));
    if (ht == NULL)
    {
        printf("Failed to allocate hash table!\n");
        return NULL;
    }

    // Allocate the buckets array (array of pointers)
    // Each bucket is initially NULL (empty linked list)
    ht->buckets = malloc(sizeof(HashNode *) * initial_size);
    if (ht->buckets == NULL)
    {
        printf("Failed to allocate buckets array!\n");
        free(ht);
        return NULL;
    }

    // Initialize all buckets to NULL
    for (int i = 0; i < initial_size; i++)
    {
        ht->buckets[i] = NULL;
    }

    ht->size = initial_size;
    ht->count = 0;
    ht->max_load_factor = 0.75;  // Resize when 75% full

    printf("Hash table created with %d buckets\n", initial_size);
    return ht;
}

// ============================================================================
// FREE - Clean up all memory (deallocate hash table)
// ============================================================================
//
// WHAT HAPPENS:
// 1. Walk through each bucket
// 2. For each bucket that has a linked list, free all nodes
// 3. Free the buckets array itself
// 4. Free the HashTable struct
//
// CRITICAL ERROR CHECKING:
// Every malloc() must have a matching free().
// Missing this = memory leak (memory never returned to OS)
// ============================================================================
void hash_free(HashTable *ht)
{
    if (ht == NULL)
        return;

    // Iterate through each bucket
    for (int i = 0; i < ht->size; i++)
    {
        // Walk the linked list in this bucket
        HashNode *current = ht->buckets[i];
        while (current != NULL)
        {
            HashNode *temp = current;
            current = current->next;
            free(temp);  // Free each node
        }
    }

    // Free the buckets array and the table itself
    free(ht->buckets);
    free(ht);
  }

// ============================================================================
// RESIZE - Double the table size and rehash all entries
// ============================================================================
//
// WHY WE RESIZE:
// If load factor = count / size > 0.75, there are too many collisions.
// Doubling the size reduces collisions and keeps lookups fast O(1).
//
// WHAT HAPPENS:
// 1. Create a new larger table (double the size)
// 2. Rehash every entry into the new table
//   (key % 100 puts it in different bucket than key % 50)
// 3. Free the old table
// 4. Replace buckets with new buckets
//
// PYTHON COMPARISON:
// Python dicts automatically resize internally. We do it explicitly.
// This is complex but necessary for O(1) performance.
// ============================================================================
static int hash_resize(HashTable *ht, int new_size)
{
    printf("Resizing hash table from %d to %d buckets...\n", ht->size, new_size);

    // Create new buckets array
    HashNode **new_buckets = malloc(sizeof(HashNode *) * new_size);
    if (new_buckets == NULL)
    {
        printf("Failed to allocate new buckets during resize!\n");
        return 0;
    }

    // Initialize all new buckets to NULL
    for (int i = 0; i < new_size; i++)
    {
        new_buckets[i] = NULL;
    }

    // Rehash: move every entry from old table to new table
    for (int i = 0; i < ht->size; i++)
    {
        HashNode *current = ht->buckets[i];
        while (current != NULL)
        {
            // Calculate new bucket for this key
            int new_bucket = hash_function(current->key, new_size);

            // Save the next node before we modify the linked list
            HashNode *next = current->next;

            // Insert at head of new bucket's linked list
            current->next = new_buckets[new_bucket];
            new_buckets[new_bucket] = current;

            current = next;
        }
    }

    // Replace old buckets with new buckets
    free(ht->buckets);
    ht->buckets = new_buckets;
    ht->size = new_size;

    printf("Hash table resized successfully\n");
    return 1;
}

// ============================================================================
// INSERT - Add a new key-value pair (or update existing)
// ============================================================================
//
// ALGORITHM:
// 1. Check load factor; resize if needed
// 2. Compute hash to find bucket
// 3. Search linked list in that bucket for key
//    - If found: update the data
//    - If not found: insert new node at head
// 4. Increment count if new entry
//
// TIME COMPLEXITY:
// Best case: O(1) - hash to bucket, no collision, insert at head
// Average case: O(1 + k) where k is avg chain length (~ 1 if load factor good)
// Worst case: O(n) - all entries hash to same bucket (bad luck with hash function)
// ============================================================================
int hash_insert(HashTable *ht, int key, Person data)
{
    if (ht == NULL)
    {
        printf("Error: Hash table is NULL\n");
        return 0;
    }

    // Check load factor and resize if necessary
    float load_factor = (float)ht->count / ht->size;
    if (load_factor > ht->max_load_factor)
    {
        if (!hash_resize(ht, ht->size * 2))
        {
            return 0;  // Resize failed
        }
    }

    // Compute bucket index
    int bucket = hash_function(key, ht->size);

    // Check if key already exists (search linked list)
    HashNode *current = ht->buckets[bucket];
    while (current != NULL)
    {
        if (current->key == key)
        {
            // Key found: update the data
            current->data = data;
            printf("Record ID %d updated in hash table\n", key);
            return 1;
        }
        current = current->next;
    }

    // Key not found: create new node and insert at head
    HashNode *new_node = malloc(sizeof(HashNode));
    if (new_node == NULL)
    {
        printf("Failed to allocate memory for new node!\n");
        return 0;
    }

    new_node->key = key;
    new_node->data = data;
    new_node->next = ht->buckets[bucket];
    ht->buckets[bucket] = new_node;
    ht->count++;

    printf("Record ID %d inserted into hash table\n", key);
    return 1;
}

// ============================================================================
// LOOKUP - Find a record by key (returns pointer to Person or NULL)
// ============================================================================
//
// ALGORITHM:
// 1. Compute hash to find bucket
// 2. Walk linked list in bucket until key found or list ends
// 3. Return pointer to data if found, NULL otherwise
//
// TIME COMPLEXITY: O(1) average, O(n) worst case
//
// PYTHON EQUIVALENT:
// person = people_by_id.get(42)  # Returns value or None
// We do: person = hash_lookup(ht, 42)  // Returns pointer or NULL
//
// CRITICAL NOTE:
// Returns a POINTER to the Person inside the hash table.
// Don't modify this data without updating the hash table!
// ============================================================================
Person *hash_lookup(HashTable *ht, int key)
{
    if (ht == NULL)
    {
        printf("Error: Hash table is NULL\n");
        return NULL;
    }

    // Compute bucket index
    int bucket = hash_function(key, ht->size);

    // Walk linked list in this bucket
    HashNode *current = ht->buckets[bucket];
    while (current != NULL)
    {
        if (current->key == key)
        {
            // Found it!
            return &current->data;
        }
        current = current->next;
    }
    
    // Not found
    return NULL;
}

// ============================================================================
// DELETE - Remove a record by key
// ============================================================================
//
// ALGORITHM:
// 1. Compute hash to find bucket
// 2. Walk linked list, find node with matching key
// 3. Unlink the node from the list
// 4. Free the node memory
// 5. Decrement count
//
// TRICKY PART:
// To remove a node from a singly-linked list, we need the previous node.
// So we keep track of 'prev' as we walk.
// ============================================================================
int hash_delete(HashTable *ht, int key)
{
    if (ht == NULL)
    {
        printf("Error: Hash table is NULL\n");
        return 0;
    }

    // Compute bucket index
    int bucket = hash_function(key, ht->size);

    // Walk linked list, keep track of previous node
    HashNode *current = ht->buckets[bucket];
    HashNode *prev = NULL;

    while (current != NULL)
    {
        if (current->key == key)
        {
            // Found it! Unlink and free
            if (prev == NULL)
            {
                // Node is at head of list
                ht->buckets[bucket] = current->next;
            }
            else
            {
                // Node is in middle or end
                prev->next = current->next;
            }

            free(current);
            ht->count--;
            printf("Record ID %d deleted from hash table\n", key);
            return 1;
        }

        prev = current;
        current = current->next;
    }

    // Not found
    printf("Record ID %d not found in hash table\n", key);
    return 0;
}

// ============================================================================
// STATISTICS - Display hash table state
// ============================================================================
//
// PYTHON COMPARISON:
// len(my_dict)  tells you how many items
// We provide more detail: load factor, collisions, etc.
// ============================================================================
void hash_display_stats(HashTable *ht)
{
    if (ht == NULL)
        return;

    float load_factor = (float)ht->count / ht->size;
    int collision_buckets = 0;
    int max_chain_length = 0;

    // Count buckets with collisions and longest chain
    for (int i = 0; i < ht->size; i++)
    {
        int chain_length = 0;
        HashNode *current = ht->buckets[i];
        while (current != NULL)
        {
            chain_length++;
            current = current->next;
        }

        if (chain_length > 1)
            collision_buckets++;
        if (chain_length > max_chain_length)
            max_chain_length = chain_length;
    }

    printf("\n=== Hash Table Statistics ===\n");
    printf("Buckets:              %d\n", ht->size);
    printf("Records stored:       %d\n", ht->count);
    printf("Load factor:          %.2f (threshold: %.2f)\n", load_factor, ht->max_load_factor);
    printf("Buckets with collisions: %d\n", collision_buckets);
    printf("Longest collision chain: %d\n", max_chain_length);
    printf("Avg records per bucket: %.2f\n", (float)ht->count / ht->size);
    printf("==============================\n\n");
}

int hash_get_count(HashTable *ht)
{
    return ht == NULL ? 0 : ht->count;
}   