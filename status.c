#include <stddef.h>
#include "status.h"

status_t status_ok() {
    status_t st = {
            .errno = 0,
            .error = NULL,
    };
    return st;
}

status_t status_(int errno, const char *error) {
    status_t st = {
            .errno = errno,
            .error = error,
    };
    return st;
}
