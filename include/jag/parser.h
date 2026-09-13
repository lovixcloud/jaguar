#ifndef JAG_PARSER_H
#define JAG_PARSER_H

#include "jag/ast.h"
#include "jag/lexer.h"

typedef struct {
    JagLexer lexer;
    JagToken current;
    JagToken previous;
    bool had_error;
    bool panic_mode;
    char error_msg[256];
    JagSourceLoc error_loc;
} JagParser;

void jag_parser_init(JagParser *parser, const char *source, const char *filename);
JagASTNode *jag_parse_program(JagParser *parser);

#endif // JAG_PARSER_H
