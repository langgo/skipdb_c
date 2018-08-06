//
// Created by fanzhaobing on 2018/7/19.
//

#ifndef SKIPDB_STATUS_H
#define SKIPDB_STATUS_H

#include <stdbool.h>

typedef struct {
    int errno;
    const char *error;
} status_t;

status_t status_ok();

status_t status_(int errno, const char *error);

#endif //SKIPDB_STATUS_H
