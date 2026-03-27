#ifndef BTREE_INDEX_H
#define BTREE_INDEX_H

#include "database.h"

#define BTREE_ORDER 5              // Each node can have up to 4 keys, 5 children

typedef struct BTreeNode {
    int *keys;                     // Sorted keys (ages, salaries, etc.)
    Person **values;               // Pointers to Person records (for leaves only)
    struct BTreeNode **children;   // Pointers to child nodes (for internal nodes)
    int key_count;                 // Number of keys in the node
    int is_leaf;                   // 1 if leaf, 0 if internal node
} BTreeNode;

typedef struct {
    BTreeNode *root;
    int order;                    // Max keys per node = order - 1
    int field_type;               // 0=age, 1=salary (which field we're indexing)
} BTreeIndex;

// API
BTreeIndex *btree_create(int order, int field_type);
void btree_free(BTreeIndex *index);

// Insert key + person pointer
int btree_insert(BTreeIndex *index, int key, Person *person);

// Range query: find all persons where min_key <= key <= max_key
typedef struct {
    Person **results;
    int count;
} RangeResult;

RangeResult btree_range_query(BTreeIndex *index, int min_key, int max_key);
void range_result_free(RangeResult *result);

// Rebuild from database
int btree_build_from_database(BTreeIndex *tree, const Database *db);

// Debug/stats
void btree_print_structure(BTreeIndex *tree);
int btree_get_height(BTreeIndex *tree);

#endif