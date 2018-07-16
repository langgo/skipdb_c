#ifndef SKIPDB_SKIPDB_H
#define SKIPDB_SKIPDB_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "skip_node.h"

#define PAGE_SIZE (4096)

#define SKIPDB_MIN_FILE_SIZE (4096)
#define SKIPDB_MAX_KEY_LEN (0xFFFF)
#define SKIPDB_MAX_LEVEL_LIMIT (64)
#define SKIPDB_HEADER_NODE_OFFSET (1024)

// maxMapSize represents the largest mmap size supported by SkipDB.
#define SKIPDB_MAX_SIZE (0xFFFFFFFFFFFF) // 256TB // TODO 和硬件平台有关系
#define SKIPDB_MAX_MMAP_STEP (1<<30)

#define SKIPDB_MAGIC (0x0706050403020100)
#define SKIPDB_VERSION (0)

typedef struct skip_list_t {
    uint64_t magic;
    uint64_t version;
    uint16_t max_level;
    uint16_t cur_level;
    float p;
    uint64_t last_offset;
    uint64_t count;
} skip_list_t;

typedef struct skipdb_t {
    skip_list_t *list;
    skip_node_t *header;
    char *mmap_addr;
    uint64_t mmap_size;
    int fd;

    int (*compare)(const void *s1, const void *s2, size_t n1, size_t n2);
} skipdb_t;

skipdb_t *skipdb_open(const char *file);

void skipdb_dump(skipdb_t *db, bool dump_node);

void skipdb_dump_node(skipdb_t *db, skip_node_t *node);

int skipdb_close(skipdb_t *db);

int skipdb_sync(skipdb_t *db);

uint64_t skipdb_get(skipdb_t *db, const char *key, uint32_t key_len);

uint64_t skipdb_del(skipdb_t *db, const char *key, uint32_t key_len);

int skipdb_put(skipdb_t *db, const char *key, uint32_t key_len, uint64_t value);

#endif //SKIPDB_SKIPDB_H
