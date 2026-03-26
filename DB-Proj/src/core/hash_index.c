#define _CRT_SECURE_NO_WARNINGS
#include "../../include/hash_index.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static unsigned int hash_int(int value, int bucket_count) {
    /* Use prime multiplier for better distribution */
    unsigned int x = (unsigned int)value;
    x = ((x >> 16) ^ x) * 0x45d9f3bU;
    x = ((x >> 16) ^ x) * 0x45d9f3bU;
    x = (x >> 16) ^ x;
    return x % (unsigned int)bucket_count;
}

static HashNode *hash_node_create(int id, Person *person) {
    HashNode *node = malloc(sizeof(HashNode));
    if (!node) {
        return NULL;
    }

    node->id = id;
    node->person = person;
    node->next = NULL;
    return node;
}

/* Helper to find next power of 2 >= n */
static int next_power_of_2(int n) {
    if (n <= 1) return 1;
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    return n + 1;
}

HashIndex *hash_index_create(int bucket_count) {
    if (bucket_count <= 0) {
        bucket_count = 16;
    }

    HashIndex *index = malloc(sizeof(HashIndex));
    if (!index) {
        return NULL;
    }

    index->buckets = calloc((size_t)bucket_count, sizeof(HashNode *));
    if (!index->buckets) {
        free(index);
        return NULL;
    }

    index->bucket_count = bucket_count;
    index->size = 0;
    return index;
}

void hash_index_free(HashIndex *index) {
    if (!index) {
        return;
    }

    if (index->buckets) {
        for (int i = 0; i < index->bucket_count; i++) {
            HashNode *current = index->buckets[i];
            while (current) {
                HashNode *next = current->next;
                free(current);
                current = next;
            }
        }
    }

    free(index->buckets);
    free(index);
}

static int hash_index_insert_node(HashIndex *index, int id, Person *person) {
    unsigned int bucket = hash_int(id, index->bucket_count);
    HashNode *node = hash_node_create(id, person);
    if (!node) {
        return 0;
    }

    node->next = index->buckets[bucket];
    index->buckets[bucket] = node;
    index->size++;
    return 1;
}

static int hash_index_resize(HashIndex *index, int new_bucket_count) {
    HashNode **new_buckets = calloc((size_t)new_bucket_count, sizeof(HashNode *));
    if (!new_buckets) {
        return 0;
    }

    for (int i = 0; i < index->bucket_count; i++) {
        HashNode *current = index->buckets[i];
        while (current) {
            HashNode *next = current->next;
            unsigned int bucket = hash_int(current->id, new_bucket_count);
            current->next = new_buckets[bucket];
            new_buckets[bucket] = current;
            current = next;
        }
    }

    free(index->buckets);
    index->buckets = new_buckets;
    index->bucket_count = new_bucket_count;
    printf("Resized hash index to %d buckets (load factor: %.2f)\n", new_bucket_count, (double)index->size / (double)new_bucket_count);
    return 1;
}

int hash_index_build(HashIndex *index, Database *db) {
    if (!index || !db) {
        return 0;
    }

    /* Clear existing index */
    for (int i = 0; i < index->bucket_count; i++) {
        HashNode *current = index->buckets[i];
        while (current) {
            HashNode *next = current->next;
            free(current);
            current = next;
        }
        index->buckets[i] = NULL;
    }

    index->size = 0;

    /* Use resize-aware insert (will grow as needed) */
    for (int i = 0; i < db->count; i++) {
        if (!hash_index_insert(index, &db->records[i])) {
            printf("Failed to insert person %d into hash index\n", db->records[i].id);
            return 0;
        }
    }

    printf("Hash index built with %d entries in %d buckets (load factor: %.2f)\n", 
           index->size, index->bucket_count, (double)index->size / (double)index->bucket_count);
    return 1;
}

Person *hash_index_find_by_id(const HashIndex *index, int id) {
    if (!index || index->bucket_count <= 0) {
        return NULL;
    }

    unsigned int bucket = hash_int(id, index->bucket_count);
    HashNode *current = index->buckets[bucket];

    while (current) {
        if (current->id == id) {
            printf("Found person %d in bucket %d\n", id, bucket);
            return current->person;
        }
        current = current->next;
    }

    return NULL;
}

int hash_index_insert(HashIndex *index, Person *person) {
    if (!index || !person) {
        return 0;
    }

    double load_factor = (double)(index->size + 1) / (double)index->bucket_count;
    if (load_factor > 0.90) {
        int new_bucket_count = index->bucket_count * 2;
        if (!hash_index_resize(index, new_bucket_count)) {
            return 0;
        }
    }

    return hash_index_insert_node(index, person->id, person);
}

int hash_index_remove(HashIndex *index, int id) {
    if (!index || index->bucket_count <= 0) {
        return 0;
    }

    unsigned int bucket = hash_int(id, index->bucket_count);
    HashNode *current = index->buckets[bucket];
    HashNode *prev = NULL;

    while (current) {
        if (current->id == id) {
            if (prev) {
                prev->next = current->next;
            } else {
                index->buckets[bucket] = current->next;
            }

            free(current);
            index->size--;
            return 1;
        }

        prev = current;
        current = current->next;
    }

    return 0;
}

void hash_index_print_stats(const HashIndex *index) {
    if (!index) {
        printf("Hash index is NULL\n");
        return;
    }

    printf("Hash Index Statistics:\n");
    printf("  Bucket Count: %d\n", index->bucket_count);
    printf("  Entries: %d\n", index->size);
}

void hash_index_print_buckets(const HashIndex *index) {
    if (!index) {
        printf("Hash index is NULL\n");
        return;
    }

    printf("\n=== Hash Index Bucket Distribution ===\n");
    printf("Total buckets: %d | Total Entries: %d | Load Factor: %.2f\n", index->bucket_count, index->size, (double)index->size / (double)index->bucket_count);
    printf("----------------------------------------\n");

    int non_empty = 0;
    int max_chain = 0;

    for (int i = 0; i < index->bucket_count; i++) {
        if (index->buckets[i] == NULL) {
            continue;
        }

        non_empty++;
        printf("Bucket %d: ", i);

        HashNode *current = index->buckets[i];
        int chain_length = 0;

        while (current) {
            if (chain_length > 0) printf(" -> ");
            printf("[ID:%d, %s]", current->id, current->person->name);
            chain_length++;
            current = current->next;
        }

        if (chain_length > max_chain) max_chain = chain_length;
        printf(" (chain length: %d)\n", chain_length);
    }

    printf("----------------------------------------\n");
    printf("Non-empty buckets: %d | Empty buckets: %d | Max chain length: %d\n", non_empty, index->bucket_count - non_empty, max_chain);
    printf("========================================\n");
}