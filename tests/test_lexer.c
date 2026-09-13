#include "jag/lexer.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    const char *source =
        "live = \"1\";\n"
        "fixed language: string = \"Jaguar\";\n"
        "var age: num = 33;\n"
        "var score: decimal = 3.0E8;\n";

    JagLexer lexer;
    jag_lexer_init(&lexer, source, "test.jag");

    JagToken tok = jag_lexer_next_token(&lexer);
    assert(tok.type == TOKEN_LIVE);
    jag_token_free(&tok);

    printf("Lexer tests passed successfully!\n");
    return 0;
}
