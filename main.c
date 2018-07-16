#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdbool.h>
#include "skipdb.h"

void test() {

}

// 功能测试
void test_put_get(skipdb_t *db) {
    char *key_prefix = "test_put_get|";
    uint32_t key_len = (uint32_t) (strlen(key_prefix) + 2);

    char key[key_len + 1];
    for (int i = 0; i < 100; ++i) {
        sprintf(key, "%s%02d", key_prefix, i);

        if (skipdb_put(db, key, key_len, i + 1) == -1) {
            perror("skipdb_put");
            exit(-1);
        }

        uint64_t value = skipdb_get(db, key, key_len);
        if (value != i + 1) {
            perror("skipdb_get");
            printf("获取的值不一致 1");
            printf("i: %d, key: %s, value: %ld\n", i, key, value);
            exit(-1);
        }
    }

    for (int i = 0; i < 100; ++i) {
        sprintf(key, "%s%02d", key_prefix, i);

        uint64_t value = skipdb_get(db, key, key_len);
        if (value != i + 1) {
            perror("skipdb_get");
            printf("获取的值不一致 2");
            printf("i: %d, key: %s, value: %ld\n", i, key, value);
            exit(-1);
        }
    }
}

// 功能测试
void test_put_del_get(skipdb_t *db) {
    char *key_prefix = "test_put_del_get|";
    uint32_t key_len = (uint32_t) (strlen(key_prefix) + 2);

    char key[key_len + 1];
    for (int i = 0; i < 100; ++i) {
        sprintf(key, "%s%02d", key_prefix, i);

        if (skipdb_put(db, key, key_len, i + 1) == -1) {
            perror("skipdb_put");
            printf("i: %d, key: %s\n", i, key);
            exit(-1);
        }

        if (i < 50) {
            uint64_t value = skipdb_del(db, key, key_len);
            if (value != i + 1) {
                perror("skipdb_del 1");
                printf("i: %d, key: %s, value: %ld\n", i, key, value);
                exit(-1);
            }
        }
    }
    for (int i = 50; i < 100; ++i) {
        sprintf(key, "%s%02d", key_prefix, i);
        uint64_t value = skipdb_del(db, key, key_len);
        if (value != i + 1) {
            perror("skipdb_del 2");
            printf("i: %d, key: %s, value: %ld\n", i, key, value);
            exit(-1);
        }
    }

    for (int i = 0; i < 100; ++i) {
        sprintf(key, "%s%02d", key_prefix, i);
        if (skipdb_get(db, key, key_len) != 0) {
            perror("skipdb_get");
            printf("删除失败，仍然获取到了删除的值");
        }
    }
}

// need free
char *randString(size_t size) {
    char *str = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    size_t len = strlen(str);

    char *ret = malloc(size + 1);
    for (int i = 0; i < size; ++i) {
        ret[i] = str[random() % len];
    }
    ret[size] = '\0';
    return ret;
}

double delta(struct timeval t1, struct timeval t2) {
    return ((double) t2.tv_sec + (double) t2.tv_usec / 1000000) - ((double) t1.tv_sec + (double) t1.tv_usec / 1000000);
}

// benchmark
void benchmark(skipdb_t *db, const int count) {
    const int key_size = 32;

    // 这里是测试，就不刻意的 free 了
    char **data = malloc(sizeof(char *) * count);

    for (int i = 0; i < count; ++i) {
        data[i] = randString(key_size);
    }

    { // put
        struct timeval t1, t2;
        gettimeofday(&t1, NULL);

        for (int i = 0; i < count; ++i) {
            printf("key: %s, value: %d\n", data[i], i + 1);
            fflush(stdout);
            if (skipdb_put(db, data[i], key_size, i + 1) == -1) {
                perror("skipdb_put");
                exit(-1);
            }
        }

        gettimeofday(&t2, NULL);

        double d = delta(t1, t2);
        printf("PUT time: %lf, %lfw/s\n", d, count / d / 10000);
    }

    sleep(5);

    { // get
        struct timeval t1, t2;
        gettimeofday(&t1, NULL);

        for (int i = 0; i < count; ++i) {
            uint64_t value = skipdb_get(db, data[i], key_size);
            if (value != i + 1) {
                perror("skipdb_get");
                printf("i: %d, key: %s, value: %ld\n", i, data[i], value);
                exit(-1);
            }
        }

        gettimeofday(&t2, NULL);

        double d = delta(t1, t2);
        printf("GET time: %lf, %lfw/s\n", d, count / d / 10000);
    }
}

int main(int argc, const char *argv[]) {
    if (argc != 2) {
        printf("./skipdb <benchmark count>\n");
        exit(-1);
    }

    int count = atoi(argv[1]);

    { // 功能
        skipdb_t *db = skipdb_open("test.db");
        if (db == NULL) {
            perror("skipdb_open");
            exit(-1);
        }

        printf("test_put_get:\n");
        fflush(stdout);
        test_put_get(db);

        printf("test_put_del_get:\n");
        fflush(stdout);
        test_put_del_get(db);

        if (skipdb_close(db) == -1) {
            perror("skipdb_close");
            exit(-1);
        }
    }

    if (count > 0) { // 性能
        skipdb_t *db = skipdb_open("benchmark.db");
        if (db == NULL) {
            perror("skipdb_open");
            exit(-1);
        }

        benchmark(db, count);

        if (skipdb_close(db) == -1) {
            perror("skipdb_close");
            exit(-1);
        }
    }

    return 0;
}
