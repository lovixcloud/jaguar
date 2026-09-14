#include "jag/common.h"
#include "jag/runtime.h"
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *jag_alloc(size_t size) {
    void *ptr = malloc(size);
    if (!ptr && size > 0) {
        fprintf(stderr, "Jaguar Runtime Error: Out of memory\n");
        exit(1);
    }
    return ptr;
}

void *jag_realloc(void *ptr, size_t size) {
    void *new_ptr = realloc(ptr, size);
    if (!new_ptr && size > 0) {
        fprintf(stderr, "Jaguar Runtime Error: Out of memory\n");
        exit(1);
    }
    return new_ptr;
}

void jag_free(void *ptr) {
    if (ptr) free(ptr);
}

JagVal jag_val_num(int64_t n) {
    JagVal v;
    v.kind = JAG_VAL_NUM;
    v.as.num = n;
    return v;
}

JagVal jag_val_decimal(double d) {
    JagVal v;
    v.kind = JAG_VAL_DECIMAL;
    v.as.decimal = d;
    return v;
}

JagVal jag_val_bool(bool b) {
    JagVal v;
    v.kind = JAG_VAL_BOOL;
    v.as.boolean = b;
    return v;
}

JagVal jag_val_string(const char *str) {
    JagVal v;
    v.kind = JAG_VAL_STRING;
    v.as.string = jag_strdup(str ? str : "");
    return v;
}

JagVal jag_val_array(JagArray *arr) {
    JagVal v;
    v.kind = JAG_VAL_ARRAY;
    v.as.array = arr;
    return v;
}

JagVal jag_val_null(void) {
    JagVal v;
    v.kind = JAG_VAL_NULL;
    return v;
}

char *jag_val_to_string(JagVal v) {
    char buf[128];
    switch (v.kind) {
        case JAG_VAL_NUM:
            snprintf(buf, sizeof(buf), "%ld", (long)v.as.num);
            return jag_strdup(buf);
        case JAG_VAL_DECIMAL:
            snprintf(buf, sizeof(buf), "%g", v.as.decimal);
            return jag_strdup(buf);
        case JAG_VAL_BOOL:
            return jag_strdup(v.as.boolean ? "true" : "false");
        case JAG_VAL_STRING:
            return jag_strdup(v.as.string ? v.as.string : "");
        case JAG_VAL_ARRAY:
            return jag_strdup("[array]");
        case JAG_VAL_NULL:
        default:
            return jag_strdup("null");
    }
}

char *jag_str_concat(const char *a, const char *b) {
    if (!a) a = "";
    if (!b) b = "";
    size_t la = strlen(a);
    size_t lb = strlen(b);
    char *res = malloc(la + lb + 1);
    memcpy(res, a, la);
    memcpy(res + la, b, lb);
    res[la + lb] = '\0';
    return res;
}

JagArray *jag_array_create(size_t initial_cap) {
    JagArray *arr = jag_alloc(sizeof(JagArray));
    arr->capacity = initial_cap > 0 ? initial_cap : 4;
    arr->count = 0;
    arr->items = jag_alloc(arr->capacity * sizeof(JagVal));
    return arr;
}

void jag_array_append(JagArray *arr, JagVal val) {
    if (!arr) return;
    if (arr->count >= arr->capacity) {
        arr->capacity *= 2;
        arr->items = jag_realloc(arr->items, arr->capacity * sizeof(JagVal));
    }
    arr->items[arr->count++] = val;
}

void jag_array_insert(JagArray *arr, size_t index, JagVal val) {
    if (!arr || index > arr->count) return;
    if (arr->count >= arr->capacity) {
        arr->capacity *= 2;
        arr->items = jag_realloc(arr->items, arr->capacity * sizeof(JagVal));
    }
    memmove(&arr->items[index + 1], &arr->items[index], (arr->count - index) * sizeof(JagVal));
    arr->items[index] = val;
    arr->count++;
}

void jag_array_delete(JagArray *arr, size_t index) {
    if (!arr || index >= arr->count) return;
    memmove(&arr->items[index], &arr->items[index + 1], (arr->count - index - 1) * sizeof(JagVal));
    arr->count--;
}

static int compare_vals(const void *a, const void *b) {
    const JagVal *va = (const JagVal *)a;
    const JagVal *vb = (const JagVal *)b;
    if (va->kind == JAG_VAL_NUM && vb->kind == JAG_VAL_NUM) {
        return (va->as.num > vb->as.num) - (va->as.num < vb->as.num);
    }
    if (va->kind == JAG_VAL_DECIMAL && vb->kind == JAG_VAL_DECIMAL) {
        return (va->as.decimal > vb->as.decimal) - (va->as.decimal < vb->as.decimal);
    }
    return 0;
}

void jag_array_sort(JagArray *arr) {
    if (!arr || arr->count <= 1) return;
    qsort(arr->items, arr->count, sizeof(JagVal), compare_vals);
}

JagVal jag_array_get(JagArray *arr, size_t index) {
    if (!arr || index >= arr->count) return jag_val_null();
    return arr->items[index];
}

void jag_array_set(JagArray *arr, size_t index, JagVal val) {
    if (!arr || index >= arr->count) return;
    arr->items[index] = val;
}

void jag_array_free(JagArray *arr) {
    if (!arr) return;
    jag_free(arr->items);
    jag_free(arr);
}

void jag_live_on_val(JagVal v) {
    char *s = jag_val_to_string(v);
    printf("%s\n", s);
    free(s);
}

void jag_live_on_str(const char *str) {
    printf("%s\n", str ? str : "");
}

void jag_live_on_num(int64_t num) {
    printf("%ld\n", (long)num);
}

void jag_live_on_decimal(double d) {
    printf("%g\n", d);
}

void jag_live_on_bool(bool b) {
    printf("%s\n", b ? "true" : "false");
}

char *jag_live_in(void) {
    char buf[1024];
    if (fgets(buf, sizeof(buf), stdin)) {
        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';
        return jag_strdup(buf);
    }
    return jag_strdup("");
}

void jag_live_log(const char *msg) {
    printf("[jaguar:log] %s\n", msg ? msg : "");
}

void jag_live_deg(const char *type_name, JagVal expected, JagVal actual) {
    char *e_str = jag_val_to_string(expected);
    char *a_str = jag_val_to_string(actual);
    bool match = (strcmp(e_str, a_str) == 0);
    printf("[jaguar:deg] check %s: expected=%s, actual=%s -> %s\n",
           type_name ? type_name : "unknown", e_str, a_str, match ? "PASSED" : "FAILED");
    free(e_str);
    free(a_str);
}

bool jag_val_equals(JagVal a, JagVal b) {
    if (a.kind != b.kind) return false;
    switch (a.kind) {
        case JAG_VAL_NUM: return a.as.num == b.as.num;
        case JAG_VAL_DECIMAL: return a.as.decimal == b.as.decimal;
        case JAG_VAL_BOOL: return a.as.boolean == b.as.boolean;
        case JAG_VAL_STRING: return strcmp(a.as.string ? a.as.string : "", b.as.string ? b.as.string : "") == 0;
        default: return false;
    }
}
