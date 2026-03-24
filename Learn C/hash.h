#ifndef HASH_H
#define HASH_H

#include "person.h"

// Node in the hash table's linked list (for collision resolution via chaining)
typedef struct HashNode
{
    int key;               // The ID (key for lookup)
    Person data;           // The person record (value)
    struct HashNode *next; // Pointer to the next node (for chaining collisions)
} HashNode;

// Hash table structure
typedef struct {
    HashNode **buckets;     // Array of pointers to linked lists (buckets)
    int size;               // Number of buckets in the table (capacity)
    int count;              // Number of records in the table actually stored
    float max_load_factor;  // Maximum load factor before resizing (0.75)
} HashTable;

// Public API
HashTable *hash_create(int initial_size);
void hash_free(HashTable *ht);

int hash_insert(HashTable *ht, int key, Person record);
Person *hash_lookup(HashTable *ht, int key);
int hash_delete(HashTable *ht, int key);

void hash_display_stats(HashTable *ht);
int hash_get_count(HashTable *ht);

#endif // HASH_H