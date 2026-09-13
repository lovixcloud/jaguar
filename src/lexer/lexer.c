#include "jag/lexer.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

void jag_lexer_init(JagLexer *lexer, const char *source, const char *filename) {
    lexer->source = source;
    lexer->source_len = strlen(source);
    lexer->cursor = 0;
    lexer->line = 1;
    lexer->column = 1;
    lexer->filename = filename ? filename : "<stdin>";
}

static char peek(JagLexer *lexer) {
    if (lexer->cursor >= lexer->source_len) return '\0';
    return lexer->source[lexer->cursor];
}

static char peek_next(JagLexer *lexer) {
    if (lexer->cursor + 1 >= lexer->source_len) return '\0';
    return lexer->source[lexer->cursor + 1];
}

static char advance(JagLexer *lexer) {
    char c = peek(lexer);
    if (c != '\0') {
        lexer->cursor++;
        if (c == '\n') {
            lexer->line++;
            lexer->column = 1;
        } else {
            lexer->column++;
        }
    }
    return c;
}

static void skip_whitespace_and_comments(JagLexer *lexer) {
    for (;;) {
        char c = peek(lexer);
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance(lexer);
        } else if (c == '/' && peek_next(lexer) == '/') {
            advance(lexer);
            advance(lexer);
            while (peek(lexer) != '\n' && peek(lexer) != '\0') {
                advance(lexer);
            }
        } else if (c == '/' && peek_next(lexer) == '*') {
            advance(lexer);
            advance(lexer);
            while (peek(lexer) != '\0') {
                if (peek(lexer) == '*' && peek_next(lexer) == '/') {
                    advance(lexer);
                    advance(lexer);
                    break;
                }
                advance(lexer);
            }
        } else {
            break;
        }
    }
}

static JagToken make_token(JagLexer *lexer, JagTokenType type, const char *start, int line, int col) {
    JagToken token;
    token.type = type;
    token.start = start;
    token.length = (size_t)(&lexer->source[lexer->cursor] - start);
    token.line = line;
    token.column = col;
    token.file = lexer->filename;
    token.as.string_val = NULL;
    return token;
}

static JagToken make_error_token(JagLexer *lexer, const char *msg, int line, int col) {
    JagToken token;
    token.type = TOKEN_ERROR;
    token.start = msg;
    token.length = strlen(msg);
    token.line = line;
    token.column = col;
    token.file = lexer->filename;
    token.as.string_val = NULL;
    return token;
}

static JagTokenType check_keyword(const char *str, size_t len) {
    switch (len) {
        case 2:
            if (memcmp(str, "if", 2) == 0) return TOKEN_IF;
            if (memcmp(str, "do", 2) == 0) return TOKEN_DO;
            if (memcmp(str, "in", 2) == 0) return TOKEN_IN;
            break;
        case 3:
            if (memcmp(str, "var", 3) == 0) return TOKEN_VAR;
            if (memcmp(str, "fun", 3) == 0) return TOKEN_FUN;
            if (memcmp(str, "num", 3) == 0) return TOKEN_TYPE_NUM;
            break;
        case 4:
            if (memcmp(str, "elif", 4) == 0) return TOKEN_ELIF;
            if (memcmp(str, "else", 4) == 0) return TOKEN_ELSE;
            if (memcmp(str, "loop", 4) == 0) return TOKEN_LOOP;
            if (memcmp(str, "enum", 4) == 0) return TOKEN_ENUM;
            if (memcmp(str, "bool", 4) == 0) return TOKEN_TYPE_BOOL;
            if (memcmp(str, "void", 4) == 0) return TOKEN_TYPE_VOID;
            if (memcmp(str, "true", 4) == 0) return TOKEN_TRUE;
            if (memcmp(str, "data", 4) == 0) return TOKEN_DATA;
            if (memcmp(str, "live", 4) == 0) return TOKEN_LIVE;
            break;
        case 5:
            if (memcmp(str, "fixed", 5) == 0) return TOKEN_FIXED;
            if (memcmp(str, "async", 5) == 0) return TOKEN_ASYNC;
            if (memcmp(str, "class", 5) == 0) return TOKEN_CLASS;
            if (memcmp(str, "false", 5) == 0) return TOKEN_FALSE;
            if (memcmp(str, "mixed", 5) == 0) return TOKEN_TYPE_MIXED;
            break;
        case 6:
            if (memcmp(str, "struct", 6) == 0) return TOKEN_STRUCT;
            if (memcmp(str, "public", 6) == 0) return TOKEN_PUBLIC;
            if (memcmp(str, "import", 6) == 0) return TOKEN_IMPORT;
            if (memcmp(str, "export", 6) == 0) return TOKEN_EXPORT;
            if (memcmp(str, "return", 6) == 0) return TOKEN_RETURN;
            if (memcmp(str, "string", 6) == 0) return TOKEN_TYPE_STRING;
            if (memcmp(str, "vector", 6) == 0) return TOKEN_TYPE_VECTOR;
            if (memcmp(str, "matrix", 6) == 0) return TOKEN_TYPE_MATRIX;
            break;
        case 7:
            if (memcmp(str, "private", 7) == 0) return TOKEN_PRIVATE;
            if (memcmp(str, "decimal", 7) == 0) return TOKEN_TYPE_DECIMAL;
            break;
    }
    return TOKEN_IDENTIFIER;
}

JagToken jag_lexer_next_token(JagLexer *lexer) {
    skip_whitespace_and_comments(lexer);

    const char *start = &lexer->source[lexer->cursor];
    int start_line = lexer->line;
    int start_col = lexer->column;

    char c = advance(lexer);
    if (c == '\0') {
        return make_token(lexer, TOKEN_EOF, start, start_line, start_col);
    }

    if (isalpha(c) || c == '_') {
        while (isalnum(peek(lexer)) || peek(lexer) == '_') {
            advance(lexer);
        }
        size_t len = (size_t)(&lexer->source[lexer->cursor] - start);
        JagTokenType type = check_keyword(start, len);
        return make_token(lexer, type, start, start_line, start_col);
    }

    if (isdigit(c)) {
        bool is_float = false;
        while (isdigit(peek(lexer))) {
            advance(lexer);
        }
        if (peek(lexer) == '.' && isdigit(peek_next(lexer))) {
            is_float = true;
            advance(lexer);
            while (isdigit(peek(lexer))) {
                advance(lexer);
            }
        }
        if (peek(lexer) == 'e' || peek(lexer) == 'E') {
            is_float = true;
            advance(lexer);
            if (peek(lexer) == '+' || peek(lexer) == '-') {
                advance(lexer);
            }
            while (isdigit(peek(lexer))) {
                advance(lexer);
            }
        }
        JagToken token = make_token(lexer, is_float ? TOKEN_FLOAT_LITERAL : TOKEN_INT_LITERAL, start, start_line, start_col);
        char buf[128];
        size_t len = token.length < sizeof(buf) - 1 ? token.length : sizeof(buf) - 1;
        memcpy(buf, start, len);
        buf[len] = '\0';
        if (is_float) {
            token.as.float_val = strtod(buf, NULL);
        } else {
            token.as.int_val = strtoll(buf, NULL, 10);
        }
        return token;
    }

    if (c == '"') {
        size_t cap = 64;
        size_t len = 0;
        char *val = malloc(cap);
        while (peek(lexer) != '"' && peek(lexer) != '\0') {
            char ch = advance(lexer);
            if (ch == '\\') {
                char next_ch = advance(lexer);
                switch (next_ch) {
                    case 'n': ch = '\n'; break;
                    case 't': ch = '\t'; break;
                    case 'r': ch = '\r'; break;
                    case '"': ch = '"'; break;
                    case '\\': ch = '\\'; break;
                    default: ch = next_ch; break;
                }
            }
            if (len + 1 >= cap) {
                cap *= 2;
                val = realloc(val, cap);
            }
            val[len++] = ch;
        }
        val[len] = '\0';
        if (peek(lexer) == '"') {
            advance(lexer);
            JagToken token = make_token(lexer, TOKEN_STRING_LITERAL, start, start_line, start_col);
            token.as.string_val = val;
            return token;
        } else {
            free(val);
            return make_error_token(lexer, "Unterminated string literal", start_line, start_col);
        }
    }

    switch (c) {
        case '+':
            if (peek(lexer) == '=') { advance(lexer); return make_token(lexer, TOKEN_PLUS_EQ, start, start_line, start_col); }
            return make_token(lexer, TOKEN_PLUS, start, start_line, start_col);
        case '-':
            if (peek(lexer) == '=') { advance(lexer); return make_token(lexer, TOKEN_MINUS_EQ, start, start_line, start_col); }
            return make_token(lexer, TOKEN_MINUS, start, start_line, start_col);
        case '*':
            if (peek(lexer) == '*') { advance(lexer); return make_token(lexer, TOKEN_POWER, start, start_line, start_col); }
            if (peek(lexer) == '=') { advance(lexer); return make_token(lexer, TOKEN_STAR_EQ, start, start_line, start_col); }
            return make_token(lexer, TOKEN_STAR, start, start_line, start_col);
        case '/':
            if (peek(lexer) == '=') { advance(lexer); return make_token(lexer, TOKEN_SLASH_EQ, start, start_line, start_col); }
            return make_token(lexer, TOKEN_SLASH, start, start_line, start_col);
        case '%':
            if (peek(lexer) == '=') { advance(lexer); return make_token(lexer, TOKEN_PERCENT_EQ, start, start_line, start_col); }
            return make_token(lexer, TOKEN_PERCENT, start, start_line, start_col);
        case '=':
            if (peek(lexer) == '=') { advance(lexer); return make_token(lexer, TOKEN_EQ_EQ, start, start_line, start_col); }
            return make_token(lexer, TOKEN_EQ, start, start_line, start_col);
        case '!':
            if (peek(lexer) == '=') { advance(lexer); return make_token(lexer, TOKEN_BANG_EQ, start, start_line, start_col); }
            return make_token(lexer, TOKEN_BANG, start, start_line, start_col);
        case '<':
            if (peek(lexer) == '=') { advance(lexer); return make_token(lexer, TOKEN_LESS_EQ, start, start_line, start_col); }
            if (peek(lexer) == '<') { advance(lexer); return make_token(lexer, TOKEN_SHL, start, start_line, start_col); }
            return make_token(lexer, TOKEN_LESS, start, start_line, start_col);
        case '>':
            if (peek(lexer) == '=') { advance(lexer); return make_token(lexer, TOKEN_GREATER_EQ, start, start_line, start_col); }
            if (peek(lexer) == '>') { advance(lexer); return make_token(lexer, TOKEN_SHR, start, start_line, start_col); }
            return make_token(lexer, TOKEN_GREATER, start, start_line, start_col);
        case '&':
            if (peek(lexer) == '&') { advance(lexer); return make_token(lexer, TOKEN_AND, start, start_line, start_col); }
            return make_token(lexer, TOKEN_BIT_AND, start, start_line, start_col);
        case '|':
            if (peek(lexer) == '|') { advance(lexer); return make_token(lexer, TOKEN_OR, start, start_line, start_col); }
            return make_token(lexer, TOKEN_BIT_OR, start, start_line, start_col);
        case '^': return make_token(lexer, TOKEN_BIT_XOR, start, start_line, start_col);
        case '~': return make_token(lexer, TOKEN_BIT_NOT, start, start_line, start_col);
        case '?': return make_token(lexer, TOKEN_QUESTION, start, start_line, start_col);
        case ':': return make_token(lexer, TOKEN_COLON, start, start_line, start_col);
        case '.': return make_token(lexer, TOKEN_DOT, start, start_line, start_col);
        case ',': return make_token(lexer, TOKEN_COMMA, start, start_line, start_col);
        case ';': return make_token(lexer, TOKEN_SEMICOLON, start, start_line, start_col);
        case '(': return make_token(lexer, TOKEN_LPAREN, start, start_line, start_col);
        case ')': return make_token(lexer, TOKEN_RPAREN, start, start_line, start_col);
        case '{': return make_token(lexer, TOKEN_LBRACE, start, start_line, start_col);
        case '}': return make_token(lexer, TOKEN_RBRACE, start, start_line, start_col);
        case '[': return make_token(lexer, TOKEN_LBRACKET, start, start_line, start_col);
        case ']': return make_token(lexer, TOKEN_RBRACKET, start, start_line, start_col);
    }

    return make_error_token(lexer, "Unexpected character", start_line, start_col);
}

const char *jag_token_type_name(JagTokenType type) {
    switch (type) {
        case TOKEN_EOF: return "EOF";
        case TOKEN_ERROR: return "ERROR";
        case TOKEN_VAR: return "var";
        case TOKEN_FIXED: return "fixed";
        case TOKEN_FUN: return "fun";
        case TOKEN_ASYNC: return "async";
        case TOKEN_IF: return "if";
        case TOKEN_ELIF: return "elif";
        case TOKEN_ELSE: return "else";
        case TOKEN_LOOP: return "loop";
        case TOKEN_DO: return "do";
        case TOKEN_FOR: return "for";
        case TOKEN_IN: return "in";
        case TOKEN_STRUCT: return "struct";
        case TOKEN_CLASS: return "class";
        case TOKEN_PUBLIC: return "public";
        case TOKEN_PRIVATE: return "private";
        case TOKEN_ENUM: return "enum";
        case TOKEN_IMPORT: return "import";
        case TOKEN_EXPORT: return "export";
        case TOKEN_LIVE: return "live";
        case TOKEN_RETURN: return "return";
        case TOKEN_DATA: return "data";
        case TOKEN_TYPE_NUM: return "num";
        case TOKEN_TYPE_DECIMAL: return "decimal";
        case TOKEN_TYPE_BOOL: return "bool";
        case TOKEN_TYPE_STRING: return "string";
        case TOKEN_TYPE_VOID: return "void";
        case TOKEN_TYPE_MIXED: return "mixed";
        case TOKEN_TYPE_VECTOR: return "vector";
        case TOKEN_TYPE_MATRIX: return "matrix";
        case TOKEN_TRUE: return "true";
        case TOKEN_FALSE: return "false";
        case TOKEN_INT_LITERAL: return "INT_LITERAL";
        case TOKEN_FLOAT_LITERAL: return "FLOAT_LITERAL";
        case TOKEN_STRING_LITERAL: return "STRING_LITERAL";
        case TOKEN_IDENTIFIER: return "IDENTIFIER";
        default: return "OPERATOR_DELIMITER";
    }
}

void jag_token_free(JagToken *token) {
    if (token->type == TOKEN_STRING_LITERAL && token->as.string_val) {
        free(token->as.string_val);
        token->as.string_val = NULL;
    }
}
