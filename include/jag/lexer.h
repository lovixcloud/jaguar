#ifndef JAG_LEXER_H
#define JAG_LEXER_H

#include "jag/common.h"

typedef enum {
    TOKEN_EOF = 0,
    TOKEN_ERROR,

    // Keywords
    TOKEN_VAR,
    TOKEN_FIXED,
    TOKEN_FUN,
    TOKEN_ASYNC,
    TOKEN_IF,
    TOKEN_ELIF,
    TOKEN_ELSE,
    TOKEN_LOOP,
    TOKEN_DO,
    TOKEN_FOR,
    TOKEN_IN,
    TOKEN_STRUCT,
    TOKEN_CLASS,
    TOKEN_PUBLIC,
    TOKEN_PRIVATE,
    TOKEN_ENUM,
    TOKEN_IMPORT,
    TOKEN_EXPORT,
    TOKEN_LIVE,
    TOKEN_RETURN,
    TOKEN_DATA,

    // Type names
    TOKEN_TYPE_NUM,
    TOKEN_TYPE_DECIMAL,
    TOKEN_TYPE_BOOL,
    TOKEN_TYPE_STRING,
    TOKEN_TYPE_VOID,
    TOKEN_TYPE_MIXED,
    TOKEN_TYPE_VECTOR,
    TOKEN_TYPE_MATRIX,

    // Literals
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_INT_LITERAL,
    TOKEN_FLOAT_LITERAL,
    TOKEN_STRING_LITERAL,
    TOKEN_IDENTIFIER,

    // Operators
    TOKEN_PLUS,          // +
    TOKEN_MINUS,         // -
    TOKEN_STAR,          // *
    TOKEN_SLASH,         // /
    TOKEN_PERCENT,       // %
    TOKEN_POWER,         // **
    TOKEN_PLUS_EQ,       // +=
    TOKEN_MINUS_EQ,      // -=
    TOKEN_STAR_EQ,       // *=
    TOKEN_SLASH_EQ,      // /=
    TOKEN_PERCENT_EQ,    // %=

    TOKEN_EQ,            // =
    TOKEN_EQ_EQ,         // ==
    TOKEN_BANG,          // !
    TOKEN_BANG_EQ,       // !=
    TOKEN_LESS,          // <
    TOKEN_LESS_EQ,       // <=
    TOKEN_GREATER,       // >
    TOKEN_GREATER_EQ,    // >=

    TOKEN_AND,           // &&
    TOKEN_OR,            // ||
    TOKEN_BIT_AND,       // &
    TOKEN_BIT_OR,        // |
    TOKEN_BIT_XOR,       // ^
    TOKEN_BIT_NOT,       // ~
    TOKEN_SHL,           // <<
    TOKEN_SHR,           // >>

    TOKEN_QUESTION,      // ?
    TOKEN_COLON,         // :
    TOKEN_DOT,           // .
    TOKEN_COMMA,         // ,
    TOKEN_SEMICOLON,     // ;

    // Delimiters
    TOKEN_LPAREN,        // (
    TOKEN_RPAREN,        // )
    TOKEN_LBRACE,        // {
    TOKEN_RBRACE,        // }
    TOKEN_LBRACKET,      // [
    TOKEN_RBRACKET       // ]
} JagTokenType;

typedef struct {
    JagTokenType type;
    const char *start;
    size_t length;
    int line;
    int column;
    const char *file;
    union {
        int64_t int_val;
        double float_val;
        char *string_val;
    } as;
} JagToken;

typedef struct {
    const char *source;
    size_t source_len;
    size_t cursor;
    int line;
    int column;
    const char *filename;
} JagLexer;

void jag_lexer_init(JagLexer *lexer, const char *source, const char *filename);
JagToken jag_lexer_next_token(JagLexer *lexer);
const char *jag_token_type_name(JagTokenType type);
void jag_token_free(JagToken *token);

#endif // JAG_LEXER_H
