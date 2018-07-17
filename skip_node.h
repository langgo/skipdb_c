#ifndef SKIPDB_SKIP_NODE_H
#define SKIPDB_SKIP_NODE_H

#include <stdint.h>

#define SKIP_NODE_HEADER_SIZE (sizeof(skip_node_t))
#define SKIP_NODE_FORWARD_SIZE (3) // 2 ^ SKIP_NODE_FORWARD_SIZE = sizeof(uint64_t)
#define SKIP_NODE_FLAG_DEL (1)

typedef struct skip_node_t {
    uint64_t forwards[1];
    uint16_t flag;
    uint16_t level;
    uint32_t key_len;
    uint64_t value; // TODO 不同平台怎么处理 64 32
    char key[0];
} skip_node_t;

void skip_node_init(skip_node_t *node, uint16_t level, const char *key, uint32_t key_len, uint64_t value);

size_t skip_node_size(skip_node_t *node);

#endif //SKIPDB_SKIP_NODE_H
