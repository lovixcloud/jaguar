#include "jag/ir.h"
#include "jag/optimizer.h"
#include "jag/parser.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
    const char *source =
        "var a: num = 10 + 20;\n"
        "live.deg(num, 30, a);\n";

    JagParser parser;
    jag_parser_init(&parser, source, "test_ir.jag");
    JagASTNode *ast = jag_parse_program(&parser);
    assert(!parser.had_error);

    JagIRModule *ir = jag_ir_lower_ast(ast);
    assert(ir != NULL);

    jag_optimizer_optimize_module(ir);

    jag_ir_module_free(ir);
    jag_ast_free(ast);

    printf("IR and optimizer tests passed successfully!\n");
    return 0;
}
