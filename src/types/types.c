#include "jag/types.h"
#include <stdlib.h>
#include <string.h>

static JagType g_type_num = { .kind = JAG_TYPE_NUM, .name = "num" };
static JagType g_type_decimal = { .kind = JAG_TYPE_DECIMAL, .name = "decimal" };
static JagType g_type_bool = { .kind = JAG_TYPE_BOOL, .name = "bool" };
static JagType g_type_string = { .kind = JAG_TYPE_STRING, .name = "string" };
static JagType g_type_void = { .kind = JAG_TYPE_VOID, .name = "void" };
static JagType g_type_mixed = { .kind = JAG_TYPE_MIXED, .name = "mixed" };
static JagType g_type_vector = { .kind = JAG_TYPE_VECTOR, .name = "vector" };
static JagType g_type_matrix = { .kind = JAG_TYPE_MATRIX, .name = "matrix" };
static JagType g_type_data = { .kind = JAG_TYPE_DATA, .name = "data" };
static JagType g_type_unknown = { .kind = JAG_TYPE_UNKNOWN, .name = "unknown" };

JagType *jag_type_primitive(JagTypeKind kind) {
    switch (kind) {
        case JAG_TYPE_NUM: return &g_type_num;
        case JAG_TYPE_DECIMAL: return &g_type_decimal;
        case JAG_TYPE_BOOL: return &g_type_bool;
        case JAG_TYPE_STRING: return &g_type_string;
        case JAG_TYPE_VOID: return &g_type_void;
        case JAG_TYPE_MIXED: return &g_type_mixed;
        case JAG_TYPE_VECTOR: return &g_type_vector;
        case JAG_TYPE_MATRIX: return &g_type_matrix;
        case JAG_TYPE_DATA: return &g_type_data;
        default: return &g_type_unknown;
    }
}

JagType *jag_type_array(JagType *element_type) {
    JagType *t = calloc(1, sizeof(JagType));
    t->kind = JAG_TYPE_ARRAY;
    t->as.array.element_type = element_type;
    size_t len = strlen(jag_type_to_string(element_type)) + 3;
    t->name = malloc(len);
    snprintf(t->name, len, "%s[]", jag_type_to_string(element_type));
    return t;
}

JagType *jag_type_custom(JagTypeKind kind, const char *name) {
    JagType *t = calloc(1, sizeof(JagType));
    t->kind = kind;
    t->name = jag_strdup(name ? name : "custom");
    return t;
}

JagType *jag_type_function(JagType **params, size_t param_count, JagType *ret) {
    JagType *t = calloc(1, sizeof(JagType));
    t->kind = JAG_TYPE_FUNCTION;
    t->as.function.param_types = malloc(param_count * sizeof(JagType *));
    memcpy(t->as.function.param_types, params, param_count * sizeof(JagType *));
    t->as.function.param_count = param_count;
    t->as.function.return_type = ret;
    t->name = jag_strdup("function");
    return t;
}

bool jag_type_equals(const JagType *a, const JagType *b) {
    if (!a || !b) return false;
    if (a == b) return true;
    if (a->kind != b->kind) {
        return false;
    }
    if (a->kind == JAG_TYPE_ARRAY) {
        return jag_type_equals(a->as.array.element_type, b->as.array.element_type);
    }
    if (a->kind == JAG_TYPE_STRUCT || a->kind == JAG_TYPE_CLASS || a->kind == JAG_TYPE_ENUM) {
        if (!a->name || !b->name) return false;
        return strcmp(a->name, b->name) == 0;
    }
    return true;
}

bool jag_type_is_assignable(const JagType *target, const JagType *source) {
    if (!target || !source) return false;
    if (target->kind == JAG_TYPE_MIXED) return true;
    if (jag_type_equals(target, source)) return true;
    if (target->kind == JAG_TYPE_DECIMAL && source->kind == JAG_TYPE_NUM) return true;
    if (target->kind == JAG_TYPE_NUM && source->kind == JAG_TYPE_DECIMAL) return true;
    return false;
}

const char *jag_type_to_string(const JagType *type) {
    if (!type) return "unknown";
    if (type->name) return type->name;
    return "unknown";
}

void jag_type_free(JagType *type) {
    if (!type) return;
    if (type == &g_type_num || type == &g_type_decimal || type == &g_type_bool ||
        type == &g_type_string || type == &g_type_void || type == &g_type_mixed ||
        type == &g_type_vector || type == &g_type_matrix || type == &g_type_data ||
        type == &g_type_unknown) {
        return;
    }
    if (type->name) free(type->name);
    if (type->kind == JAG_TYPE_FUNCTION) {
        free(type->as.function.param_types);
    }
    free(type);
}
