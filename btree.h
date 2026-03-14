#ifndef BTREE_H
#define BTREE_H

#include "person.h"

// B-Tree Node
typedef struct BTreeNode {
    int *keys;                      // Array of keys (sorted)
    struct BTreeNode **children;    // Array of child pointers
    void **values;                  // Array of values (Person pointers)
    int key_count;                  // Number of keys in use
    int is_leaf;                    // 1 if leaf node, 0 if internal node
    int order;                      // Max keys per node (t)
} BTreeNode;

// B-Tree structure
typedef struct {
    BTreeNode *root;
    int order;                      // Typically 3-5 for small trees
    int total_keys;                 // Total kesy in tree
} BTree;

// B-Tree operations
BTree *btree_create(int order);
void btree_free(BTree *bt);

void btree_insert(BTree *bt, int key, Person *value);
Person *btree_search(BTree *bt, int key);

// Range query: find all keys between min and max
int btree_range_search(BTree *bt, int min_key, int max_key, Person **results);

void btree_display_stats(BTree *bt);

#endif // BTREE_H