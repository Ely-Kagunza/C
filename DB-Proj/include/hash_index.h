#ifndef HASH_INDEX_H
#define HASH_INDEX_H

#include "database.h"

typedef struct HashNode {
    int id;
    Person *person;
    struct HashNode *next;
} HashNode;

typedef struct {
    HashNode **buckets;
    int bucket_count;
    int size;
} HashIndex;

HashIndex *hash_index_create(int bucket_count);
void hash_index_free(HashIndex *index);

int hash_index_build(HashIndex *index, Database *db);
Person *hash_index_find_by_id(const HashIndex *index, int id);
int hash_index_insert(HashIndex *index, Person *person);
int hash_index_remove(HashIndex *index, int id);

void hash_index_print_stats(const HashIndex *index);
void hash_index_print_buckets(const HashIndex *index);

#endif