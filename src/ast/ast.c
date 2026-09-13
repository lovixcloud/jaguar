#include "jag/ast.h"
#include <stdlib.h>
#include <string.h>

JagASTNode *jag_ast_create_node(JagASTKind kind, JagSourceLoc loc) {
    JagASTNode *node = calloc(1, sizeof(JagASTNode));
    if (!node) return NULL;
    node->kind = kind;
    node->loc = loc;
    return node;
}

void jag_ast_free(JagASTNode *node) {
    if (!node) return;

    switch (node->kind) {
        case AST_PROGRAM:
            for (size_t i = 0; i < node->as.program.count; i++) {
                jag_ast_free(node->as.program.stmts[i]);
            }
            free(node->as.program.stmts);
            break;

        case AST_VAR_DECL:
            free(node->as.var_decl.name);
            free(node->as.var_decl.type.name);
            jag_ast_free(node->as.var_decl.init);
            break;

        case AST_FUN_DECL:
            free(node->as.fun_decl.name);
            for (size_t i = 0; i < node->as.fun_decl.param_count; i++) {
                free(node->as.fun_decl.params[i].name);
                free(node->as.fun_decl.params[i].type.name);
            }
            free(node->as.fun_decl.params);
            free(node->as.fun_decl.return_type.name);
            jag_ast_free(node->as.fun_decl.body);
            break;

        case AST_CLASS_DECL:
            free(node->as.class_decl.name);
            for (size_t i = 0; i < node->as.class_decl.field_count; i++) {
                free(node->as.class_decl.fields[i].name);
                free(node->as.class_decl.fields[i].type.name);
            }
            free(node->as.class_decl.fields);
            for (size_t i = 0; i < node->as.class_decl.method_count; i++) {
                jag_ast_free(node->as.class_decl.methods[i]);
            }
            free(node->as.class_decl.methods);
            break;

        case AST_STRUCT_DECL:
            free(node->as.struct_decl.name);
            for (size_t i = 0; i < node->as.struct_decl.field_count; i++) {
                free(node->as.struct_decl.fields[i].name);
                free(node->as.struct_decl.fields[i].type.name);
            }
            free(node->as.struct_decl.fields);
            break;

        case AST_ENUM_DECL:
            free(node->as.enum_decl.name);
            for (size_t i = 0; i < node->as.enum_decl.enum_count; i++) {
                free(node->as.enum_decl.enumerators[i]);
            }
            free(node->as.enum_decl.enumerators);
            break;

        case AST_IF:
            jag_ast_free(node->as.if_stmt.cond);
            jag_ast_free(node->as.if_stmt.then_branch);
            for (size_t i = 0; i < node->as.if_stmt.elif_count; i++) {
                jag_ast_free(node->as.if_stmt.elif_conds[i]);
                jag_ast_free(node->as.if_stmt.elif_branches[i]);
            }
            free(node->as.if_stmt.elif_conds);
            free(node->as.if_stmt.elif_branches);
            jag_ast_free(node->as.if_stmt.else_branch);
            break;

        case AST_LOOP:
            jag_ast_free(node->as.loop_stmt.cond);
            jag_ast_free(node->as.loop_stmt.body);
            break;

        case AST_DO_LOOP:
            jag_ast_free(node->as.do_loop_stmt.body);
            jag_ast_free(node->as.do_loop_stmt.cond);
            break;

        case AST_FOR_IN:
            free(node->as.for_in_stmt.var_name);
            jag_ast_free(node->as.for_in_stmt.collection);
            jag_ast_free(node->as.for_in_stmt.body);
            break;

        case AST_RETURN:
            jag_ast_free(node->as.return_stmt.expr);
            break;

        case AST_CALL:
            jag_ast_free(node->as.call_expr.callee);
            for (size_t i = 0; i < node->as.call_expr.arg_count; i++) {
                jag_ast_free(node->as.call_expr.args[i]);
            }
            free(node->as.call_expr.args);
            break;

        case AST_BINARY:
            jag_ast_free(node->as.binary_expr.left);
            jag_ast_free(node->as.binary_expr.right);
            break;

        case AST_UNARY:
            jag_ast_free(node->as.unary_expr.operand);
            break;

        case AST_LITERAL:
            if (node->as.literal.lit_kind == LITERAL_STRING) {
                free(node->as.literal.val.string_val);
            }
            break;

        case AST_IDENTIFIER:
            free(node->as.identifier.name);
            break;

        case AST_ARRAY_LITERAL:
            for (size_t i = 0; i < node->as.array_literal.count; i++) {
                jag_ast_free(node->as.array_literal.elements[i]);
            }
            free(node->as.array_literal.elements);
            break;

        case AST_DATA_LITERAL:
            for (size_t i = 0; i < node->as.data_literal.count; i++) {
                free(node->as.data_literal.entries[i].key);
                jag_ast_free(node->as.data_literal.entries[i].val);
            }
            free(node->as.data_literal.entries);
            break;

        case AST_IMPORT:
            free(node->as.import_stmt.path);
            break;

        case AST_EXPORT:
            jag_ast_free(node->as.export_stmt.decl);
            break;

        case AST_LIVE_ACTIVATION:
            free(node->as.live_activation.value);
            break;

        case AST_LIVE_DEG:
            free(node->as.live_deg.type_name);
            jag_ast_free(node->as.live_deg.expected);
            jag_ast_free(node->as.live_deg.var_expr);
            break;

        case AST_MEMBER_ACCESS:
            jag_ast_free(node->as.member_access.object);
            free(node->as.member_access.member);
            break;

        case AST_INDEX_ACCESS:
            jag_ast_free(node->as.index_access.array_or_map);
            jag_ast_free(node->as.index_access.index);
            break;

        case AST_TERNARY:
            jag_ast_free(node->as.ternary_expr.cond);
            jag_ast_free(node->as.ternary_expr.then_expr);
            jag_ast_free(node->as.ternary_expr.else_expr);
            break;

        case AST_EXPR_STMT:
            jag_ast_free(node->as.expr_stmt.expr);
            break;

        case AST_BLOCK:
            for (size_t i = 0; i < node->as.block.count; i++) {
                jag_ast_free(node->as.block.stmts[i]);
            }
            free(node->as.block.stmts);
            break;

        case AST_ASSIGNMENT:
            jag_ast_free(node->as.assignment.target);
            jag_ast_free(node->as.assignment.value);
            break;
    }

    free(node);
}
