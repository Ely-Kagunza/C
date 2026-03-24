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
// SPLIT CHILD - Split a full child node
// ============================================================================
//
// CONCEPT: When a child node is full (key_count == 2*order-1),
// we split it into two nodes and promote the middle key to parent
//
// ALGORITHM:
// 1. Find middle index (order - 1)
// 2. Create new right node
// 3. Copy right half of keys/values to new node
// 4. Shift parent's children forward
// 5. Promote middle key to parent
// 6. Update key counts
//
// VISUAL (order=3, so max 5 keys):
// Before split (child full with 5 keys):
//   Parent: [___]
//   Child:  [1][2][3][4][5]
//
// After split:
//   Parent: [3]
//   Left:   [1][2]    Right: [4][5]
// ============================================================================
static void btree_split_child(BTreeNode *parent, int child_index, BTreeNode *child)
{
    if (parent == NULL || child == NULL)
        return;

    int order = child->order;
    int mid_index = order - 1; // Middle position

    // Create new right node
    BTreeNode *right = malloc(sizeof(BTreeNode));
    if (right == NULL)
    {
        printf("Failed to allocate new node during split\n");
        return;
    }

    right->keys = malloc(sizeof(int) * (2 * order - 1));
    right->values = malloc(sizeof(Person *) * (2 * order - 1));
    right->children = malloc(sizeof(BTreeNode *) * (2 * order));

    if (right->keys == NULL || right->values == NULL || right->children == NULL)
    {
        printf("Failed to allocate node arrays during split\n");
        free(right->keys);
        free(right->values);
        free(right->children);
        free(right);
        return;
    }

    // Copy right half of keys/values to new node
    right->key_count = order - 1;
    right->is_leaf = child->is_leaf;
    right->order = order;

    for (int i = 0; i < right->key_count; i++)
    {
        right->keys[i] = child->keys[i + mid_index + 1];
        right->values[i] = child->values[i + mid_index + 1];
    }

    // Copy right half of children (if not leaf)
    if (!child->is_leaf)
    {
        for (int i = 0; i < right->key_count + 1; i++)
        {
            right->children[i] = child->children[i + mid_index + 1];
        }
    }

    // Reduce left child's key count
    child->key_count = mid_index;

    // Shift parent's children forward to make room
    for (int i = parent->key_count; i > child_index; i--)
    {
        parent->children[i + 1] = parent->children[i];
    }

    // Insert middle key into parent
    for (int i = parent->key_count - 1; i >= child_index; i--)
    {
        parent->keys[i + 1] = parent->keys[i];
        parent->values[i + 1] = parent->values[i];
    }

    parent->keys[child_index] = child->keys[mid_index];
    parent->values[child_index] = child->values[mid_index];
    parent->children[child_index + 1] = right;
    parent->key_count++;

    printf("Split child: key %d promoted to parent\n", child->keys[mid_index]);
}

// ============================================================================
// INSERT RECURSIVE - Insert with node splitting
// ============================================================================
//
// KEY CHANGE: Check if child is full BEFORE recursing
// If full, split it first, then decide which child to go to
// ============================================================================
static void btree_insert_recursive(BTreeNode *node, int key, Person *value)
{
    if (node == NULL)
        return;

    int i = 0;

    // Find insertion position
    while (i < node->key_count && key > node->keys[i])
        i++;

    // Internal node: go to child (but split first if full)
    if (!node->is_leaf)
    {
        // Check if child is full
        if (node->children[i]->key_count == 2 * node->order - 1)
        {
            // Split the full child
            printf("Child node full, splitting...\n");
            btree_split_child(node, i, node->children[i]);

            // After split, key might go to different child
            if (key > node->keys[i])
                i++;
        }

        // Recurse to appropriate child
        if (node->children[i] == NULL)
        {
            printf("Error: Child pointer is NULL\n");
            return;
        }

        btree_insert_recursive(node->children[i], key, value);
        return;
    }

    // Leaf node: check if full before inserting
    if (node->key_count == 2 * node->order - 1)
    {
        printf("Leaf node full, cannot insert (need parent split)\n");
        return;
    }

    // Shift elements to make room
    for (int j = node->key_count - 1; j >= i; j--)
    {
        node->keys[j] = node->keys[j - 1];
        node->values[j] = node->values[j - 1];
    }

    // Insert new key and value
    node->keys[i] = key;
    node->values[i] = (void *)value;
    node->key_count++;
}

// ============================================================================
// INSERT - Add key-value pair with splitting support
// ============================================================================
//
// KEY CHANGE: If root is full, create new root and split old root
// This is how B-tree grows in height
// ============================================================================
void btree_insert(BTree *bt, int key, Person *value)
{
    if (bt == NULL || bt->root == NULL || value == NULL)
        return;

    BTreeNode *root = bt->root;

    // If root is full, reate new root
    if (root->key_count == 2 * root->order - 1)
    {
        printf("Root full, creating new root and splitting old root...\n");

        // Create new root node
        BTreeNode *new_root = malloc(sizeof(BTreeNode));
        if (new_root == NULL)
        {
            printf("Failed to allocate new root node\n");
            return;
        }

        new_root->keys = malloc(sizeof(int) * (2 * root->order - 1));
        new_root->values = malloc(sizeof(Person *) * (2 * root->order - 1));
        new_root->children = malloc(sizeof(BTreeNode *) * (2 * root->order));

        if (new_root->keys == NULL || new_root->values == NULL || new_root->children == NULL)
        {
            printf("Failed to allocate new root node arrays\n");
            free(new_root->keys);
            free(new_root->values);
            free(new_root->children);
            free(new_root);
            return;
        }

        new_root->key_count = 0;
        new_root->is_leaf = 0;
        new_root->order = bt->order;
        new_root->children[0] = root;

        // Split old root as child of new root
        btree_split_child(new_root, 0, root);

        bt->root = new_root;
    }

    // Now insert into appropriate subtree
    btree_insert_recursive(bt->root, key, value);
    bt->total_keys++;
    printf("Inserted key %d: %s\n", key, value->name);
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
        // Visit child[i] if it might contain keys >= min_key
        // child[i] contains all keys < keys[i], so visit if min_key < keys[i]
        if (min_key < node->keys[i])
        {
            result_count = btree_range_recursive(node->children[i], min_key, max_key, results, result_count);
        }

        // Include key[i] if it's in the range
        if (node->keys[i] >= min_key && node->keys[i] <= max_key)
        {
            results[result_count++] = (Person *)node->values[i];
        }

        // Stop if we've passed the range
        if (node->keys[i] > max_key)
            break;
    }

    // Visit rightmost child if it might contain keys <= max_key
    // child[key_count] contains all keys > keys[key_count-1]
    if (node->children[node->key_count] != NULL)
    {
        result_count = btree_range_recursive(node->children[node->key_count], min_key, max_key, results, result_count);
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