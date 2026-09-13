#ifndef JAG_COMMON_H
#define JAG_COMMON_H

#define _POSIX_C_SOURCE 200809L
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define JAG_VERSION "0.1.0"

typedef struct {
    const char *file;
    int line;
    int column;
} JagSourceLoc;

static inline char *jag_strdup(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    char *dup = malloc(len + 1);
    if (dup) {
        memcpy(dup, s, len + 1);
    }
    return dup;
}

#endif // JAG_COMMON_H
