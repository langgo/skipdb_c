#include <string.h>
#include <stdio.h>
#include "skip_node.h"

void skip_node_init(skip_node_t *node, uint16_t level, const char *key, uint32_t key_len, uint64_t value) {
    for (int i = 0; i < level; ++i) {
        node->forwards[-i] = 0;
    }

    node->flag = 0;
    node->level = level;
    node->key_len = key_len;
    node->value = value;

    memcpy(node->key, key, key_len);
}

inline size_t skip_node_size(skip_node_t *node) {
    return SKIP_NODE_HEADER_SIZE + (node->level << SKIP_NODE_FORWARD_SIZE) + node->key_len;
}