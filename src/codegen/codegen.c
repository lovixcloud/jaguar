#include "jag/codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    FILE *f;
    int indent;
} CodegenCtx;

static void emit_indent(CodegenCtx *ctx) {
    for (int i = 0; i < ctx->indent; i++) {
        fprintf(ctx->f, "    ");
    }
}

static void gen_expr(CodegenCtx *ctx, JagASTNode *expr);

static void gen_primary_expr(CodegenCtx *ctx, JagASTNode *expr) {
    if (!expr) {
        fprintf(ctx->f, "jag_val_null()");
        return;
    }

    switch (expr->kind) {
        case AST_LITERAL:
            switch (expr->as.literal.lit_kind) {
                case LITERAL_INT:
                    fprintf(ctx->f, "jag_val_num(%ldL)", (long)expr->as.literal.val.int_val);
                    break;
                case LITERAL_FLOAT:
                    fprintf(ctx->f, "jag_val_decimal(%g)", expr->as.literal.val.float_val);
                    break;
                case LITERAL_STRING:
                    fprintf(ctx->f, "jag_val_string(\"%s\")", expr->as.literal.val.string_val ? expr->as.literal.val.string_val : "");
                    break;
                case LITERAL_BOOL:
                    fprintf(ctx->f, "jag_val_bool(%s)", expr->as.literal.val.bool_val ? "true" : "false");
                    break;
            }
            break;

        case AST_IDENTIFIER:
            fprintf(ctx->f, "%s", expr->as.identifier.name);
            break;

        case AST_BINARY: {
            if (expr->as.binary_expr.op == TOKEN_PLUS) {
                fprintf(ctx->f, "(({");
                fprintf(ctx->f, "JagVal _l = ");
                gen_expr(ctx, expr->as.binary_expr.left);
                fprintf(ctx->f, "; JagVal _r = ");
                gen_expr(ctx, expr->as.binary_expr.right);
                fprintf(ctx->f, "; ");
                fprintf(ctx->f, "(_l.kind == JAG_VAL_STRING || _r.kind == JAG_VAL_STRING) ? ");
                fprintf(ctx->f, "jag_val_string(jag_str_concat(jag_val_to_string(_l), jag_val_to_string(_r))) : ");
                fprintf(ctx->f, "(_l.kind == JAG_VAL_DECIMAL || _r.kind == JAG_VAL_DECIMAL ? ");
                fprintf(ctx->f, "jag_val_decimal((_l.kind == JAG_VAL_DECIMAL ? _l.as.decimal : (double)_l.as.num) + (_r.kind == JAG_VAL_DECIMAL ? _r.as.decimal : (double)_r.as.num)) : ");
                fprintf(ctx->f, "jag_val_num(_l.as.num + _r.as.num));");
                fprintf(ctx->f, "}))");
            } else if (expr->as.binary_expr.op == TOKEN_EQ_EQ) {
                fprintf(ctx->f, "jag_val_bool(jag_val_equals(");
                gen_expr(ctx, expr->as.binary_expr.left);
                fprintf(ctx->f, ", ");
                gen_expr(ctx, expr->as.binary_expr.right);
                fprintf(ctx->f, "))");
            } else if (expr->as.binary_expr.op == TOKEN_GREATER_EQ) {
                fprintf(ctx->f, "jag_val_bool((");
                gen_expr(ctx, expr->as.binary_expr.left);
                fprintf(ctx->f, ").as.num >= (");
                gen_expr(ctx, expr->as.binary_expr.right);
                fprintf(ctx->f, ").as.num)");
            } else if (expr->as.binary_expr.op == TOKEN_LESS_EQ) {
                fprintf(ctx->f, "jag_val_bool((");
                gen_expr(ctx, expr->as.binary_expr.left);
                fprintf(ctx->f, ").as.num <= (");
                gen_expr(ctx, expr->as.binary_expr.right);
                fprintf(ctx->f, ").as.num)");
            } else if (expr->as.binary_expr.op == TOKEN_GREATER) {
                fprintf(ctx->f, "jag_val_bool((");
                gen_expr(ctx, expr->as.binary_expr.left);
                fprintf(ctx->f, ").as.num > (");
                gen_expr(ctx, expr->as.binary_expr.right);
                fprintf(ctx->f, ").as.num)");
            } else if (expr->as.binary_expr.op == TOKEN_LESS) {
                fprintf(ctx->f, "jag_val_bool((");
                gen_expr(ctx, expr->as.binary_expr.left);
                fprintf(ctx->f, ").as.num < (");
                gen_expr(ctx, expr->as.binary_expr.right);
                fprintf(ctx->f, ").as.num)");
            } else {
                fprintf(ctx->f, "jag_val_num((");
                gen_expr(ctx, expr->as.binary_expr.left);
                fprintf(ctx->f, ").as.num + (");
                gen_expr(ctx, expr->as.binary_expr.right);
                fprintf(ctx->f, ").as.num)");
            }
            break;
        }

        case AST_ARRAY_LITERAL: {
            fprintf(ctx->f, "({\n");
            ctx->indent++;
            emit_indent(ctx);
            fprintf(ctx->f, "JagArray *_arr = jag_array_create(%zu);\n", expr->as.array_literal.count > 0 ? expr->as.array_literal.count : 4);
            for (size_t i = 0; i < expr->as.array_literal.count; i++) {
                emit_indent(ctx);
                fprintf(ctx->f, "jag_array_append(_arr, ");
                gen_expr(ctx, expr->as.array_literal.elements[i]);
                fprintf(ctx->f, ");\n");
            }
            emit_indent(ctx);
            fprintf(ctx->f, "jag_val_array(_arr);\n");
            ctx->indent--;
            emit_indent(ctx);
            fprintf(ctx->f, "})");
            break;
        }

        case AST_CALL: {
            if (expr->as.call_expr.callee->kind == AST_MEMBER_ACCESS) {
                JagASTNode *obj = expr->as.call_expr.callee->as.member_access.object;
                const char *member = expr->as.call_expr.callee->as.member_access.member;

                if (obj->kind == AST_IDENTIFIER && strcmp(obj->as.identifier.name, "live") == 0) {
                    if (strcmp(member, "on") == 0) {
                        fprintf(ctx->f, "jag_live_on_val(");
                        if (expr->as.call_expr.arg_count > 0) gen_expr(ctx, expr->as.call_expr.args[0]);
                        else fprintf(ctx->f, "jag_val_null()");
                        fprintf(ctx->f, ")");
                        return;
                    } else if (strcmp(member, "log") == 0) {
                        fprintf(ctx->f, "jag_live_log(");
                        if (expr->as.call_expr.arg_count > 0) {
                            fprintf(ctx->f, "jag_val_to_string(");
                            gen_expr(ctx, expr->as.call_expr.args[0]);
                            fprintf(ctx->f, ")");
                        } else fprintf(ctx->f, "\"\"");
                        fprintf(ctx->f, ")");
                        return;
                    } else if (strcmp(member, "in") == 0) {
                        fprintf(ctx->f, "jag_val_string(jag_live_in())");
                        return;
                    }
                }

                if (obj->kind == AST_IDENTIFIER && strcmp(obj->as.identifier.name, "File") == 0) {
                    if (strcmp(member, "open") == 0) {
                        fprintf(ctx->f, "jag_file_open(jag_val_to_string(");
                        gen_expr(ctx, expr->as.call_expr.args[0]);
                        fprintf(ctx->f, "), \"r\")");
                        return;
                    } else if (strcmp(member, "read") == 0) {
                        fprintf(ctx->f, "jag_val_string(jag_file_read(");
                        gen_expr(ctx, expr->as.call_expr.args[0]);
                        fprintf(ctx->f, "))");
                        return;
                    } else if (strcmp(member, "close") == 0) {
                        fprintf(ctx->f, "jag_file_close(");
                        gen_expr(ctx, expr->as.call_expr.args[0]);
                        fprintf(ctx->f, ")");
                        return;
                    }
                }

                if (obj->kind == AST_IDENTIFIER && strcmp(obj->as.identifier.name, "dir") == 0) {
                    if (strcmp(member, "on") == 0) {
                        fprintf(ctx->f, "jag_dir_on(jag_val_to_string(");
                        gen_expr(ctx, expr->as.call_expr.args[0]);
                        fprintf(ctx->f, "))");
                        return;
                    } else if (strcmp(member, "read") == 0) {
                        fprintf(ctx->f, "jag_val_array(jag_dir_read(");
                        gen_expr(ctx, expr->as.call_expr.args[0]);
                        fprintf(ctx->f, "))");
                        return;
                    } else if (strcmp(member, "close") == 0) {
                        fprintf(ctx->f, "jag_dir_close(");
                        gen_expr(ctx, expr->as.call_expr.args[0]);
                        fprintf(ctx->f, ")");
                        return;
                    }
                }

                if (strcmp(member, "append") == 0) {
                    fprintf(ctx->f, "jag_array_append((");
                    gen_expr(ctx, obj);
                    fprintf(ctx->f, ").as.array, ");
                    gen_expr(ctx, expr->as.call_expr.args[0]);
                    fprintf(ctx->f, ")");
                    return;
                } else if (strcmp(member, "sort") == 0) {
                    fprintf(ctx->f, "jag_array_sort((");
                    gen_expr(ctx, obj);
                    fprintf(ctx->f, ").as.array)");
                    return;
                }
            } else if (expr->as.call_expr.callee->kind == AST_IDENTIFIER) {
                fprintf(ctx->f, "%s(", expr->as.call_expr.callee->as.identifier.name);
                for (size_t i = 0; i < expr->as.call_expr.arg_count; i++) {
                    if (i > 0) fprintf(ctx->f, ", ");
                    gen_expr(ctx, expr->as.call_expr.args[i]);
                }
                fprintf(ctx->f, ")");
                return;
            }
            break;
        }

        case AST_MEMBER_ACCESS:
            gen_expr(ctx, expr->as.member_access.object);
            fprintf(ctx->f, ".%s", expr->as.member_access.member);
            break;

        case AST_INDEX_ACCESS:
            fprintf(ctx->f, "jag_array_get((");
            gen_expr(ctx, expr->as.index_access.array_or_map);
            fprintf(ctx->f, ").as.array, (size_t)(");
            gen_expr(ctx, expr->as.index_access.index);
            fprintf(ctx->f, ").as.num)");
            break;

        case AST_TERNARY:
            fprintf(ctx->f, "((");
            gen_expr(ctx, expr->as.ternary_expr.cond);
            fprintf(ctx->f, ").as.boolean ? ");
            gen_expr(ctx, expr->as.ternary_expr.then_expr);
            fprintf(ctx->f, " : ");
            gen_expr(ctx, expr->as.ternary_expr.else_expr);
            fprintf(ctx->f, ")");
            break;

        default:
            fprintf(ctx->f, "jag_val_null()");
            break;
    }
}

static void gen_expr(CodegenCtx *ctx, JagASTNode *expr) {
    gen_primary_expr(ctx, expr);
}

static void gen_stmt(CodegenCtx *ctx, JagASTNode *stmt);

static void gen_block(CodegenCtx *ctx, JagASTNode *block) {
    fprintf(ctx->f, "{\n");
    ctx->indent++;
    for (size_t i = 0; i < block->as.block.count; i++) {
        gen_stmt(ctx, block->as.block.stmts[i]);
    }
    ctx->indent--;
    emit_indent(ctx);
    fprintf(ctx->f, "}\n");
}

static void gen_stmt(CodegenCtx *ctx, JagASTNode *stmt) {
    if (!stmt) return;

    switch (stmt->kind) {
        case AST_LIVE_ACTIVATION:
            emit_indent(ctx);
            fprintf(ctx->f, "/* live = \"1\"; activated */\n");
            break;

        case AST_VAR_DECL:
            emit_indent(ctx);
            fprintf(ctx->f, "JagVal %s = ", stmt->as.var_decl.name);
            if (stmt->as.var_decl.init) {
                gen_expr(ctx, stmt->as.var_decl.init);
            } else {
                fprintf(ctx->f, "jag_val_null()");
            }
            fprintf(ctx->f, ";\n");
            break;

        case AST_ASSIGNMENT:
            emit_indent(ctx);
            if (stmt->as.assignment.op == TOKEN_PLUS_EQ) {
                fprintf(ctx->f, "%s = jag_val_num(%s.as.num + (",
                        stmt->as.assignment.target->as.identifier.name,
                        stmt->as.assignment.target->as.identifier.name);
                gen_expr(ctx, stmt->as.assignment.value);
                fprintf(ctx->f, ").as.num);\n");
            } else {
                fprintf(ctx->f, "%s = ", stmt->as.assignment.target->as.identifier.name);
                gen_expr(ctx, stmt->as.assignment.value);
                fprintf(ctx->f, ";\n");
            }
            break;

        case AST_FUN_DECL:
            emit_indent(ctx);
            fprintf(ctx->f, "JagVal %s(", stmt->as.fun_decl.name);
            for (size_t i = 0; i < stmt->as.fun_decl.param_count; i++) {
                if (i > 0) fprintf(ctx->f, ", ");
                fprintf(ctx->f, "JagVal %s", stmt->as.fun_decl.params[i].name);
            }
            fprintf(ctx->f, ") ");
            gen_block(ctx, stmt->as.fun_decl.body);
            break;

        case AST_IF:
            emit_indent(ctx);
            fprintf(ctx->f, "if ((");
            gen_expr(ctx, stmt->as.if_stmt.cond);
            fprintf(ctx->f, ").as.boolean) ");
            gen_block(ctx, stmt->as.if_stmt.then_branch);

            for (size_t i = 0; i < stmt->as.if_stmt.elif_count; i++) {
                emit_indent(ctx);
                fprintf(ctx->f, "else if ((");
                gen_expr(ctx, stmt->as.if_stmt.elif_conds[i]);
                fprintf(ctx->f, ").as.boolean) ");
                gen_block(ctx, stmt->as.if_stmt.elif_branches[i]);
            }

            if (stmt->as.if_stmt.else_branch) {
                emit_indent(ctx);
                fprintf(ctx->f, "else ");
                gen_block(ctx, stmt->as.if_stmt.else_branch);
            }
            break;

        case AST_LOOP:
            emit_indent(ctx);
            fprintf(ctx->f, "while ((");
            gen_expr(ctx, stmt->as.loop_stmt.cond);
            fprintf(ctx->f, ").as.boolean) ");
            gen_block(ctx, stmt->as.loop_stmt.body);
            break;

        case AST_DO_LOOP:
            emit_indent(ctx);
            fprintf(ctx->f, "do ");
            gen_block(ctx, stmt->as.do_loop_stmt.body);
            emit_indent(ctx);
            fprintf(ctx->f, "while ((");
            gen_expr(ctx, stmt->as.do_loop_stmt.cond);
            fprintf(ctx->f, ").as.boolean);\n");
            break;

        case AST_FOR_IN:
            emit_indent(ctx);
            fprintf(ctx->f, "{\n");
            ctx->indent++;
            emit_indent(ctx);
            fprintf(ctx->f, "JagVal _coll = ");
            gen_expr(ctx, stmt->as.for_in_stmt.collection);
            fprintf(ctx->f, ";\n");
            emit_indent(ctx);
            fprintf(ctx->f, "if (_coll.kind == JAG_VAL_ARRAY && _coll.as.array) {\n");
            ctx->indent++;
            emit_indent(ctx);
            fprintf(ctx->f, "for (size_t _i = 0; _i < _coll.as.array->count; _i++) {\n");
            ctx->indent++;
            emit_indent(ctx);
            fprintf(ctx->f, "JagVal %s = _coll.as.array->items[_i];\n", stmt->as.for_in_stmt.var_name);

            for (size_t i = 0; i < stmt->as.for_in_stmt.body->as.block.count; i++) {
                gen_stmt(ctx, stmt->as.for_in_stmt.body->as.block.stmts[i]);
            }
            ctx->indent--;
            emit_indent(ctx);
            fprintf(ctx->f, "}\n");
            ctx->indent--;
            emit_indent(ctx);
            fprintf(ctx->f, "}\n");
            ctx->indent--;
            emit_indent(ctx);
            fprintf(ctx->f, "}\n");
            break;

        case AST_RETURN:
            emit_indent(ctx);
            fprintf(ctx->f, "return ");
            if (stmt->as.return_stmt.expr) {
                gen_expr(ctx, stmt->as.return_stmt.expr);
            } else {
                fprintf(ctx->f, "jag_val_null()");
            }
            fprintf(ctx->f, ";\n");
            break;

        case AST_LIVE_DEG:
            emit_indent(ctx);
            fprintf(ctx->f, "jag_live_deg(\"%s\", ", stmt->as.live_deg.type_name);
            gen_expr(ctx, stmt->as.live_deg.expected);
            fprintf(ctx->f, ", ");
            gen_expr(ctx, stmt->as.live_deg.var_expr);
            fprintf(ctx->f, ");\n");
            break;

        case AST_EXPR_STMT:
            emit_indent(ctx);
            gen_expr(ctx, stmt->as.expr_stmt.expr);
            fprintf(ctx->f, ";\n");
            break;

        case AST_STRUCT_DECL:
            emit_indent(ctx);
            fprintf(ctx->f, "typedef struct %s %s;\n", stmt->as.struct_decl.name, stmt->as.struct_decl.name);
            break;

        case AST_CLASS_DECL:
            emit_indent(ctx);
            fprintf(ctx->f, "typedef struct %s %s;\n", stmt->as.class_decl.name, stmt->as.class_decl.name);
            break;

        case AST_ENUM_DECL:
            emit_indent(ctx);
            fprintf(ctx->f, "typedef enum { ");
            for (size_t i = 0; i < stmt->as.enum_decl.enum_count; i++) {
                if (i > 0) fprintf(ctx->f, ", ");
                fprintf(ctx->f, "%s_%s", stmt->as.enum_decl.name, stmt->as.enum_decl.enumerators[i]);
            }
            fprintf(ctx->f, " } %s;\n", stmt->as.enum_decl.name);
            break;

        case AST_BLOCK:
            gen_block(ctx, stmt);
            break;

        default:
            break;
    }
}

bool jag_codegen_generate_c(JagASTNode *ast, const char *output_c_filename, JagCodegenOptions *opts) {
    (void)opts;
    FILE *f = fopen(output_c_filename, "w");
    if (!f) return false;

    CodegenCtx ctx = { f, 0 };

    fprintf(f, "/* Generated by Jaguar Compiler 0.1.0 */\n");
    fprintf(f, "#include \"jag/runtime.h\"\n\n");

    if (ast && ast->kind == AST_PROGRAM) {
        for (size_t i = 0; i < ast->as.program.count; i++) {
            JagASTNode *stmt = ast->as.program.stmts[i];
            if (stmt->kind == AST_FUN_DECL) {
                fprintf(f, "JagVal %s(", stmt->as.fun_decl.name);
                for (size_t p = 0; p < stmt->as.fun_decl.param_count; p++) {
                    if (p > 0) fprintf(f, ", ");
                    fprintf(f, "JagVal %s", stmt->as.fun_decl.params[p].name);
                }
                fprintf(f, ");\n");
            }
        }
    }
    fprintf(f, "\n");

    if (ast && ast->kind == AST_PROGRAM) {
        for (size_t i = 0; i < ast->as.program.count; i++) {
            JagASTNode *stmt = ast->as.program.stmts[i];
            if (stmt->kind == AST_FUN_DECL || stmt->kind == AST_STRUCT_DECL ||
                stmt->kind == AST_CLASS_DECL || stmt->kind == AST_ENUM_DECL) {
                gen_stmt(&ctx, stmt);
            }
        }

        fprintf(f, "\nint main(int argc, char **argv) {\n");
        fprintf(f, "    (void)argc; (void)argv;\n");
        ctx.indent = 1;

        for (size_t i = 0; i < ast->as.program.count; i++) {
            JagASTNode *stmt = ast->as.program.stmts[i];
            if (stmt->kind != AST_FUN_DECL && stmt->kind != AST_STRUCT_DECL &&
                stmt->kind != AST_CLASS_DECL && stmt->kind != AST_ENUM_DECL) {
                gen_stmt(&ctx, stmt);
            }
        }

        fprintf(f, "    return 0;\n");
        fprintf(f, "}\n");
    }

    fclose(f);
    return true;
}
