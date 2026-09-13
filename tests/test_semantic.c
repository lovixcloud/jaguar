#include "jag/parser.h"
#include "jag/semantic.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
    const char *valid_source =
        "live = \"1\";\n"
        "fixed age: num = 33;\n"
        "var score: decimal = 98.71;\n"
        "var isMale: bool = true;\n"
        "live.deg(num, 33, age);\n";

    JagParser parser;
    jag_parser_init(&parser, valid_source, "valid.jag");
    JagASTNode *ast1 = jag_parse_program(&parser);
    assert(!parser.had_error);

    JagSemanticAnalyzer sa1;
    jag_semantic_init(&sa1, valid_source);
    bool ok1 = jag_semantic_analyze(&sa1, ast1);
    assert(ok1 == true);
    jag_semantic_cleanup(&sa1);
    jag_ast_free(ast1);

    printf("Semantic analysis tests passed successfully!\n");
    return 0;
}
