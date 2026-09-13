#include "jag/semantic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void jag_semantic_init(JagSemanticAnalyzer *analyzer, const char *source) {
    analyzer->current_scope = jag_symbol_table_create(SCOPE_GLOBAL, NULL);
    jag_diag_init(&analyzer->diag, source);
    analyzer->programmatic_live_enabled = false;
}

void jag_semantic_cleanup(JagSemanticAnalyzer *analyzer) {
    while (analyzer->current_scope) {
        JagSymbolTable *parent = analyzer->current_scope->parent;
        jag_symbol_table_free(analyzer->current_scope);
        analyzer->current_scope = parent;
    }
}

static JagType *resolve_type_spec(JagSemanticAnalyzer *analyzer, const JagTypeSpec *spec) {
    if (!spec || !spec->name) return jag_type_primitive(JAG_TYPE_VOID);

    JagType *base_type = NULL;
    if (strcmp(spec->name, "num") == 0) base_type = jag_type_primitive(JAG_TYPE_NUM);
    else if (strcmp(spec->name, "decimal") == 0) base_type = jag_type_primitive(JAG_TYPE_DECIMAL);
    else if (strcmp(spec->name, "bool") == 0) base_type = jag_type_primitive(JAG_TYPE_BOOL);
    else if (strcmp(spec->name, "string") == 0) base_type = jag_type_primitive(JAG_TYPE_STRING);
    else if (strcmp(spec->name, "void") == 0) base_type = jag_type_primitive(JAG_TYPE_VOID);
    else if (strcmp(spec->name, "mixed") == 0) base_type = jag_type_primitive(JAG_TYPE_MIXED);
    else if (strcmp(spec->name, "vector") == 0) base_type = jag_type_primitive(JAG_TYPE_VECTOR);
    else if (strcmp(spec->name, "matrix") == 0) base_type = jag_type_primitive(JAG_TYPE_MATRIX);
    else if (strcmp(spec->name, "data") == 0) base_type = jag_type_primitive(JAG_TYPE_DATA);
    else {
        JagSymbol *sym = jag_symbol_table_lookup(analyzer->current_scope, spec->name);
        if (sym) {
            base_type = sym->type;
        } else {
            base_type = jag_type_custom(JAG_TYPE_STRUCT, spec->name);
        }
    }

    if (spec->is_array) {
        return jag_type_array(base_type);
    }
    return base_type;
}

static void analyze_node(JagSemanticAnalyzer *analyzer, JagASTNode *node);

JagType *jag_semantic_get_expr_type(JagSemanticAnalyzer *analyzer, JagASTNode *expr) {
    if (!expr) return jag_type_primitive(JAG_TYPE_VOID);

    switch (expr->kind) {
        case AST_LITERAL:
            switch (expr->as.literal.lit_kind) {
                case LITERAL_INT: return jag_type_primitive(JAG_TYPE_NUM);
                case LITERAL_FLOAT: return jag_type_primitive(JAG_TYPE_DECIMAL);
                case LITERAL_STRING: return jag_type_primitive(JAG_TYPE_STRING);
                case LITERAL_BOOL: return jag_type_primitive(JAG_TYPE_BOOL);
            }
            return jag_type_primitive(JAG_TYPE_VOID);

        case AST_IDENTIFIER: {
            JagSymbol *sym = jag_symbol_table_lookup(analyzer->current_scope, expr->as.identifier.name);
            if (sym) return sym->type;
            if (strcmp(expr->as.identifier.name, "live") == 0 || strcmp(expr->as.identifier.name, "File") == 0 ||
                strcmp(expr->as.identifier.name, "dir") == 0) {
                return jag_type_primitive(JAG_TYPE_DATA);
            }
            jag_diag_report(&analyzer->diag, expr->loc, "JAG-SYM-001", "Use of undeclared identifier '%s'", expr->as.identifier.name);
            return jag_type_primitive(JAG_TYPE_UNKNOWN);
        }

        case AST_BINARY: {
            JagType *left = jag_semantic_get_expr_type(analyzer, expr->as.binary_expr.left);
            JagType *right = jag_semantic_get_expr_type(analyzer, expr->as.binary_expr.right);
            JagTokenType op = expr->as.binary_expr.op;

            if (op == TOKEN_PLUS && (left->kind == JAG_TYPE_STRING || right->kind == JAG_TYPE_STRING)) {
                return jag_type_primitive(JAG_TYPE_STRING);
            }

            if (op == TOKEN_EQ_EQ || op == TOKEN_BANG_EQ || op == TOKEN_LESS || op == TOKEN_LESS_EQ ||
                op == TOKEN_GREATER || op == TOKEN_GREATER_EQ || op == TOKEN_AND || op == TOKEN_OR) {
                return jag_type_primitive(JAG_TYPE_BOOL);
            }

            if (left->kind == JAG_TYPE_DECIMAL || right->kind == JAG_TYPE_DECIMAL) {
                return jag_type_primitive(JAG_TYPE_DECIMAL);
            }
            return jag_type_primitive(JAG_TYPE_NUM);
        }

        case AST_UNARY: {
            JagTokenType op = expr->as.unary_expr.op;
            if (op == TOKEN_BANG) return jag_type_primitive(JAG_TYPE_BOOL);
            return jag_semantic_get_expr_type(analyzer, expr->as.unary_expr.operand);
        }

        case AST_TERNARY: {
            JagType *t = jag_semantic_get_expr_type(analyzer, expr->as.ternary_expr.then_expr);
            JagType *e = jag_semantic_get_expr_type(analyzer, expr->as.ternary_expr.else_expr);
            if (jag_type_equals(t, e)) return t;
            return jag_type_primitive(JAG_TYPE_MIXED);
        }

        case AST_CALL: {
            if (expr->as.call_expr.callee->kind == AST_MEMBER_ACCESS) {
                JagASTNode *obj = expr->as.call_expr.callee->as.member_access.object;
                const char *member = expr->as.call_expr.callee->as.member_access.member;

                if (obj->kind == AST_IDENTIFIER) {
                    const char *obj_name = obj->as.identifier.name;
                    if (strcmp(obj_name, "live") == 0) {
                        if (strcmp(member, "on") == 0 || strcmp(member, "log") == 0 || strcmp(member, "in") == 0) {
                            return jag_type_primitive(JAG_TYPE_VOID);
                        }
                    }
                    if (strcmp(obj_name, "File") == 0) {
                        if (strcmp(member, "open") == 0) return jag_type_custom(JAG_TYPE_STRUCT, "File");
                        if (strcmp(member, "read") == 0) return jag_type_primitive(JAG_TYPE_STRING);
                        if (strcmp(member, "close") == 0 || strcmp(member, "loop") == 0) return jag_type_primitive(JAG_TYPE_VOID);
                    }
                    if (strcmp(obj_name, "dir") == 0) {
                        if (strcmp(member, "on") == 0) return jag_type_custom(JAG_TYPE_STRUCT, "Directory");
                        if (strcmp(member, "read") == 0) return jag_type_array(jag_type_primitive(JAG_TYPE_STRING));
                        if (strcmp(member, "close") == 0 || strcmp(member, "loop") == 0 || strcmp(member, "del") == 0) return jag_type_primitive(JAG_TYPE_VOID);
                    }
                }

                if (strcmp(member, "append") == 0 || strcmp(member, "insert") == 0 ||
                    strcmp(member, "delete") == 0 || strcmp(member, "sort") == 0) {
                    return jag_type_primitive(JAG_TYPE_VOID);
                }
            } else if (expr->as.call_expr.callee->kind == AST_IDENTIFIER) {
                JagSymbol *sym = jag_symbol_table_lookup(analyzer->current_scope, expr->as.call_expr.callee->as.identifier.name);
                if (sym && sym->type && sym->type->kind == JAG_TYPE_FUNCTION) {
                    return sym->type->as.function.return_type;
                }
            }
            return jag_type_primitive(JAG_TYPE_VOID);
        }

        case AST_ARRAY_LITERAL: {
            if (expr->as.array_literal.count == 0) {
                return jag_type_array(jag_type_primitive(JAG_TYPE_MIXED));
            }
            JagType *elem0 = jag_semantic_get_expr_type(analyzer, expr->as.array_literal.elements[0]);
            bool homogeneous = true;
            for (size_t i = 1; i < expr->as.array_literal.count; i++) {
                JagType *elem_i = jag_semantic_get_expr_type(analyzer, expr->as.array_literal.elements[i]);
                if (!jag_type_equals(elem0, elem_i)) {
                    homogeneous = false;
                    break;
                }
            }
            if (homogeneous) {
                return jag_type_array(elem0);
            } else {
                return jag_type_array(jag_type_primitive(JAG_TYPE_MIXED));
            }
        }

        case AST_DATA_LITERAL:
            return jag_type_primitive(JAG_TYPE_DATA);

        case AST_MEMBER_ACCESS: {
            JagType *obj_type = jag_semantic_get_expr_type(analyzer, expr->as.member_access.object);
            if (obj_type->kind == JAG_TYPE_ENUM) {
                return obj_type;
            }
            return jag_type_primitive(JAG_TYPE_UNKNOWN);
        }

        case AST_INDEX_ACCESS: {
            JagType *target_type = jag_semantic_get_expr_type(analyzer, expr->as.index_access.array_or_map);
            if (target_type->kind == JAG_TYPE_ARRAY) {
                return target_type->as.array.element_type;
            }
            return jag_type_primitive(JAG_TYPE_MIXED);
        }

        default:
            return jag_type_primitive(JAG_TYPE_VOID);
    }
}

static void analyze_node(JagSemanticAnalyzer *analyzer, JagASTNode *node) {
    if (!node) return;

    switch (node->kind) {
        case AST_PROGRAM:
            for (size_t i = 0; i < node->as.program.count; i++) {
                analyze_node(analyzer, node->as.program.stmts[i]);
            }
            break;

        case AST_LIVE_ACTIVATION:
            analyzer->programmatic_live_enabled = true;
            break;

        case AST_VAR_DECL: {
            JagType *decl_type = resolve_type_spec(analyzer, &node->as.var_decl.type);
            if (node->as.var_decl.init) {
                JagType *init_type = jag_semantic_get_expr_type(analyzer, node->as.var_decl.init);
                if (!jag_type_is_assignable(decl_type, init_type)) {
                    jag_diag_report(&analyzer->diag, node->loc, "JAG-TYPE-001",
                                    "cannot assign value of type '%s' to variable of type '%s'",
                                    jag_type_to_string(init_type), jag_type_to_string(decl_type));
                }
            }
            if (!jag_symbol_table_insert(analyzer->current_scope, node->as.var_decl.name, decl_type,
                                         node->as.var_decl.is_fixed ? SYMBOL_FIXED : SYMBOL_VAR,
                                         node->as.var_decl.is_fixed, true, node->loc)) {
                jag_diag_report(&analyzer->diag, node->loc, "JAG-SYM-002",
                                "Redeclaration of variable '%s'", node->as.var_decl.name);
            }
            break;
        }

        case AST_ASSIGNMENT: {
            if (node->as.assignment.target->kind == AST_IDENTIFIER) {
                const char *var_name = node->as.assignment.target->as.identifier.name;
                JagSymbol *sym = jag_symbol_table_lookup(analyzer->current_scope, var_name);
                if (sym) {
                    if (sym->is_fixed) {
                        jag_diag_report(&analyzer->diag, node->loc, "JAG-CONST-001",
                                        "Cannot reassign fixed constant variable '%s'", var_name);
                    }
                    JagType *val_type = jag_semantic_get_expr_type(analyzer, node->as.assignment.value);
                    if (!jag_type_is_assignable(sym->type, val_type)) {
                        jag_diag_report(&analyzer->diag, node->loc, "JAG-TYPE-001",
                                        "cannot assign value of type '%s' to variable of type '%s'",
                                        jag_type_to_string(val_type), jag_type_to_string(sym->type));
                    }
                } else if (strcmp(var_name, "live") == 0) {
                    analyzer->programmatic_live_enabled = true;
                } else {
                    jag_diag_report(&analyzer->diag, node->loc, "JAG-SYM-001",
                                    "Use of undeclared variable '%s'", var_name);
                }
            }
            break;
        }

        case AST_FUN_DECL: {
            JagType **param_types = malloc(node->as.fun_decl.param_count * sizeof(JagType *));
            for (size_t i = 0; i < node->as.fun_decl.param_count; i++) {
                param_types[i] = resolve_type_spec(analyzer, &node->as.fun_decl.params[i].type);
            }
            JagType *ret_type = resolve_type_spec(analyzer, &node->as.fun_decl.return_type);
            JagType *fn_type = jag_type_function(param_types, node->as.fun_decl.param_count, ret_type);

            jag_symbol_table_insert(analyzer->current_scope, node->as.fun_decl.name, fn_type,
                                     SYMBOL_FUN, false, true, node->loc);

            JagSymbolTable *fn_scope = jag_symbol_table_create(SCOPE_FUNCTION, analyzer->current_scope);
            analyzer->current_scope = fn_scope;

            for (size_t i = 0; i < node->as.fun_decl.param_count; i++) {
                jag_symbol_table_insert(fn_scope, node->as.fun_decl.params[i].name, param_types[i],
                                         SYMBOL_VAR, false, true, node->loc);
            }

            analyze_node(analyzer, node->as.fun_decl.body);

            analyzer->current_scope = fn_scope->parent;
            jag_symbol_table_free(fn_scope);
            free(param_types);
            break;
        }

        case AST_STRUCT_DECL: {
            JagType *struct_type = jag_type_custom(JAG_TYPE_STRUCT, node->as.struct_decl.name);
            jag_symbol_table_insert(analyzer->current_scope, node->as.struct_decl.name, struct_type,
                                     SYMBOL_STRUCT, false, true, node->loc);
            break;
        }

        case AST_CLASS_DECL: {
            JagType *class_type = jag_type_custom(JAG_TYPE_CLASS, node->as.class_decl.name);
            jag_symbol_table_insert(analyzer->current_scope, node->as.class_decl.name, class_type,
                                     SYMBOL_CLASS, false, true, node->loc);
            break;
        }

        case AST_ENUM_DECL: {
            JagType *enum_type = jag_type_custom(JAG_TYPE_ENUM, node->as.enum_decl.name);
            jag_symbol_table_insert(analyzer->current_scope, node->as.enum_decl.name, enum_type,
                                     SYMBOL_ENUM, false, true, node->loc);
            break;
        }

        case AST_IF:
            analyze_node(analyzer, node->as.if_stmt.cond);
            analyze_node(analyzer, node->as.if_stmt.then_branch);
            for (size_t i = 0; i < node->as.if_stmt.elif_count; i++) {
                analyze_node(analyzer, node->as.if_stmt.elif_conds[i]);
                analyze_node(analyzer, node->as.if_stmt.elif_branches[i]);
            }
            if (node->as.if_stmt.else_branch) analyze_node(analyzer, node->as.if_stmt.else_branch);
            break;

        case AST_LOOP:
            analyze_node(analyzer, node->as.loop_stmt.cond);
            analyze_node(analyzer, node->as.loop_stmt.body);
            break;

        case AST_DO_LOOP:
            analyze_node(analyzer, node->as.do_loop_stmt.body);
            analyze_node(analyzer, node->as.do_loop_stmt.cond);
            break;

        case AST_FOR_IN: {
            analyze_node(analyzer, node->as.for_in_stmt.collection);
            JagSymbolTable *loop_scope = jag_symbol_table_create(SCOPE_BLOCK, analyzer->current_scope);
            analyzer->current_scope = loop_scope;

            JagType *coll_type = jag_semantic_get_expr_type(analyzer, node->as.for_in_stmt.collection);
            JagType *elem_type = (coll_type->kind == JAG_TYPE_ARRAY) ? coll_type->as.array.element_type : jag_type_primitive(JAG_TYPE_MIXED);

            jag_symbol_table_insert(loop_scope, node->as.for_in_stmt.var_name, elem_type,
                                     SYMBOL_VAR, false, true, node->loc);

            analyze_node(analyzer, node->as.for_in_stmt.body);

            analyzer->current_scope = loop_scope->parent;
            jag_symbol_table_free(loop_scope);
            break;
        }

        case AST_BLOCK: {
            JagSymbolTable *block_scope = jag_symbol_table_create(SCOPE_BLOCK, analyzer->current_scope);
            analyzer->current_scope = block_scope;

            for (size_t i = 0; i < node->as.block.count; i++) {
                analyze_node(analyzer, node->as.block.stmts[i]);
            }

            analyzer->current_scope = block_scope->parent;
            jag_symbol_table_free(block_scope);
            break;
        }

        case AST_LIVE_DEG: {
            JagTypeSpec spec = { node->as.live_deg.type_name, false };
            JagType *expected_spec_type = resolve_type_spec(analyzer, &spec);
            JagType *actual_exp_type = jag_semantic_get_expr_type(analyzer, node->as.live_deg.expected);
            JagType *actual_var_type = jag_semantic_get_expr_type(analyzer, node->as.live_deg.var_expr);

            if (!jag_type_is_assignable(expected_spec_type, actual_exp_type) ||
                !jag_type_is_assignable(expected_spec_type, actual_var_type)) {

                jag_diag_report(&analyzer->diag, node->loc, "JAG-DEG-001",
                                "live.deg check failed: expected type '%s' but got '%s' and '%s'",
                                node->as.live_deg.type_name,
                                jag_type_to_string(actual_exp_type),
                                jag_type_to_string(actual_var_type));
            }
            break;
        }

        case AST_EXPR_STMT:
            analyze_node(analyzer, node->as.expr_stmt.expr);
            break;

        default:
            break;
    }
}

bool jag_semantic_analyze(JagSemanticAnalyzer *analyzer, JagASTNode *program) {
    analyze_node(analyzer, program);
    return analyzer->diag.error_count == 0;
}
