#ifndef JAG_AST_H
#define JAG_AST_H

#include "jag/common.h"
#include "jag/lexer.h"

typedef enum {
    AST_PROGRAM,
    AST_VAR_DECL,
    AST_FUN_DECL,
    AST_CLASS_DECL,
    AST_STRUCT_DECL,
    AST_ENUM_DECL,
    AST_IF,
    AST_LOOP,
    AST_DO_LOOP,
    AST_FOR_IN,
    AST_RETURN,
    AST_CALL,
    AST_BINARY,
    AST_UNARY,
    AST_LITERAL,
    AST_IDENTIFIER,
    AST_ARRAY_LITERAL,
    AST_DATA_LITERAL,
    AST_IMPORT,
    AST_EXPORT,
    AST_LIVE_ACTIVATION,
    AST_LIVE_DEG,
    AST_MEMBER_ACCESS,
    AST_INDEX_ACCESS,
    AST_TERNARY,
    AST_EXPR_STMT,
    AST_BLOCK,
    AST_ASSIGNMENT
} JagASTKind;

typedef struct {
    char *name;
    bool is_array;
} JagTypeSpec;

typedef struct JagASTNode JagASTNode;

typedef struct {
    char *name;
    JagTypeSpec type;
} JagParam;

typedef struct {
    char *name;
    JagTypeSpec type;
    bool is_public;
} JagField;

typedef struct {
    char *key;
    JagASTNode *val;
} JagDataEntry;

struct JagASTNode {
    JagASTKind kind;
    JagSourceLoc loc;

    union {
        struct {
            JagASTNode **stmts;
            size_t count;
        } program;

        struct {
            char *name;
            JagTypeSpec type;
            JagASTNode *init;
            bool is_fixed;
        } var_decl;

        struct {
            char *name;
            JagParam *params;
            size_t param_count;
            JagTypeSpec return_type;
            JagASTNode *body;
            bool is_async;
        } fun_decl;

        struct {
            char *name;
            JagField *fields;
            size_t field_count;
            JagASTNode **methods;
            size_t method_count;
        } class_decl;

        struct {
            char *name;
            JagField *fields;
            size_t field_count;
        } struct_decl;

        struct {
            char *name;
            char **enumerators;
            size_t enum_count;
        } enum_decl;

        struct {
            JagASTNode *cond;
            JagASTNode *then_branch;
            JagASTNode **elif_conds;
            JagASTNode **elif_branches;
            size_t elif_count;
            JagASTNode *else_branch;
        } if_stmt;

        struct {
            JagASTNode *cond;
            JagASTNode *body;
        } loop_stmt;

        struct {
            JagASTNode *body;
            JagASTNode *cond;
        } do_loop_stmt;

        struct {
            char *var_name;
            JagASTNode *collection;
            JagASTNode *body;
        } for_in_stmt;

        struct {
            JagASTNode *expr;
        } return_stmt;

        struct {
            JagASTNode *callee;
            JagASTNode **args;
            size_t arg_count;
        } call_expr;

        struct {
            JagTokenType op;
            JagASTNode *left;
            JagASTNode *right;
        } binary_expr;

        struct {
            JagTokenType op;
            JagASTNode *operand;
        } unary_expr;

        struct {
            enum { LITERAL_INT, LITERAL_FLOAT, LITERAL_STRING, LITERAL_BOOL } lit_kind;
            union {
                int64_t int_val;
                double float_val;
                char *string_val;
                bool bool_val;
            } val;
        } literal;

        struct {
            char *name;
        } identifier;

        struct {
            JagASTNode **elements;
            size_t count;
        } array_literal;

        struct {
            JagDataEntry *entries;
            size_t count;
        } data_literal;

        struct {
            char *path;
        } import_stmt;

        struct {
            JagASTNode *decl;
        } export_stmt;

        struct {
            char *value;
        } live_activation;

        struct {
            char *type_name;
            JagASTNode *expected;
            JagASTNode *var_expr;
        } live_deg;

        struct {
            JagASTNode *object;
            char *member;
        } member_access;

        struct {
            JagASTNode *array_or_map;
            JagASTNode *index;
        } index_access;

        struct {
            JagASTNode *cond;
            JagASTNode *then_expr;
            JagASTNode *else_expr;
        } ternary_expr;

        struct {
            JagASTNode *expr;
        } expr_stmt;

        struct {
            JagASTNode **stmts;
            size_t count;
        } block;

        struct {
            JagASTNode *target;
            JagTokenType op;
            JagASTNode *value;
        } assignment;
    } as;
};

JagASTNode *jag_ast_create_node(JagASTKind kind, JagSourceLoc loc);
void jag_ast_free(JagASTNode *node);

#endif // JAG_AST_H
