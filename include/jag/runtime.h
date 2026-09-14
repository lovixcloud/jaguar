#ifndef JAG_RUNTIME_H
#define JAG_RUNTIME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    JAG_VAL_NUM,
    JAG_VAL_DECIMAL,
    JAG_VAL_BOOL,
    JAG_VAL_STRING,
    JAG_VAL_ARRAY,
    JAG_VAL_NULL
} JagValKind;

typedef struct JagVal JagVal;
typedef struct JagArray JagArray;

struct JagArray {
    JagVal *items;
    size_t count;
    size_t capacity;
};

struct JagVal {
    JagValKind kind;
    union {
        int64_t num;
        double decimal;
        bool boolean;
        char *string;
        JagArray *array;
    } as;
};

void *jag_alloc(size_t size);
void *jag_realloc(void *ptr, size_t size);
void jag_free(void *ptr);

JagVal jag_val_num(int64_t n);
JagVal jag_val_decimal(double d);
JagVal jag_val_bool(bool b);
JagVal jag_val_string(const char *str);
JagVal jag_val_array(JagArray *arr);
JagVal jag_val_null(void);

bool jag_val_equals(JagVal a, JagVal b);
char *jag_val_to_string(JagVal v);
char *jag_str_concat(const char *a, const char *b);

JagArray *jag_array_create(size_t initial_cap);
void jag_array_append(JagArray *arr, JagVal val);
void jag_array_insert(JagArray *arr, size_t index, JagVal val);
void jag_array_delete(JagArray *arr, size_t index);
void jag_array_sort(JagArray *arr);
JagVal jag_array_get(JagArray *arr, size_t index);
void jag_array_set(JagArray *arr, size_t index, JagVal val);
void jag_array_free(JagArray *arr);

void jag_live_on_val(JagVal v);
void jag_live_on_str(const char *str);
void jag_live_on_num(int64_t num);
void jag_live_on_decimal(double d);
void jag_live_on_bool(bool b);

char *jag_live_in(void);
void jag_live_log(const char *msg);
void jag_live_deg(const char *type_name, JagVal expected, JagVal actual);

typedef struct {
    FILE *fp;
    char *path;
} JagFile;

JagFile *jag_file_open(const char *path, const char *mode);
char *jag_file_read(JagFile *file);
void jag_file_close(JagFile *file);

typedef struct {
    char *path;
    void *dir_handle;
} JagDir;

JagDir *jag_dir_on(const char *path);
JagArray *jag_dir_read(JagDir *dir);
void jag_dir_close(JagDir *dir);
void jag_dir_del(const char *path);

void jag_net_http_serve(int port, const char *response_body);

#endif // JAG_RUNTIME_H
