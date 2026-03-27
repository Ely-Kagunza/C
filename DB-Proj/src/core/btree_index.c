#define _CRT_SECURE_NO_WARNINGS
#include "../../include/btree_index.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static BTreeNode *btree_node_create(int order, int is_leaf)
{
  BTreeNode *node = malloc(sizeof(BTreeNode));
  if (!node)
    return NULL;

  node->keys = malloc((size_t)(order - 1) * sizeof(int));
  node->values = malloc((size_t)(order - 1) * sizeof(Person *));
  node->children = malloc((size_t)order * sizeof(BTreeNode *));

  if (!node->keys || !node->values || !node->children)
  {
    free(node->keys);
    free(node->values);
    free(node->children);
    free(node);
    return NULL;
  }

  node->key_count = 0;
  node->is_leaf = is_leaf;

  for (int i = 0; i < order - 1; i++)
  {
    node->keys[i] = 0;
    node->values[i] = NULL;
  }
  for (int i = 0; i < order; i++)
  {
    node->children[i] = NULL;
  }

  return node;
}

static void btree_node_free(BTreeNode *node)
{
  if (!node)
    return;

  if (!node->is_leaf)
  {
    for (int i = 0; i <= node->key_count; i++)
    {
      btree_node_free(node->children[i]);
    }
  }

  free(node->keys);
  free(node->values);
  free(node->children);
  free(node);
}

BTreeIndex *btree_create(int order, int field_type)
{
  if (order < 3)
    order = 5;

  BTreeIndex *tree = malloc(sizeof(BTreeIndex));
  if (!tree)
    return NULL;

  tree->root = btree_node_create(order, 1);
  if (!tree->root)
  {
    free(tree);
    return NULL;
  }

  tree->order = order;
  tree->field_type = field_type;
  return tree;
}

void btree_free(BTreeIndex *tree)
{
  if (!tree)
    return;
  btree_node_free(tree->root);
  free(tree);
}

static int btree_find_position(BTreeNode *node, int key)
{
  int i = 0;
  while (i < node->key_count && key > node->keys[i])
  {
    i++;
  }
  return i;
}

static void btree_split_child(BTreeNode *parent, int child_index,
                              BTreeNode *child, int order)
{
  int mid_index = order / 2 - 1;
  int mid_key = child->keys[mid_index];
  Person *mid_value = child->values[mid_index];

  BTreeNode *new_node = btree_node_create(order, child->is_leaf);
  if (!new_node)
    return;

  new_node->key_count = order - 2 - mid_index;
  for (int i = 0; i < new_node->key_count; i++)
  {
    new_node->keys[i] = child->keys[mid_index + 1 + i];
    if (child->is_leaf)
    {
      new_node->values[i] = child->values[mid_index + 1 + i];
    }
  }

  if (!child->is_leaf)
  {
    for (int i = 0; i <= new_node->key_count; i++)
    {
      new_node->children[i] = child->children[mid_index + 1 + i];
      child->children[mid_index + 1 + i] = NULL;
    }
  }

  child->key_count = mid_index;

  for (int i = parent->key_count; i > child_index; i--)
  {
    parent->keys[i] = parent->keys[i - 1];
    parent->values[i] = parent->values[i - 1];
  }
  parent->keys[child_index] = mid_key;
  parent->values[child_index] = mid_value;

  for (int i = parent->key_count + 1; i > child_index + 1; i--)
  {
    parent->children[i] = parent->children[i - 1];
  }
  parent->children[child_index + 1] = new_node;
  parent->key_count++;
}

static int btree_insert_non_full(BTreeNode *node, int key, Person *person,
                                 int order)
{
  int i = node->key_count - 1;

  if (node->is_leaf)
  {
    while (i >= 0 && key < node->keys[i])
    {
      node->keys[i + 1] = node->keys[i];
      node->values[i + 1] = node->values[i];
      i--;
    }
    node->keys[i + 1] = key;
    node->values[i + 1] = person;
    node->key_count++;
    return 1;
  }
  else
  {
    while (i >= 0 && key < node->keys[i])
    {
      i--;
    }
    i++;

    if (node->children[i]->key_count >= order - 1)
    {
      btree_split_child(node, i, node->children[i], order);
      if (key > node->keys[i])
      {
        i++;
      }
    }

    return btree_insert_non_full(node->children[i], key, person, order);
  }
}

int btree_insert(BTreeIndex *tree, int key, Person *person)
{
  if (!tree || !person)
    return 0;

  BTreeNode *root = tree->root;

  if (root->key_count >= tree->order - 1)
  {
    BTreeNode *new_root = btree_node_create(tree->order, 0);
    if (!new_root)
      return 0;

    new_root->children[0] = root;
    tree->root = new_root;
    btree_split_child(new_root, 0, root, tree->order);
    btree_insert_non_full(new_root, key, person, tree->order);
  }
  else
  {
    btree_insert_non_full(root, key, person, tree->order);
  }

  return 1;
}

static void btree_collect_range(BTreeNode *node, int min_key, int max_key,
                                Person ***results, int *count, int *capacity)
{
  if (!node)
    return;

  if (node->is_leaf)
  {
    for (int i = 0; i < node->key_count; i++)
    {
      if (node->keys[i] >= min_key && node->keys[i] <= max_key)
      {
        if (*count >= *capacity)
        {
          *capacity *= 2;
          *results = realloc(*results, (size_t)*capacity * sizeof(Person *));
        }
        (*results)[(*count)++] = node->values[i];
      }
    }
  }
  else
  {
    for (int i = 0; i <= node->key_count; i++)
    {
      if (i == 0 || node->keys[i - 1] <= max_key)
      {
        if (i == node->key_count || node->keys[i] >= min_key)
        {
          btree_collect_range(node->children[i], min_key, max_key, results,
                              count, capacity);
        }
      }
    }
  }
}

RangeResult btree_range_query(BTreeIndex *tree, int min_key, int max_key)
{
  RangeResult result = {0};

  if (!tree)
    return result;

  int capacity = 16;
  result.results = malloc((size_t)capacity * sizeof(Person *));
  if (!result.results)
    return result;

  result.count = 0;
  btree_collect_range(tree->root, min_key, max_key, &result.results, &result.count, &capacity);

  return result;
}

void range_result_free(RangeResult *result)
{
  if (result && result->results)
  {
    free(result->results);
    result->results = NULL;
    result->count = 0;
  }
}

int btree_build_from_database(BTreeIndex *tree, const Database *db)
{
  if (!tree || !db)
    return 0;

  btree_node_free(tree->root);
  tree->root = btree_node_create(tree->order, 1);
  if (!tree->root)
    return 0;

  for (int i = 0; i < db->count; i++)
  {
    int key = (tree->field_type == 0) ? db->records[i].age : (int)db->records[i].salary;

    if (!btree_insert(tree, key, &db->records[i]))
    {
      printf("Failed to insert record into B-Tree\n");
      return 0;
    }
  }

  printf("B-Tree built with %d records\n", db->count);
  return 1;
}

static int btree_get_height_helper(BTreeNode *node)
{
  if (!node)
    return 0;
  if (node->is_leaf)
    return 1;
  return 1 + btree_get_height_helper(node->children[0]);
}

int btree_get_height(BTreeIndex *tree)
{
  if (!tree)
    return 0;
  return btree_get_height_helper(tree->root);
}

static void btree_print_helper(BTreeNode *node, int depth)
{
  if (!node)
    return;

  printf("%*sNode (is_leaf=%d, keys=%d): [", depth * 2, "", node->is_leaf, node->key_count);
  for (int i = 0; i < node->key_count; i++)
  {
    if (i > 0)
      printf(", ");
    printf("%d", node->keys[i]);
  }
  printf("]\n");

  if (!node->is_leaf)
  {
    for (int i = 0; i <= node->key_count; i++)
    {
      btree_print_helper(node->children[i], depth + 1);
    }
  }
}

void btree_print_structure(BTreeIndex *tree)
{
  if (!tree)
  {
    printf("B-Tree is NULL\n");
    return;
  }

  printf("/n=== B-Tree Structure (Height=%d) ===\n", btree_get_height(tree));
  btree_print_helper(tree->root, 0);
  printf("===================================\n\n");
}