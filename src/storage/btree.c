#include "btree.h"

#define BTREE_HEADER_SIZE (sizeof(FileHeader))
#define DEFAULT_BTREE_ORDER 3
#define PAGE_SIZE 4096

typedef struct {
    uint32_t node_id;
    uint32_t type;
    uint32_t key_count;
    uint32_t record_count;
    uint32_t next_leaf;
} BTreeNodeHeader;

static BTreeNode* btree_node_create(BTree* tree, BTreeNodeType type) {
    BTreeNode* node = malloc(sizeof(BTreeNode));
    if (!node) return NULL;
    
    node->id = tree->next_node_id++;
    node->type = type;
    node->is_dirty = true;
    node->key_count = 0;
    node->next_leaf = 0;
    
    if (type == BTREE_NODE_INTERNAL) {
        node->keys = malloc(sizeof(Value) * tree->order);
        node->child_ids = malloc(sizeof(uint32_t) * (tree->order + 1));
        node->record_count = 0;
        node->record_ids = NULL;
        
        if (!node->keys || !node->child_ids) {
            free(node->keys);
            free(node->child_ids);
            free(node);
            return NULL;
        }
        
        for (uint32_t i = 0; i < tree->order; i++) {
            node->keys[i].type = VALUE_NULL;
        }
        for (uint32_t i = 0; i <= tree->order; i++) {
            node->child_ids[i] = 0;
        }
    } else {
        node->record_ids = malloc(sizeof(uint64_t) * tree->order);
        node->keys = malloc(sizeof(Value) * tree->order);
        node->child_ids = NULL;
        node->record_count = 0;
        
        if (!node->record_ids || !node->keys) {
            free(node->record_ids);
            free(node->keys);
            free(node);
            return NULL;
        }
        
        for (uint32_t i = 0; i < tree->order; i++) {
            node->keys[i].type = VALUE_NULL;
            node->record_ids[i] = 0;
        }
    }
    
    return node;
}

static void btree_node_destroy(BTreeNode* node) {
    if (!node) return;
    
    if (node->keys) {
        for (uint32_t i = 0; i < node->key_count; i++) {
            value_destroy(&node->keys[i]);
        }
        free(node->keys);
    }
    
    if (node->child_ids) free(node->child_ids);
    if (node->record_ids) free(node->record_ids);
    free(node);
}

static bool btree_write_header(BTree* tree) {
    if (!tree || !tree->file) return false;

    FileHeader fh = {
        .root_node_id = (uint64_t)tree->root_node_id,
        .next_node_id = (uint64_t)tree->next_node_id,
        .order = tree->order
    };

    if (fseek(tree->file, 0, SEEK_SET) != 0) return false;
    if (fwrite(&fh, sizeof(FileHeader), 1, tree->file) != 1) return false;

    return fflush(tree->file) == 0;
}

static bool btree_read_header(BTree* tree) {
    if (!tree || !tree->file) return false;

    FileHeader fh;
    if (fseek(tree->file, 0, SEEK_SET) != 0) return false;
    if (fread(&fh, sizeof(FileHeader), 1, tree->file) != 1) return false;

    tree->root_node_id = (uint32_t)fh.root_node_id;
    tree->next_node_id = (uint32_t)fh.next_node_id;
    tree->order = fh.order;

    return true;
}

static bool btree_write_node(BTree* tree, BTreeNode* node) {
    if (!tree || !tree->file || !node) return false;
    
    size_t node_offset = BTREE_HEADER_SIZE + (node->id - 1) * PAGE_SIZE;
    
    if (fseek(tree->file, node_offset, SEEK_SET) != 0) return false;
    
    BTreeNodeHeader header = {
        .node_id = node->id,
        .type = (uint32_t)node->type,
        .key_count = node->key_count,
        .record_count = node->record_count,
        .next_leaf = node->next_leaf
    };
    
    if (fwrite(&header, sizeof(BTreeNodeHeader), 1, tree->file) != 1) return false;
    
    for (uint32_t i = 0; i < node->key_count; i++) {
        if (fwrite(&node->keys[i].type, sizeof(ValueType), 1, tree->file) != 1) return false;
        
        switch (node->keys[i].type) {
            case VALUE_INTEGER:
                if (fwrite(&node->keys[i].data.integer, sizeof(int64_t), 1, tree->file) != 1) return false;
                break;
            case VALUE_FLOAT:
                if (fwrite(&node->keys[i].data.float_val, sizeof(double), 1, tree->file) != 1) return false;
                break;
            case VALUE_BOOLEAN:
                if (fwrite(&node->keys[i].data.boolean, sizeof(bool), 1, tree->file) != 1) return false;
                break;
            case VALUE_STRING:
                if (node->keys[i].data.string) {
                    uint32_t len = strlen(node->keys[i].data.string);
                    if (fwrite(&len, sizeof(uint32_t), 1, tree->file) != 1) return false;
                    if (fwrite(node->keys[i].data.string, 1, len, tree->file) != len) return false;
                } else {
                    uint32_t len = 0;
                    if (fwrite(&len, sizeof(uint32_t), 1, tree->file) != 1) return false;
                }
                break;
            case VALUE_NULL:
                break;
        }
    }
    
    if (node->type == BTREE_NODE_INTERNAL) {
        if (fwrite(node->child_ids, sizeof(uint32_t), node->key_count + 1, tree->file) != node->key_count + 1) return false;
    } else {
        if (fwrite(node->record_ids, sizeof(uint64_t), node->record_count, tree->file) != node->record_count) return false;
    }
    
    long current_pos = ftell(tree->file);
    if (current_pos == -1) return false;
    
    long bytes_written = current_pos - node_offset;
    for (long i = bytes_written; i < PAGE_SIZE; i++) {
        if (fputc(0, tree->file) == EOF) return false;
    }
    
    if (fflush(tree->file) != 0) return false;
    node->is_dirty = false;
    
    return true;
}

static BTreeNode* btree_read_node(BTree* tree, uint32_t node_id) {
    if (!tree || !tree->file || node_id == 0) return NULL;
    
    size_t node_offset = BTREE_HEADER_SIZE + (node_id - 1) * PAGE_SIZE;
    if (fseek(tree->file, node_offset, SEEK_SET) != 0) return NULL;
    
    BTreeNodeHeader header;
    if (fread(&header, sizeof(BTreeNodeHeader), 1, tree->file) != 1) return NULL;
    
    BTreeNode* node = malloc(sizeof(BTreeNode));
    if (!node) return NULL;
    
    node->id = header.node_id;
    node->type = (BTreeNodeType)header.type;
    node->key_count = header.key_count;
    node->record_count = header.record_count;
    node->next_leaf = header.next_leaf;
    node->is_dirty = false;
    
    if (node->type == BTREE_NODE_INTERNAL) {
        node->keys = malloc(sizeof(Value) * tree->order);
        node->child_ids = malloc(sizeof(uint32_t) * (tree->order + 1));
        node->record_ids = NULL;
        
        if (!node->keys || !node->child_ids) {
            free(node->keys);
            free(node->child_ids);
            free(node);
            return NULL;
        }
    } else {
        node->record_ids = malloc(sizeof(uint64_t) * tree->order);
        node->keys = malloc(sizeof(Value) * tree->order);
        node->child_ids = NULL;
        
        if (!node->record_ids || !node->keys) {
            free(node->record_ids);
            free(node->keys);
            free(node);
            return NULL;
        }
    }
    
    for (uint32_t i = 0; i < node->key_count; i++) {
        ValueType type;
        if (fread(&type, sizeof(ValueType), 1, tree->file) != 1) {
            btree_node_destroy(node);
            return NULL;
        }
        node->keys[i].type = type;
        
        switch (type) {
            case VALUE_INTEGER:
                if (fread(&node->keys[i].data.integer, sizeof(int64_t), 1, tree->file) != 1) {
                    btree_node_destroy(node);
                    return NULL;
                }
                break;
            case VALUE_FLOAT:
                if (fread(&node->keys[i].data.float_val, sizeof(double), 1, tree->file) != 1) {
                    btree_node_destroy(node);
                    return NULL;
                }
                break;
            case VALUE_BOOLEAN:
                if (fread(&node->keys[i].data.boolean, sizeof(bool), 1, tree->file) != 1) {
                    btree_node_destroy(node);
                    return NULL;
                }
                break;
            case VALUE_STRING:
                {
                    uint32_t len;
                    if (fread(&len, sizeof(uint32_t), 1, tree->file) != 1) {
                        btree_node_destroy(node);
                        return NULL;
                    }
                    if (len > 0) {
                        node->keys[i].data.string = malloc(len + 1);
                        if (!node->keys[i].data.string) {
                            btree_node_destroy(node);
                            return NULL;
                        }
                        if (fread((char*)node->keys[i].data.string, 1, len, tree->file) != len) {
                            btree_node_destroy(node);
                            return NULL;
                        }
                        ((char*)node->keys[i].data.string)[len] = '\0';
                    } else {
                        node->keys[i].data.string = NULL;
                    }
                }
                break;
            case VALUE_NULL:
                break;
        }
    }
    
    if (node->type == BTREE_NODE_INTERNAL) {
        if (fread(node->child_ids, sizeof(uint32_t), node->key_count + 1, tree->file) != node->key_count + 1) {
            btree_node_destroy(node);
            return NULL;
        }
    } else {
        if (fread(node->record_ids, sizeof(uint64_t), node->record_count, tree->file) != node->record_count) {
            btree_node_destroy(node);
            return NULL;
        }
    }
    
    return node;
}

BTree* btree_create(const char* filename, uint32_t order) {
    BTree* tree = malloc(sizeof(BTree));
    if (!tree) return NULL;
    
    tree->order = order ? order : DEFAULT_BTREE_ORDER;
    tree->next_node_id = 1;
    tree->filename = string_duplicate(filename);
    if (!tree->filename) {
        free(tree);
        return NULL;
    }
    
    BTreeNode* root = btree_node_create(tree, BTREE_NODE_LEAF);
    if (!root) {
        free(tree->filename);
        free(tree);
        return NULL;
    }
    
    tree->root_node_id = root->id;
    
    tree->file = fopen(filename, "w+b");
    if (!tree->file) {
        free(tree->filename);
        btree_node_destroy(root);
        free(tree);
        return NULL;
    }
    
    if (!btree_write_header(tree) || !btree_write_node(tree, root)) {
        fclose(tree->file);
        free(tree->filename);
        btree_node_destroy(root);
        free(tree);
        return NULL;
    }
    
    btree_node_destroy(root);
    return tree;
}

BTree* btree_open(const char* filename) {
    BTree* tree = malloc(sizeof(BTree));
    if (!tree) return NULL;
    
    tree->filename = string_duplicate(filename);
    if (!tree->filename) {
        free(tree);
        return NULL;
    }
    
    tree->file = fopen(filename, "r+b");
    if (!tree->file) {
        free(tree->filename);
        free(tree);
        return NULL;
    }
    
    if (!btree_read_header(tree)) {
        fclose(tree->file);
        free(tree->filename);
        free(tree);
        return NULL;
    }
    
    return tree;
}

bool btree_flush(BTree* tree) {
    if (!tree || !tree->file) return false;
    return fflush(tree->file) == 0;
}

void btree_close(BTree* tree) {
    if (!tree) return;

    if (tree->file) {
        btree_write_header(tree);   
        btree_flush(tree);          
        fclose(tree->file);
    }

    free(tree->filename);
    free(tree);
}

void btree_destroy(BTree* tree) {
    btree_close(tree);
}

bool btree_split_child(BTree* tree, BTreeNode* parent, int index, BTreeNode* child) {
    if (!tree || !parent || !child) return false;

    BTreeNode* new_child = btree_node_create(tree, child->type);
    if (!new_child) return false;

    uint32_t mid;
    if (child->type == BTREE_NODE_INTERNAL) {
        mid = tree->order / 2;
        
        new_child->key_count = child->key_count - mid - 1;
        for (uint32_t i = 0; i < new_child->key_count; i++) {
            new_child->keys[i] = value_clone(&child->keys[mid + 1 + i]);
        }
        for (uint32_t i = 0; i <= new_child->key_count; i++) {
            new_child->child_ids[i] = child->child_ids[mid + 1 + i];
        }

        Value promote_key = value_clone(&child->keys[mid]);
        parent->keys[index] = promote_key;

        child->key_count = mid;

    } else {
        mid = (tree->order - 1) / 2;  
        
        new_child->key_count = child->key_count - mid;
        new_child->record_count = child->record_count - mid;
        
        for (uint32_t i = 0; i < new_child->key_count; i++) {
            new_child->keys[i] = value_clone(&child->keys[mid + i]);
            new_child->record_ids[i] = child->record_ids[mid + i];
        }

        Value promote_key = value_clone(&child->keys[mid]);
        parent->keys[index] = promote_key;

        new_child->next_leaf = child->next_leaf;
        child->next_leaf = new_child->id;

        uint32_t old_key_count = child->key_count;
        child->key_count = mid;
        child->record_count = mid;

        for (uint32_t i = mid; i < old_key_count; i++) {
            value_destroy(&child->keys[i]);
        }
    }

    for (int i = parent->key_count; i > index; i--) {
        parent->keys[i] = parent->keys[i - 1];
    }
    for (int i = parent->key_count + 1; i > index + 1; i--) {
        parent->child_ids[i] = parent->child_ids[i - 1];
    }

    parent->child_ids[index + 1] = new_child->id;
    parent->key_count++;

    bool success = btree_write_node(tree, child) &&
                   btree_write_node(tree, new_child) &&
                   btree_write_node(tree, parent);

    btree_node_destroy(new_child);
    return success;
}

bool btree_insert_nonfull(BTree* tree, BTreeNode* node, const Value* key, uint64_t record_id) {
    if (!tree || !node || !key) return false;
    
    int i = (int)node->key_count - 1;
    
    if (node->type == BTREE_NODE_LEAF) {
        while (i >= 0 && value_compare(&node->keys[i], key) > 0) {
            node->keys[i + 1] = node->keys[i];
            node->record_ids[i + 1] = node->record_ids[i];
            i--;
        }
        
        node->keys[i + 1] = value_clone(key);
        node->record_ids[i + 1] = record_id;
        node->key_count++;
        node->record_count++;
        node->is_dirty = true;
        
        return btree_write_node(tree, node);
    } else {
        while (i >= 0 && value_compare(&node->keys[i], key) > 0) {
            i--;
        }
        i++; 
        
        BTreeNode* child = btree_read_node(tree, node->child_ids[i]);
        if (!child) return false;
        
        if (child->key_count == tree->order - 1) {
            if (!btree_split_child(tree, node, i, child)) {
                btree_node_destroy(child);
                return false;
            }
            
            if (value_compare(key, &node->keys[i]) > 0) {
                i++;
            }
            
            btree_node_destroy(child);
            child = btree_read_node(tree, node->child_ids[i]);
            if (!child) return false;
        }
        
        bool result = btree_insert_nonfull(tree, child, key, record_id);
        btree_node_destroy(child);
        return result;
    }
}

bool btree_insert(BTree* tree, const Value* key, uint64_t record_id) {
    if (!tree || !key) return false;
    
    BTreeNode* root = btree_read_node(tree, tree->root_node_id);
    if (!root) return false;
    
    if (root->key_count == tree->order - 1) {
        BTreeNode* new_root = btree_node_create(tree, BTREE_NODE_INTERNAL);
        if (!new_root) {
            btree_node_destroy(root);
            return false;
        }
        
        new_root->child_ids[0] = root->id;
        tree->root_node_id = new_root->id;
        
        if (!btree_split_child(tree, new_root, 0, root)) {
            btree_node_destroy(new_root);
            btree_node_destroy(root);
            return false;
        }
        
        int i = 0;
        if (value_compare(key, &new_root->keys[0]) > 0) {
            i = 1;
        }
        
        BTreeNode* child = btree_read_node(tree, new_root->child_ids[i]);
        if (!child) {
            btree_node_destroy(new_root);
            btree_node_destroy(root);
            return false;
        }
        
        bool result = btree_insert_nonfull(tree, child, key, record_id);
        btree_node_destroy(child);
        btree_node_destroy(new_root);
        btree_node_destroy(root);
        return result;
        
    } else {
        bool result = btree_insert_nonfull(tree, root, key, record_id);
        btree_node_destroy(root);
        return result;
    }
}

bool btree_search(BTree* tree, const Value* key, uint64_t** record_ids, uint32_t* count) {
    if (!tree || !key || !record_ids || !count) return false;
    
    *record_ids = NULL;
    *count = 0;

    BTreeNode* current = btree_read_node(tree, tree->root_node_id);
    if (!current) return false;

    while (current->type == BTREE_NODE_INTERNAL) {
        uint32_t i = 0;
        while (i < current->key_count && value_compare(key, &current->keys[i]) > 0) {
            i++;
        }
        BTreeNode* next = btree_read_node(tree, current->child_ids[i]);
        btree_node_destroy(current);
        current = next;
        if (!current) return false;
    }

    while (current) {
        for (uint32_t i = 0; i < current->key_count; i++) {
            int cmp = value_compare(key, &current->keys[i]);
            if (cmp == 0) {
                *record_ids = malloc(sizeof(uint64_t));
                if (!*record_ids) {
                    btree_node_destroy(current);
                    return false;
                }
                (*record_ids)[0] = current->record_ids[i];
                *count = 1;
                btree_node_destroy(current);
                return true;
            }
            if (cmp < 0) {
                btree_node_destroy(current);
                return false;
            }
        }

        uint32_t next_id = current->next_leaf;
        btree_node_destroy(current);
        if (next_id == 0) return false;
        current = btree_read_node(tree, next_id);
        if (!current) return false;
    }

    return false;
}

uint64_t* btree_scan_all(BTree* tree, uint32_t* result_count) {
    if (!tree || !result_count) return NULL;
    
    *result_count = 0;
    uint64_t* results = NULL;
    uint32_t capacity = 0;
    
    BTreeNode* current = btree_read_node(tree, tree->root_node_id);
    while (current && current->type == BTREE_NODE_INTERNAL) {
        BTreeNode* next = btree_read_node(tree, current->child_ids[0]);
        btree_node_destroy(current);
        current = next;
    }
    
    while (current) {
        for (uint32_t i = 0; i < current->record_count; i++) {
            if (*result_count >= capacity) {
                capacity = capacity == 0 ? 16 : capacity * 2;
                uint64_t* new_results = realloc(results, capacity * sizeof(uint64_t));
                if (!new_results) {
                    free(results);
                    btree_node_destroy(current);
                    return NULL;
                }
                results = new_results;
            }
            results[(*result_count)++] = current->record_ids[i];
        }
        
        if (current->next_leaf == 0) {
            btree_node_destroy(current);
            break;
        }
        
        BTreeNode* next = btree_read_node(tree, current->next_leaf);
        btree_node_destroy(current);
        current = next;
    }
    
    return results;
}

void btree_free_results(uint64_t* results) {
    free(results);
}

uint32_t btree_get_height(BTree* tree) {
    if (!tree) return 0;
    
    uint32_t height = 0;
    BTreeNode* current = btree_read_node(tree, tree->root_node_id);
    
    while (current) {
        height++;
        if (current->type == BTREE_NODE_LEAF) {
            btree_node_destroy(current);
            break;
        }
        
        BTreeNode* next = btree_read_node(tree, current->child_ids[0]);
        btree_node_destroy(current);
        current = next;
    }
    
    return height;
}

bool btree_validate(BTree* tree) {
    if (!tree) return false;
    
    BTreeNode* root = btree_read_node(tree, tree->root_node_id);
    if (!root) return false;
    
    btree_node_destroy(root);
    return true;
}

uint64_t* btree_range_query(BTree* tree, const BTreeRange* range, uint32_t* result_count) {
    if (!tree || !range || !result_count) {
        *result_count = 0;
        return NULL;
    }
    
    *result_count = 0;
    uint64_t* results = NULL;
    uint32_t capacity = 0;
    
    BTreeNode* current = btree_read_node(tree, tree->root_node_id);
    if (!current) return NULL;
    
    while (current->type == BTREE_NODE_INTERNAL) {
        uint32_t i = 0;
        while (i < current->key_count) {
            if (range->include_start ? 
                value_compare(&range->start_key, &current->keys[i]) <= 0 :
                value_compare(&range->start_key, &current->keys[i]) < 0) {
                break;
            }
            i++;
        }
        
        BTreeNode* next = btree_read_node(tree, current->child_ids[i]);
        btree_node_destroy(current);
        current = next;
        if (!current) return NULL;
    }
    
    while (current) {
        for (uint32_t i = 0; i < current->key_count; i++) {
            int cmp_start = value_compare(&current->keys[i], &range->start_key);
            int cmp_end = value_compare(&current->keys[i], &range->end_key);
            
            bool in_range = false;
            if (range->include_start && range->include_end) {
                in_range = (cmp_start >= 0 && cmp_end <= 0);
            } else if (range->include_start && !range->include_end) {
                in_range = (cmp_start >= 0 && cmp_end < 0);
            } else if (!range->include_start && range->include_end) {
                in_range = (cmp_start > 0 && cmp_end <= 0);
            } else {
                in_range = (cmp_start > 0 && cmp_end < 0);
            }
            
            if (in_range) {
                if (*result_count >= capacity) {
                    capacity = capacity == 0 ? 16 : capacity * 2;
                    uint64_t* new_results = realloc(results, capacity * sizeof(uint64_t));
                    if (!new_results) {
                        free(results);
                        btree_node_destroy(current);
                        return NULL;
                    }
                    results = new_results;
                }
                results[(*result_count)++] = current->record_ids[i];
            }
            
            if ((range->include_end && cmp_end > 0) || 
                (!range->include_end && cmp_end >= 0)) {
                btree_node_destroy(current);
                return results;
            }
        }
        
        if (current->next_leaf == 0) {
            btree_node_destroy(current);
            break;
        }
        
        BTreeNode* next = btree_read_node(tree, current->next_leaf);
        btree_node_destroy(current);
        current = next;
        if (!current) break;
    }
    
    return results;
}

uint64_t* btree_find_ghosts(BTree* tree, float min_strength, uint32_t* count) {
    (void)tree; 
    (void)min_strength;
    *count = 0;
    return NULL;
}
