#include "jag/optimizer.h"
#include <stdlib.h>

static void fold_constants(JagIRInstruction *head) {
    JagIRInstruction *curr = head;
    while (curr) {
        if ((curr->op == IR_ADD || curr->op == IR_SUB || curr->op == IR_MUL || curr->op == IR_DIV) &&
            curr->src1_reg > 0 && curr->src2_reg > 0) {

            JagIRInstruction *p1 = head;
            int64_t v1 = 0, v2 = 0;
            bool found1 = false, found2 = false;

            while (p1 && p1 != curr) {
                if (p1->dest_reg == curr->src1_reg && p1->op == IR_CONST_INT) {
                    v1 = p1->imm.int_val;
                    found1 = true;
                }
                if (p1->dest_reg == curr->src2_reg && p1->op == IR_CONST_INT) {
                    v2 = p1->imm.int_val;
                    found2 = true;
                }
                p1 = p1->next;
            }

            if (found1 && found2) {
                curr->op = IR_CONST_INT;
                if (curr->op == IR_ADD) curr->imm.int_val = v1 + v2;
                else if (curr->op == IR_SUB) curr->imm.int_val = v1 - v2;
                else if (curr->op == IR_MUL) curr->imm.int_val = v1 * v2;
                else if (curr->op == IR_DIV && v2 != 0) curr->imm.int_val = v1 / v2;
                curr->src1_reg = 0;
                curr->src2_reg = 0;
            }
        }
        curr = curr->next;
    }
}

void jag_optimizer_optimize_module(JagIRModule *module) {
    if (!module) return;
    if (module->global_init) {
        fold_constants(module->global_init);
    }
    for (size_t i = 0; i < module->function_count; i++) {
        if (module->functions[i]->head) {
            fold_constants(module->functions[i]->head);
        }
    }
}
