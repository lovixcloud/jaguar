#include "jag/ir.h"
#include <stdlib.h>
#include <string.h>

JagIRModule *jag_ir_module_create(void) {
    return calloc(1, sizeof(JagIRModule));
}

static void free_instructions(JagIRInstruction *head) {
    JagIRInstruction *curr = head;
    while (curr) {
        JagIRInstruction *next = curr->next;
        if (curr->op == IR_CONST_STR || curr->op == IR_LOAD || curr->op == IR_STORE ||
            curr->op == IR_LABEL || curr->op == IR_JUMP || curr->op == IR_JUMP_IF_FALSE ||
            curr->op == IR_CALL) {
            free(curr->imm.str_val);
        }
        free(curr);
        curr = next;
    }
}

void jag_ir_module_free(JagIRModule *module) {
    if (!module) return;
    for (size_t i = 0; i < module->function_count; i++) {
        free(module->functions[i]->name);
        free_instructions(module->functions[i]->head);
        free(module->functions[i]);
    }
    free(module->functions);
    if (module->global_init) {
        free_instructions(module->global_init);
    }
    free(module);
}

static JagIRInstruction *emit_inst(JagIRInstruction **tail, JagIROpcode op, int dest, int src1, int src2) {
    JagIRInstruction *inst = calloc(1, sizeof(JagIRInstruction));
    inst->op = op;
    inst->dest_reg = dest;
    inst->src1_reg = src1;
    inst->src2_reg = src2;

    if (*tail) {
        (*tail)->next = inst;
    }
    *tail = inst;
    return inst;
}

static int g_reg_counter = 1;

static int lower_expr(JagASTNode *node, JagIRInstruction **head, JagIRInstruction **tail) {
    if (!node) return 0;

    switch (node->kind) {
        case AST_LITERAL: {
            int reg = g_reg_counter++;
            if (node->as.literal.lit_kind == LITERAL_INT) {
                JagIRInstruction *inst = emit_inst(tail, IR_CONST_INT, reg, 0, 0);
                inst->imm.int_val = node->as.literal.val.int_val;
            } else if (node->as.literal.lit_kind == LITERAL_FLOAT) {
                JagIRInstruction *inst = emit_inst(tail, IR_CONST_FLOAT, reg, 0, 0);
                inst->imm.float_val = node->as.literal.val.float_val;
            } else if (node->as.literal.lit_kind == LITERAL_STRING) {
                JagIRInstruction *inst = emit_inst(tail, IR_CONST_STR, reg, 0, 0);
                inst->imm.str_val = jag_strdup(node->as.literal.val.string_val ? node->as.literal.val.string_val : "");
            } else if (node->as.literal.lit_kind == LITERAL_BOOL) {
                JagIRInstruction *inst = emit_inst(tail, IR_CONST_BOOL, reg, 0, 0);
                inst->imm.bool_val = node->as.literal.val.bool_val;
            }
            if (!*head) *head = *tail;
            return reg;
        }

        case AST_IDENTIFIER: {
            int reg = g_reg_counter++;
            JagIRInstruction *inst = emit_inst(tail, IR_LOAD, reg, 0, 0);
            inst->imm.var_name = jag_strdup(node->as.identifier.name);
            if (!*head) *head = *tail;
            return reg;
        }

        case AST_BINARY: {
            int r1 = lower_expr(node->as.binary_expr.left, head, tail);
            int r2 = lower_expr(node->as.binary_expr.right, head, tail);
            int reg = g_reg_counter++;
            JagIROpcode op = IR_ADD;
            switch (node->as.binary_expr.op) {
                case TOKEN_PLUS: op = IR_ADD; break;
                case TOKEN_MINUS: op = IR_SUB; break;
                case TOKEN_STAR: op = IR_MUL; break;
                case TOKEN_SLASH: op = IR_DIV; break;
                case TOKEN_PERCENT: op = IR_MOD; break;
                case TOKEN_POWER: op = IR_POWER; break;
                case TOKEN_EQ_EQ: op = IR_CMP_EQ; break;
                case TOKEN_BANG_EQ: op = IR_CMP_NE; break;
                case TOKEN_LESS: op = IR_CMP_LT; break;
                case TOKEN_LESS_EQ: op = IR_CMP_LE; break;
                case TOKEN_GREATER: op = IR_CMP_GT; break;
                case TOKEN_GREATER_EQ: op = IR_CMP_GE; break;
                default: op = IR_ADD; break;
            }
            emit_inst(tail, op, reg, r1, r2);
            if (!*head) *head = *tail;
            return reg;
        }

        default:
            return 0;
    }
}

static void lower_stmt(JagASTNode *stmt, JagIRInstruction **head, JagIRInstruction **tail) {
    if (!stmt) return;

    switch (stmt->kind) {
        case AST_VAR_DECL: {
            if (stmt->as.var_decl.init) {
                int reg = lower_expr(stmt->as.var_decl.init, head, tail);
                JagIRInstruction *inst = emit_inst(tail, IR_STORE, 0, reg, 0);
                inst->imm.var_name = jag_strdup(stmt->as.var_decl.name);
                if (!*head) *head = *tail;
            }
            break;
        }

        case AST_EXPR_STMT:
            lower_expr(stmt->as.expr_stmt.expr, head, tail);
            break;

        case AST_LIVE_DEG: {
            int r_exp = lower_expr(stmt->as.live_deg.expected, head, tail);
            int r_var = lower_expr(stmt->as.live_deg.var_expr, head, tail);
            JagIRInstruction *inst = emit_inst(tail, IR_LIVE_DEG, 0, r_exp, r_var);
            inst->imm.str_val = jag_strdup(stmt->as.live_deg.type_name);
            if (!*head) *head = *tail;
            break;
        }

        default:
            break;
    }
}

JagIRModule *jag_ir_lower_ast(JagASTNode *ast) {
    JagIRModule *module = jag_ir_module_create();
    if (!ast || ast->kind != AST_PROGRAM) return module;

    JagIRInstruction *head = NULL;
    JagIRInstruction *tail = NULL;

    for (size_t i = 0; i < ast->as.program.count; i++) {
        lower_stmt(ast->as.program.stmts[i], &head, &tail);
    }

    module->global_init = head;
    return module;
}
