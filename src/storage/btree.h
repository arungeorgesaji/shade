#ifndef BTREE_H
#define BTREE_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include "../types/value.h"

typedef struct BTree BTree;
typedef struct BTreeNode BTreeNode;

typedef enum {
    BTREE_NODE_INTERNAL,
    BTREE_NODE_LEAF
} BTreeNodeType;

typedef struct BTreeNode {
    uint32_t id;
    BTreeNodeType type;
    bool is_dirty;
    
    uint32_t* child_ids;
    Value* keys;
    uint32_t key_count;
    
    uint64_t* record_ids;
    uint32_t record_count;
    
    uint32_t next_leaf;
} BTreeNode;

typedef struct {
    uint64_t root_node_id;
    uint64_t next_node_id;
    uint32_t order;
} FileHeader;

typedef struct BTree {
    uint32_t root_node_id;
    uint32_t next_node_id;
    uint32_t order;
    char* filename;
    FILE* file;
} BTree;

typedef struct BTreeRange {
    Value start_key;
    Value end_key;
    bool include_start;
    bool include_end;
} BTreeRange;

BTree* btree_create(const char* filename, uint32_t order);
BTree* btree_open(const char* filename);
bool btree_flush(BTree* tree);
void btree_close(BTree* tree);
void btree_destroy(BTree* tree);

bool btree_insert(BTree* tree, const Value* key, uint64_t record_id);
bool btree_search(BTree* tree, const Value* key, uint64_t** record_ids, uint32_t* count);

uint64_t* btree_scan_all(BTree* tree, uint32_t* result_count);
uint64_t* btree_range_query(BTree* tree, const BTreeRange* range, uint32_t* result_count);
uint64_t* btree_find_ghosts(BTree* tree, float min_strength, uint32_t* count);
void btree_free_results(uint64_t* results);

uint32_t btree_get_height(BTree* tree);
bool btree_validate(BTree* tree);

bool btree_split_child(BTree* tree, BTreeNode* parent, int index, BTreeNode* child);
bool btree_insert_nonfull(BTree* tree, BTreeNode* node, const Value* key, uint64_t record_id);

#endif
