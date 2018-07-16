#include <string.h>
#include <stdio.h>
#include "skip_node.h"

inline uint64_t *skip_node_forwards(skip_node_t *node) {
    return (uint64_t *) (node->ptr);
}

inline char *skip_node_key(skip_node_t *node) {
    return (char *) (node->ptr + (node->level << 3));
}

void skip_node_init(skip_node_t *node, uint16_t level, const char *key, uint32_t key_len, uint64_t value) {
    node->flag = 0;
    node->level = level;
    node->key_len = key_len;
    node->value = value;

    uint64_t *forwards = skip_node_forwards(node);
    for (int i = 0; i < level; ++i) {
        forwards[i] = 0;
    }

    char *node_key = skip_node_key(node);
    memcpy(node_key, key, key_len);
}

inline size_t skip_node_size(skip_node_t *node) {
    return SKIP_NODE_HEADER_SIZE + node->level << SKIP_NODE_FORWARD_SIZE + node->key_len;
}