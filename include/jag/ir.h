#ifndef JAG_IR_H
#define JAG_IR_H

#include "jag/ast.h"
#include "jag/common.h"

typedef enum {
    IR_NOP,
    IR_CONST_INT,
    IR_CONST_FLOAT,
    IR_CONST_STR,
    IR_CONST_BOOL,
    IR_LOAD,
    IR_STORE,
    IR_ADD,
    IR_SUB,
    IR_MUL,
    IR_DIV,
    IR_MOD,
    IR_POWER,
    IR_CMP_EQ,
    IR_CMP_NE,
    IR_CMP_LT,
    IR_CMP_LE,
    IR_CMP_GT,
    IR_CMP_GE,
    IR_LABEL,
    IR_JUMP,
    IR_JUMP_IF_FALSE,
    IR_CALL,
    IR_RETURN,
    IR_ARRAY_GET,
    IR_ARRAY_SET,
    IR_LIVE_ON,
    IR_LIVE_LOG,
    IR_LIVE_DEG
} JagIROpcode;

typedef struct JagIRInstruction JagIRInstruction;

struct JagIRInstruction {
    JagIROpcode op;
    int dest_reg;
    int src1_reg;
    int src2_reg;
    union {
        int64_t int_val;
        double float_val;
        char *str_val;
        bool bool_val;
        char *label_name;
        char *var_name;
    } imm;
    JagIRInstruction *next;
};

typedef struct {
    char *name;
    JagIRInstruction *head;
    JagIRInstruction *tail;
} JagIRFunction;

typedef struct {
    JagIRFunction **functions;
    size_t function_count;
    JagIRInstruction *global_init;
} JagIRModule;

JagIRModule *jag_ir_module_create(void);
void jag_ir_module_free(JagIRModule *module);
JagIRModule *jag_ir_lower_ast(JagASTNode *ast);

#endif // JAG_IR_H
