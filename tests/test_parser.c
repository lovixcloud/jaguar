#include "jag/parser.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    const char *source =
        "live = \"1\";\n"
        "fixed language: string = \"Jaguar\";\n"
        "var age: num = 33;\n"
        "fun add(a: num, b: num): num {\n"
        "    return a + b;\n"
        "}\n";

    JagParser parser;
    jag_parser_init(&parser, source, "test_parser.jag");
    JagASTNode *program = jag_parse_program(&parser);

    assert(parser.had_error == false);
    assert(program != NULL);

    jag_ast_free(program);
    printf("Parser tests passed successfully!\n");
    return 0;
}
