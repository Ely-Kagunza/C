#include <stdio.h>
#include <stdlib.h>
#include "btree.h"

// ============================================================================
// B-TREE BASICS
// ============================================================================
//
// WHAT IS A B-TREE?
// A balanced search tree optimized for disk I/O and range queries.
//
// PROPERTIES:
// - All leaves at same depth (balanced)
// - Keys sorted at each node
// - Multiple keys per node (order determines max)
// - O(log n) search, insert, delete
// - O(log n) range queries
//
// PYTHON EQUIVALENT:
// Python's bisect module uses binary search on sorted lists.
// B-trees extend this to handle large datasets efficiently.
//
// WHY NOT JUST BINARY SEARCH?
// Binary search: O(log n) for exact match, O(n) for range
// B-tree: O(log n) for exact match, O(log n) for range start + O(k) for k results
//
// REAL WORLD:
// SQLite indexes use B-trees
// Database indexes are B-trees
// File systems use B-trees
//

// ============================================================================
// CREATE - Allocate a new B-tree
// ============================================================================
BTree *btree_create(int order)
{
    if (order < 3)
    {
        printf("Error: B-tree order must be at least 3\n");
        return NULL;
    }

    BTree *bt = malloc(sizeof(BTree));
    if (bt == NULL)
    {
        printf("Failed to allocate B-tree\n");
        return NULL;
    }

    // Create root node (empty leaf)
    BTreeNode *root = malloc(sizeof(BTreeNode));
    if (root == NULL)
    {
        printf("Failed to allocate root node\n");
        free(bt);
        return NULL;
    }

    root->keys = malloc(sizeof(int) * (2 * order - 1));
    root->children = malloc(sizeof(BTreeNode *) * (2 * order));
    root->values = malloc(sizeof(Person *) * (2 * order - 1));

    if (root->keys == NULL || root->children == NULL || root->values == NULL)
    {
        printf("Failed to allocate node arrays\n");
        free(root->keys);
        free(root->children);
        free(root->values);
        free(root);
        free(bt);
        return NULL;
    }

    root->key_count = 0;
    root->is_leaf = 1;
    root->order = order;

    bt->root = root;
    bt->order = order;
    bt->total_keys = 0;

    printf("B-tree created with order %d\n", order);
    return bt;
}

// ============================================================================
// FREE - Deallocate B-tree recursively
// ============================================================================
static void btree_free_node(BTreeNode *node)
{
    if (node == NULL)
        return;

    if (!node->is_leaf)
    {
        // Recursively free children
        for (int i = 0; i <= node->key_count; i++)
        {
            if (node->children[i] != NULL)
                btree_free_node(node->children[i]);
        }
    }

    free(node->keys);
    free(node->children);
    free(node->values);
    free(node);
}

void btree_free(BTree *bt)
{
    if (bt == NULL)
        return;

    btree_free_node(bt->root);
    free(bt);
}

// ============================================================================
// SEARCH - Find exact key match
// ============================================================================
//
// ALGORITHM:
// 1. Start at root
// 2. Search for key in current node
// 3. If found, return value
// 4. If not found and leaf, return NULL
// 5. If not found and internal, go to appropriate child
//
// TIME: O(log n) - height of tree
// ============================================================================
static Person *btree_search_recursive(BTreeNode *node, int key)
{
    if (node == NULL)
        return NULL;

    // Find position where key should be
    int i = 0;
    while (i < node->key_count && key > node->keys[i])
        i++;

    // Check if key found at position i
    if (i < node->key_count && key == node->keys[i])
        return (Person *)node->values[i];

    // If leaf, key not found
    if (node->is_leaf)
        return NULL;

    // Recursively search child
    return btree_search_recursive(node->children[i], key);
}

Person *btree_search(BTree *bt, int key)
{
    if (bt == NULL)
        return NULL;

    return btree_search_recursive(bt->root, key);
}

// ============================================================================
// INSERT - Add key-value pair
// ============================================================================
//
// SIMPLIFIED VERSION - Just insert into leaf nodes
// (Full B-tree insert handles node splitting, but that's complex)
//
// ALGORITHM:
// 1. Find leaf where key should go
// 2. Insert key and value in sorted order
// 3. If leaf is full, split it (not implemented here)
// ============================================================================
static void btree_insert_recursice(BTreeNode *node, int key, Person *value)
{
    int i = 0;

    // Find insertion position
    while (i < node->key_count && key > node->keys[i])
        i++;

    if (!node->is_leaf)
    {
        // Go to appropriate child
        if (node->children[i] == NULL)
        {
            printf("Error: Child pointer is NULL\n");
            return;
        }
        btree_insert_recursice(node->children[i], key, value);
        return;
    }

    // Leaf node: insert here
    // Shift elements to make room
    for (int j = node->key_count - 1; j > i; j--)
    {
        node->keys[j] = node->keys[j - 1];
        node->values[j] = node->values[j - 1];
    }

    // Insert new key and value
    node->keys[i] = key;
    node->values[i] = (void *)value;
    node->key_count++;
}

void btree_insert(BTree *bt, int key, Person *value)
{
    if (bt == NULL || bt->root == NULL)
        return;

    btree_insert_recursice(bt->root, key, value);
    bt->total_keys++;
}

// ============================================================================
// RANGE SEARCH - Find all keys between min and max
// ============================================================================
//
// THIS IS THE KEY ADVANTAGE OF B-TREES!
// Once we find the first key >= min, we can walk the sorted keys.
//
// ALGORITHM:
// 1. Find starting position (first key >= min_key)
// 2. Walk through node keys while key <= max_key
// 3. Recursively check children for additional results in range
//
// TIME: O(log n) to find start + O(k) where k = results found
// COMPARE TO LINEAR SCAN: O(n) always
// ============================================================================
static int btree_range_recursive(BTreeNode *node, int min_key, int max_key, Person **results, int result_count)
{
    if (node == NULL)
        return result_count;

    if (node->is_leaf)
    {
        // Leaf: collect all keys in range
        for (int i = 0; i < node->key_count; i++)
        {
            if (node->keys[i] >= min_key && node->keys[i] <= max_key)
            {
                results[result_count++] = (Person *)node->values[i];
            }
        }
        return result_count;
    }

    // Internal node: check children
    for (int i = 0; i < node->key_count; i++)
    {
        // Prune: skip children that can't contain results
        if (i < node->key_count && node->keys[i] >= min_key)
            continue;
        if (i > 0 && node->keys[i - 1] > max_key)
            break;

        result_count = btree_range_recursive(node->children[i], min_key, max_key, results, result_count);
    }

    return result_count;
}

int btree_range_search(BTree *bt, int min_key, int max_key, Person **results)
{
    if (bt == NULL || bt->root == NULL || results == NULL)
        return 0;

    return btree_range_recursive(bt->root, min_key, max_key, results, 0);
}

// ============================================================================
// STATISTICS - Show B-tree structure
// ============================================================================
static int btree_height_recursive(BTreeNode *node)
{
    if (node == NULL)
        return 0;

    if (node->is_leaf)
        return 1;

    return 1 + btree_height_recursive(node->children[0]);
}

void btree_display_stats(BTree *bt)
{
    if (bt == NULL)
        return;

    int height = btree_height_recursive(bt->root);

    printf("\n=== B-Tree Statistics ===\n");
    printf("Total keys:     %d\n", bt->total_keys);
    printf("Tree order:     %d\n", bt->order);
    printf("Tree height:    %d\n", height);
    printf("Root key count: %d\n", bt->root->key_count);
    printf("========================\n\n");
}