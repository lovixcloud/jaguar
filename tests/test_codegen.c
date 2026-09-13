#include "jag/codegen.h"
#include "jag/parser.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
    const char *source =
        "var name: string = \"Jaguar\";\n"
        "live.on(name);\n";

    JagParser parser;
    jag_parser_init(&parser, source, "test_codegen.jag");
    JagASTNode *ast = jag_parse_program(&parser);
    assert(!parser.had_error);

    JagCodegenOptions opts = { 0 };
    opts.runtime_header_dir = "include";
    opts.runtime_lib_dir = "build";

    bool gen_ok = jag_codegen_generate_c(ast, "/tmp/test_codegen.c", &opts);
    assert(gen_ok == true);

    jag_ast_free(ast);
    printf("Codegen tests passed successfully!\n");
    return 0;
}
