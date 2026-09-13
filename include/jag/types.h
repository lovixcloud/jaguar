#ifndef JAG_TYPES_H
#define JAG_TYPES_H

#include "jag/common.h"

typedef enum {
    JAG_TYPE_UNKNOWN = 0,
    JAG_TYPE_NUM,
    JAG_TYPE_DECIMAL,
    JAG_TYPE_BOOL,
    JAG_TYPE_STRING,
    JAG_TYPE_VOID,
    JAG_TYPE_MIXED,
    JAG_TYPE_VECTOR,
    JAG_TYPE_MATRIX,
    JAG_TYPE_ARRAY,
    JAG_TYPE_STRUCT,
    JAG_TYPE_CLASS,
    JAG_TYPE_ENUM,
    JAG_TYPE_DATA,
    JAG_TYPE_FUNCTION
} JagTypeKind;

typedef struct JagType JagType;

struct JagType {
    JagTypeKind kind;
    char *name;
    union {
        struct {
            JagType *element_type;
        } array;

        struct {
            JagType **param_types;
            size_t param_count;
            JagType *return_type;
        } function;
    } as;
};

JagType *jag_type_primitive(JagTypeKind kind);
JagType *jag_type_array(JagType *element_type);
JagType *jag_type_custom(JagTypeKind kind, const char *name);
JagType *jag_type_function(JagType **params, size_t param_count, JagType *ret);
bool jag_type_equals(const JagType *a, const JagType *b);
bool jag_type_is_assignable(const JagType *target, const JagType *source);
const char *jag_type_to_string(const JagType *type);
void jag_type_free(JagType *type);

#endif // JAG_TYPES_H
