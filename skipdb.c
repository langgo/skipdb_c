#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include "skipdb.h"

/********** private header start **********/

// return 0
size_t adjust_mmap_size(size_t size);

// return status
int skipdb_init(skipdb_t *db);

// return status
int skipdb_mmap(skipdb_t *db, size_t size);

skip_node_t *skipdb_node(skipdb_t *db, uint64_t offset);

uint64_t skipdb_offset(skipdb_t *db, skip_node_t *node);

uint16_t skipdb_random_level(skipdb_t *db);

skip_node_t *
skipdb_create_node(skipdb_t *db, uint16_t level, const char *key, uint32_t key_len, uint64_t value);

uint64_t skipdb_allocate(skipdb_t *db, uint32_t size);

skip_node_t *skipdb_find(skipdb_t *db, const char *key, uint32_t key_len);

/********** private header end **********/

int compare(const void *s1, const void *s2, size_t n1, size_t n2) {
    size_t min = n1 < n2 ? n1 : n2;
    int diff = memcmp(s1, s2, min);
    return diff != 0 ? diff : (int) (n1 - n2);
}

skipdb_t *skipdb_open(const char *file) {
    skipdb_t *db = malloc(sizeof(skipdb_t));

    db->list = NULL;
    db->header = NULL;
    db->mmap_addr = NULL;
    db->mmap_size = 0;
    db->compare = compare;

    if ((db->fd = open(file, O_CREAT | O_RDWR, 0644)) == -1) {
        return NULL;
    }

    struct stat st;
    if (fstat(db->fd, &st) == -1) {
        return NULL;
    }

    if (st.st_size == 0) { // create
        if (skipdb_init(db) == -1) {
            return NULL;
        }
    } else if (st.st_size < SKIPDB_MIN_FILE_SIZE) { // error size
        return NULL;
    } else { // reload
        if (skipdb_mmap(db, (size_t) st.st_size) == -1) {
            return NULL;
        }
        if (db->list->magic != SKIPDB_MAGIC) {
            return NULL;
        }
        db->header = skipdb_node(db, SKIPDB_HEADER_NODE_OFFSET);
    }
    return db;
}

int skipdb_close(skipdb_t *db) {
    if (skipdb_sync(db) == -1) {
        return -1;
    }
    if (munmap(db->mmap_addr, db->mmap_size) == -1) {
        return -1;
    }
    if ((close(db->fd)) == -1) {
        return -1;
    }

    free(db);
}

int skipdb_sync(skipdb_t *db) {
    return fdatasync(db->fd);
}

// return status
int skipdb_init(skipdb_t *db) {
    if (skipdb_mmap(db, SKIPDB_MIN_FILE_SIZE) == -1) {
        return -1;
    }

    db->list->magic = SKIPDB_MAGIC;
    db->list->version = SKIPDB_VERSION;
    db->list->max_level = 32; // TODO 当前设置为 32
    db->list->cur_level = 1;
    db->list->p = 0.5;
    db->list->last_offset = SKIPDB_HEADER_NODE_OFFSET;
    db->list->count = 0;

    db->header = skipdb_create_node(db, SKIPDB_MAX_LEVEL_LIMIT, "", 2, 0);
    if (db->header == NULL) { // TODO 创建的时候，其实不会出现错误。那么这里的错误应该怎么处理（与golang比较一下）
        return -1;
    }

    return 0;
}

// return status
int skipdb_mmap(skipdb_t *db, size_t size) {
    if (db->mmap_addr != NULL) {
        if (munmap(db->mmap_addr, db->mmap_size) == -1) {
            return -1;
        }
    }

    size = adjust_mmap_size(size);
    if (size == 0) {
        return -1;
    }

    if (ftruncate(db->fd, size) == -1) {
        return -1;
    }

    db->mmap_size = size;
    if ((db->mmap_addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, db->fd, 0)) == MAP_FAILED) {
        return -1;
    }

    db->list = (skip_list_t *) (db->mmap_addr);
    db->header = skipdb_node(db, SKIPDB_HEADER_NODE_OFFSET);
    return 0;
}

// return 0
size_t adjust_mmap_size(size_t size) {
    for (unsigned int i = 12; i <= 30; ++i) {
        if (size <= (size_t) 1 << i) {
            return (size_t) 1 << i;
        }
    }

    if (size > SKIPDB_MAX_SIZE) {
        return 0; // TODO 错误处理
    }

    size_t sz = size;
    size_t remainder = 0;
    if ((remainder = (sz % SKIPDB_MAX_MMAP_STEP)) > 0) {
        sz += SKIPDB_MAX_MMAP_STEP - remainder;
    }

    if ((sz % PAGE_SIZE) != 0) {
        sz = (sz / PAGE_SIZE + 1) * PAGE_SIZE;
    }

    if (sz > SKIPDB_MAX_SIZE) {
        sz = SKIPDB_MAX_SIZE;
    }
    return sz;
}

skip_node_t *skipdb_node(skipdb_t *db, uint64_t offset) {
    if (offset <= 0) {
        return NULL;
    }

    return (skip_node_t *) (db->mmap_addr + offset);
}

uint64_t skipdb_offset(skipdb_t *db, skip_node_t *node) {
    // TODO assert > 0
    return (char *) node - db->mmap_addr;
}

skip_node_t *skipdb_find(skipdb_t *db, const char *key, uint32_t key_len) {
    if (key == NULL) {
        return NULL;
    }
    if (key_len > SKIPDB_MAX_KEY_LEN) {
        return NULL;
    }

    skip_node_t *cur = NULL,
            *next = NULL;

    cur = db->header;
    for (int i = db->list->cur_level - 1; i >= 0; --i) {
        for (;;) {
            next = skipdb_node(db, skip_node_forwards(cur)[i]);
            if (next == NULL) {
                break;
            }

            int diff = db->compare(skip_node_key(next), key, next->key_len, key_len);
            if (diff < 0) {
                cur = next;
                continue;
            }
            if (diff > 0) {
                break;
            }
            return next;
        }
    }
    return NULL;
}

uint64_t skipdb_get(skipdb_t *db, const char *key, uint32_t key_len) {
    skip_node_t *node = skipdb_find(db, key, key_len);
    if (node == NULL) {
        return 0;
    }
    if (node->flag & SKIP_NODE_FLAG_DEL != 0) {
        return 0;
    }
    return node->value;
}

uint64_t skipdb_del(skipdb_t *db, const char *key, uint32_t key_len) {
    skip_node_t *node = skipdb_find(db, key, key_len);
    if (node == NULL) {
        return 0;
    }
    if (node->flag & SKIP_NODE_FLAG_DEL != 0) {
        return 0;
    }
    node->flag |= SKIP_NODE_FLAG_DEL;
    return node->value;
}

int skipdb_put(skipdb_t *db, const char *key, uint32_t key_len, uint64_t value) {
    if (key == NULL) {
        return -1;
    }
    if (key_len > SKIPDB_MAX_KEY_LEN) {
        return -1;
    }

    uint64_t updates[db->list->max_level];
    {
        skip_node_t *cur = NULL,
                *next = NULL;

        cur = db->header;
        for (int i = db->list->cur_level - 1; i >= 0; --i) {
            for (;;) {
                next = skipdb_node(db, skip_node_forwards(cur)[i]);
                if (next == NULL) {
                    break;
                }

                int diff = db->compare(skip_node_key(next), key, next->key_len, key_len);
                if (diff < 0) {
                    cur = next;
                    continue;
                }
                if (diff > 0) {
                    break;
                }
                next->flag &= ~SKIP_NODE_FLAG_DEL;
                next->value = value;
                return 0;
            }
            updates[i] = skipdb_offset(db, cur);
        }
    }

    uint16_t level = skipdb_random_level(db);
    if (level > db->list->cur_level) {
        for (int i = db->list->cur_level; i < level; ++i) {
            updates[i] = skipdb_offset(db, db->header);
        }
        db->list->cur_level = level;
    }

    skip_node_t *node = skipdb_create_node(db, level, key, key_len, value);
    if (node == NULL) {
        return -1; // 申请内存失败
    }

    uint64_t offset = skipdb_offset(db, node);
    for (int i = 0; i < level; ++i) {
        uint64_t *update_forwards = skip_node_forwards(skipdb_node(db, updates[i]));
        skip_node_forwards(node)[i] = update_forwards[i];
        update_forwards[i] = offset;
    }

    return 0;
}

uint16_t skipdb_random_level(skipdb_t *db) {
    uint16_t level = 1;

    int t = (int) (RAND_MAX * db->list->p);
    while (random() < t) {
        ++level;
    }
    return level < db->list->max_level ? level : db->list->max_level;
}

skip_node_t *
skipdb_create_node(skipdb_t *db, uint16_t level, const char *key, uint32_t key_len, uint64_t value) {
    uint32_t size = SKIP_NODE_HEADER_SIZE + (level << SKIP_NODE_FORWARD_SIZE) + key_len; // TODO int ?
    uint64_t off = skipdb_allocate(db, size); // allow 0
    skip_node_t *node = skipdb_node(db, off); // if off == 0 return NULL
    if (node == NULL) {
        return NULL;
    }

    skip_node_init(node, level, key, key_len, value); // TODO 不太清楚什么时候检测是否为 NULL

    return node;
}

uint64_t skipdb_allocate(skipdb_t *db, uint32_t size) {
    uint64_t last_offset = db->list->last_offset;
    uint64_t new_offset = last_offset + size;

    while (new_offset > db->mmap_size) {
        if (skipdb_mmap(db, new_offset) == -1) {
            return 0;
        }
    }

    db->list->last_offset += size;
    return last_offset;
}

void skipdb_dump_node(skipdb_t *db, skip_node_t *node) {
    // TODO write 流 与 接口
    // TODO sprintf 返回一个合适长度的字符串，与内存释放
    // TODO printf 与 forwards %v
    printf(
            "skip_node_t(0x%04lx)(%ld){flag: %d, level: %2d, key_len: %4d, value: %8ld, ",
            ((char *) node - db->mmap_addr),
            skip_node_size(node),
            node->flag,
            node->level,
            node->key_len,
            node->value
    );

    uint64_t *forwards = skip_node_forwards(node);

    printf("forwards: [");
    for (int i = 0; i < node->level; ++i) {
        printf("0x%lx, ", forwards[i]);
    }
    printf("], ");

    char *key = skip_node_key(node);

    printf("key: ");
    fwrite(key, node->key_len, 1, stdout);
    printf(" }\n");
}

void skipdb_dump(skipdb_t *db, bool dump_node) {
    printf("========= dump db start =========\n");
    printf("skipdb_t {\n"
           "\tlist: skip_list_t {\n"
           "\t\tmagic: %lx \n"
           "\t\tversion: %ld\n"
           "\t\tmax_level: %d\n"
           "\t\tcur_level: %d\n"
           "\t\tp: %f\n"
           "\t\tlast_offset: %ld\n"
           "\t\tcount: %ld\n"
           "\t}\n"
           "\theader: %p\n"
           "\tmmap_addr: %p\n"
           "\tmmap_size: %ld\n"
           "\tfd: %d\n"
           "\tcompare: %p\n"
           "}\n",
           db->list->magic,
           db->list->version,
           db->list->max_level,
           db->list->cur_level,
           db->list->p,
           db->list->last_offset,
           db->list->count,
           db->header,
           (void *) db->mmap_addr,
           db->mmap_size,
           db->fd,
           db->compare
    );

    if (dump_node) {
        printf("\n");
        skip_node_t *cur = db->header;
        while (cur != NULL) {
            skipdb_dump_node(db, cur);
            cur = skipdb_node(db, skip_node_forwards(cur)[0]);
        }
    }
    printf("========= dump db over =========\n");
}