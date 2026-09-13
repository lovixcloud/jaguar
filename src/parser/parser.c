#include "jag/parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void advance_parser(JagParser *parser) {
    parser->previous = parser->current;
    for (;;) {
        parser->current = jag_lexer_next_token(&parser->lexer);
        if (parser->current.type != TOKEN_ERROR) break;

        parser->had_error = true;
        snprintf(parser->error_msg, sizeof(parser->error_msg), "Lexical error: %.*s",
                 (int)parser->current.length, parser->current.start);
        parser->error_loc.file = parser->current.file;
        parser->error_loc.line = parser->current.line;
        parser->error_loc.column = parser->current.column;
    }
}

static bool check(JagParser *parser, JagTokenType type) {
    return parser->current.type == type;
}

static bool match(JagParser *parser, JagTokenType type) {
    if (!check(parser, type)) return false;
    advance_parser(parser);
    return true;
}

static void error_at(JagParser *parser, JagToken *token, const char *message) {
    if (parser->panic_mode) return;
    parser->panic_mode = true;
    parser->had_error = true;
    parser->error_loc.file = token->file;
    parser->error_loc.line = token->line;
    parser->error_loc.column = token->column;

    if (token->type == TOKEN_EOF) {
        snprintf(parser->error_msg, sizeof(parser->error_msg), "At end of file: %s", message);
    } else {
        snprintf(parser->error_msg, sizeof(parser->error_msg), "At '%.*s': %s",
                 (int)token->length, token->start, message);
    }
}

static void consume(JagParser *parser, JagTokenType type, const char *message) {
    if (parser->current.type == type) {
        advance_parser(parser);
        return;
    }
    error_at(parser, &parser->current, message);
}

static JagSourceLoc get_loc(JagParser *parser) {
    JagSourceLoc loc;
    loc.file = parser->current.file;
    loc.line = parser->current.line;
    loc.column = parser->current.column;
    return loc;
}

static char *copy_string(const char *start, size_t length) {
    char *str = malloc(length + 1);
    if (!str) return NULL;
    memcpy(str, start, length);
    str[length] = '\0';
    return str;
}

static JagASTNode *parse_declaration(JagParser *parser);
static JagASTNode *parse_statement(JagParser *parser);
static JagASTNode *parse_expression(JagParser *parser);
static JagASTNode *parse_block(JagParser *parser);

static JagTypeSpec parse_type_spec(JagParser *parser) {
    JagTypeSpec spec;
    spec.name = NULL;
    spec.is_array = false;

    if (check(parser, TOKEN_TYPE_NUM) || check(parser, TOKEN_TYPE_DECIMAL) ||
        check(parser, TOKEN_TYPE_BOOL) || check(parser, TOKEN_TYPE_STRING) ||
        check(parser, TOKEN_TYPE_VOID) || check(parser, TOKEN_TYPE_MIXED) ||
        check(parser, TOKEN_TYPE_VECTOR) || check(parser, TOKEN_TYPE_MATRIX) ||
        check(parser, TOKEN_IDENTIFIER)) {

        spec.name = copy_string(parser->current.start, parser->current.length);
        advance_parser(parser);
    } else {
        error_at(parser, &parser->current, "Expected type name");
        spec.name = jag_strdup("void");
    }

    if (match(parser, TOKEN_LBRACKET)) {
        consume(parser, TOKEN_RBRACKET, "Expected ']' after '[' in array type");
        spec.is_array = true;
    }

    return spec;
}

static JagASTNode *parse_primary(JagParser *parser) {
    JagSourceLoc loc = get_loc(parser);

    if (match(parser, TOKEN_INT_LITERAL)) {
        JagASTNode *node = jag_ast_create_node(AST_LITERAL, loc);
        node->as.literal.lit_kind = LITERAL_INT;
        node->as.literal.val.int_val = parser->previous.as.int_val;
        return node;
    }

    if (match(parser, TOKEN_FLOAT_LITERAL)) {
        JagASTNode *node = jag_ast_create_node(AST_LITERAL, loc);
        node->as.literal.lit_kind = LITERAL_FLOAT;
        node->as.literal.val.float_val = parser->previous.as.float_val;
        return node;
    }

    if (match(parser, TOKEN_STRING_LITERAL)) {
        JagASTNode *node = jag_ast_create_node(AST_LITERAL, loc);
        node->as.literal.lit_kind = LITERAL_STRING;
        node->as.literal.val.string_val = jag_strdup(parser->previous.as.string_val ? parser->previous.as.string_val : "");
        return node;
    }

    if (match(parser, TOKEN_TRUE)) {
        JagASTNode *node = jag_ast_create_node(AST_LITERAL, loc);
        node->as.literal.lit_kind = LITERAL_BOOL;
        node->as.literal.val.bool_val = true;
        return node;
    }

    if (match(parser, TOKEN_FALSE)) {
        JagASTNode *node = jag_ast_create_node(AST_LITERAL, loc);
        node->as.literal.lit_kind = LITERAL_BOOL;
        node->as.literal.val.bool_val = false;
        return node;
    }

    if (match(parser, TOKEN_IDENTIFIER) || match(parser, TOKEN_LIVE)) {
        JagASTNode *node = jag_ast_create_node(AST_IDENTIFIER, loc);
        node->as.identifier.name = copy_string(parser->previous.start, parser->previous.length);
        return node;
    }

    if (match(parser, TOKEN_LPAREN)) {
        JagASTNode *expr = parse_expression(parser);
        consume(parser, TOKEN_RPAREN, "Expected ')' after expression");
        return expr;
    }

    if (match(parser, TOKEN_LBRACKET)) {
        JagASTNode *node = jag_ast_create_node(AST_ARRAY_LITERAL, loc);
        size_t cap = 4;
        node->as.array_literal.elements = malloc(cap * sizeof(JagASTNode *));
        node->as.array_literal.count = 0;

        if (!check(parser, TOKEN_RBRACKET)) {
            do {
                if (node->as.array_literal.count >= cap) {
                    cap *= 2;
                    node->as.array_literal.elements = realloc(node->as.array_literal.elements, cap * sizeof(JagASTNode *));
                }
                node->as.array_literal.elements[node->as.array_literal.count++] = parse_expression(parser);
            } while (match(parser, TOKEN_COMMA));
        }
        consume(parser, TOKEN_RBRACKET, "Expected ']' after array elements");
        return node;
    }

    if (match(parser, TOKEN_LBRACE)) {
        JagASTNode *node = jag_ast_create_node(AST_DATA_LITERAL, loc);
        size_t cap = 4;
        node->as.data_literal.entries = malloc(cap * sizeof(JagDataEntry));
        node->as.data_literal.count = 0;

        if (!check(parser, TOKEN_RBRACE)) {
            do {
                if (node->as.data_literal.count >= cap) {
                    cap *= 2;
                    node->as.data_literal.entries = realloc(node->as.data_literal.entries, cap * sizeof(JagDataEntry));
                }
                JagDataEntry entry;
                if (check(parser, TOKEN_IDENTIFIER) || check(parser, TOKEN_STRING_LITERAL)) {
                    advance_parser(parser);
                    if (parser->previous.type == TOKEN_STRING_LITERAL) {
                        entry.key = jag_strdup(parser->previous.as.string_val ? parser->previous.as.string_val : "");
                    } else {
                        entry.key = copy_string(parser->previous.start, parser->previous.length);
                    }
                } else {
                    error_at(parser, &parser->current, "Expected key in data literal");
                    entry.key = jag_strdup("invalid");
                }
                consume(parser, TOKEN_COLON, "Expected ':' after key in data literal");
                entry.val = parse_expression(parser);
                node->as.data_literal.entries[node->as.data_literal.count++] = entry;
            } while (match(parser, TOKEN_COMMA));
        }
        consume(parser, TOKEN_RBRACE, "Expected '}' after data literal");
        return node;
    }

    error_at(parser, &parser->current, "Expected expression");
    return NULL;
}

static JagASTNode *parse_postfix(JagParser *parser) {
    JagASTNode *expr = parse_primary(parser);

    for (;;) {
        if (match(parser, TOKEN_DOT)) {
            JagSourceLoc loc = get_loc(parser);
            consume(parser, TOKEN_IDENTIFIER, "Expected member name after '.'");
            char *member = copy_string(parser->previous.start, parser->previous.length);

            if (expr->kind == AST_IDENTIFIER && strcmp(expr->as.identifier.name, "live") == 0 &&
                strcmp(member, "deg") == 0) {

                consume(parser, TOKEN_LPAREN, "Expected '(' after live.deg");

                char *type_name = NULL;
                if (check(parser, TOKEN_TYPE_NUM) || check(parser, TOKEN_TYPE_DECIMAL) ||
                    check(parser, TOKEN_TYPE_BOOL) || check(parser, TOKEN_TYPE_STRING) ||
                    check(parser, TOKEN_IDENTIFIER)) {
                    advance_parser(parser);
                    type_name = copy_string(parser->previous.start, parser->previous.length);
                } else {
                    error_at(parser, &parser->current, "Expected type name as 1st argument of live.deg");
                    type_name = jag_strdup("unknown");
                }
                consume(parser, TOKEN_COMMA, "Expected ',' after type in live.deg");

                JagASTNode *expected = parse_expression(parser);
                consume(parser, TOKEN_COMMA, "Expected ',' after expected value in live.deg");

                JagASTNode *var_expr = parse_expression(parser);
                consume(parser, TOKEN_RPAREN, "Expected ')' after live.deg arguments");

                jag_ast_free(expr);
                free(member);

                JagASTNode *deg_node = jag_ast_create_node(AST_LIVE_DEG, loc);
                deg_node->as.live_deg.type_name = type_name;
                deg_node->as.live_deg.expected = expected;
                deg_node->as.live_deg.var_expr = var_expr;
                expr = deg_node;
            } else {
                JagASTNode *m_node = jag_ast_create_node(AST_MEMBER_ACCESS, loc);
                m_node->as.member_access.object = expr;
                m_node->as.member_access.member = member;
                expr = m_node;
            }
        } else if (match(parser, TOKEN_LBRACKET)) {
            JagSourceLoc loc = get_loc(parser);
            JagASTNode *index = parse_expression(parser);
            consume(parser, TOKEN_RBRACKET, "Expected ']' after index");

            JagASTNode *i_node = jag_ast_create_node(AST_INDEX_ACCESS, loc);
            i_node->as.index_access.array_or_map = expr;
            i_node->as.index_access.index = index;
            expr = i_node;
        } else if (match(parser, TOKEN_LPAREN)) {
            JagSourceLoc loc = get_loc(parser);
            JagASTNode *c_node = jag_ast_create_node(AST_CALL, loc);
            c_node->as.call_expr.callee = expr;
            size_t cap = 4;
            c_node->as.call_expr.args = malloc(cap * sizeof(JagASTNode *));
            c_node->as.call_expr.arg_count = 0;

            if (!check(parser, TOKEN_RPAREN)) {
                do {
                    if (c_node->as.call_expr.arg_count >= cap) {
                        cap *= 2;
                        c_node->as.call_expr.args = realloc(c_node->as.call_expr.args, cap * sizeof(JagASTNode *));
                    }
                    c_node->as.call_expr.args[c_node->as.call_expr.arg_count++] = parse_expression(parser);
                } while (match(parser, TOKEN_COMMA));
            }
            consume(parser, TOKEN_RPAREN, "Expected ')' after function arguments");
            expr = c_node;
        } else {
            break;
        }
    }

    return expr;
}

static JagASTNode *parse_unary(JagParser *parser) {
    if (match(parser, TOKEN_MINUS) || match(parser, TOKEN_BANG) || match(parser, TOKEN_BIT_NOT)) {
        JagSourceLoc loc = get_loc(parser);
        JagTokenType op = parser->previous.type;
        JagASTNode *operand = parse_unary(parser);
        JagASTNode *node = jag_ast_create_node(AST_UNARY, loc);
        node->as.unary_expr.op = op;
        node->as.unary_expr.operand = operand;
        return node;
    }
    return parse_postfix(parser);
}

static JagASTNode *parse_exponent(JagParser *parser) {
    JagASTNode *left = parse_unary(parser);
    if (match(parser, TOKEN_POWER)) {
        JagSourceLoc loc = get_loc(parser);
        JagASTNode *right = parse_exponent(parser);
        JagASTNode *node = jag_ast_create_node(AST_BINARY, loc);
        node->as.binary_expr.op = TOKEN_POWER;
        node->as.binary_expr.left = left;
        node->as.binary_expr.right = right;
        return node;
    }
    return left;
}

static JagASTNode *parse_multiplicative(JagParser *parser) {
    JagASTNode *left = parse_exponent(parser);
    while (match(parser, TOKEN_STAR) || match(parser, TOKEN_SLASH) || match(parser, TOKEN_PERCENT)) {
        JagSourceLoc loc = get_loc(parser);
        JagTokenType op = parser->previous.type;
        JagASTNode *right = parse_exponent(parser);
        JagASTNode *node = jag_ast_create_node(AST_BINARY, loc);
        node->as.binary_expr.op = op;
        node->as.binary_expr.left = left;
        node->as.binary_expr.right = right;
        left = node;
    }
    return left;
}

static JagASTNode *parse_additive(JagParser *parser) {
    JagASTNode *left = parse_multiplicative(parser);
    while (match(parser, TOKEN_PLUS) || match(parser, TOKEN_MINUS)) {
        JagSourceLoc loc = get_loc(parser);
        JagTokenType op = parser->previous.type;
        JagASTNode *right = parse_multiplicative(parser);
        JagASTNode *node = jag_ast_create_node(AST_BINARY, loc);
        node->as.binary_expr.op = op;
        node->as.binary_expr.left = left;
        node->as.binary_expr.right = right;
        left = node;
    }
    return left;
}

static JagASTNode *parse_shift(JagParser *parser) {
    JagASTNode *left = parse_additive(parser);
    while (match(parser, TOKEN_SHL) || match(parser, TOKEN_SHR)) {
        JagSourceLoc loc = get_loc(parser);
        JagTokenType op = parser->previous.type;
        JagASTNode *right = parse_additive(parser);
        JagASTNode *node = jag_ast_create_node(AST_BINARY, loc);
        node->as.binary_expr.op = op;
        node->as.binary_expr.left = left;
        node->as.binary_expr.right = right;
        left = node;
    }
    return left;
}

static JagASTNode *parse_relational(JagParser *parser) {
    JagASTNode *left = parse_shift(parser);
    while (match(parser, TOKEN_LESS) || match(parser, TOKEN_LESS_EQ) ||
           match(parser, TOKEN_GREATER) || match(parser, TOKEN_GREATER_EQ)) {
        JagSourceLoc loc = get_loc(parser);
        JagTokenType op = parser->previous.type;
        JagASTNode *right = parse_shift(parser);
        JagASTNode *node = jag_ast_create_node(AST_BINARY, loc);
        node->as.binary_expr.op = op;
        node->as.binary_expr.left = left;
        node->as.binary_expr.right = right;
        left = node;
    }
    return left;
}

static JagASTNode *parse_equality(JagParser *parser) {
    JagASTNode *left = parse_relational(parser);
    while (match(parser, TOKEN_EQ_EQ) || match(parser, TOKEN_BANG_EQ)) {
        JagSourceLoc loc = get_loc(parser);
        JagTokenType op = parser->previous.type;
        JagASTNode *right = parse_relational(parser);
        JagASTNode *node = jag_ast_create_node(AST_BINARY, loc);
        node->as.binary_expr.op = op;
        node->as.binary_expr.left = left;
        node->as.binary_expr.right = right;
        left = node;
    }
    return left;
}

static JagASTNode *parse_bit_and(JagParser *parser) {
    JagASTNode *left = parse_equality(parser);
    while (match(parser, TOKEN_BIT_AND)) {
        JagSourceLoc loc = get_loc(parser);
        JagASTNode *right = parse_equality(parser);
        JagASTNode *node = jag_ast_create_node(AST_BINARY, loc);
        node->as.binary_expr.op = TOKEN_BIT_AND;
        node->as.binary_expr.left = left;
        node->as.binary_expr.right = right;
        left = node;
    }
    return left;
}

static JagASTNode *parse_bit_xor(JagParser *parser) {
    JagASTNode *left = parse_bit_and(parser);
    while (match(parser, TOKEN_BIT_XOR)) {
        JagSourceLoc loc = get_loc(parser);
        JagASTNode *right = parse_bit_and(parser);
        JagASTNode *node = jag_ast_create_node(AST_BINARY, loc);
        node->as.binary_expr.op = TOKEN_BIT_XOR;
        node->as.binary_expr.left = left;
        node->as.binary_expr.right = right;
        left = node;
    }
    return left;
}

static JagASTNode *parse_bit_or(JagParser *parser) {
    JagASTNode *left = parse_bit_xor(parser);
    while (match(parser, TOKEN_BIT_OR)) {
        JagSourceLoc loc = get_loc(parser);
        JagASTNode *right = parse_bit_xor(parser);
        JagASTNode *node = jag_ast_create_node(AST_BINARY, loc);
        node->as.binary_expr.op = TOKEN_BIT_OR;
        node->as.binary_expr.left = left;
        node->as.binary_expr.right = right;
        left = node;
    }
    return left;
}

static JagASTNode *parse_logical_and(JagParser *parser) {
    JagASTNode *left = parse_bit_or(parser);
    while (match(parser, TOKEN_AND)) {
        JagSourceLoc loc = get_loc(parser);
        JagASTNode *right = parse_bit_or(parser);
        JagASTNode *node = jag_ast_create_node(AST_BINARY, loc);
        node->as.binary_expr.op = TOKEN_AND;
        node->as.binary_expr.left = left;
        node->as.binary_expr.right = right;
        left = node;
    }
    return left;
}

static JagASTNode *parse_logical_or(JagParser *parser) {
    JagASTNode *left = parse_logical_and(parser);
    while (match(parser, TOKEN_OR)) {
        JagSourceLoc loc = get_loc(parser);
        JagASTNode *right = parse_logical_and(parser);
        JagASTNode *node = jag_ast_create_node(AST_BINARY, loc);
        node->as.binary_expr.op = TOKEN_OR;
        node->as.binary_expr.left = left;
        node->as.binary_expr.right = right;
        left = node;
    }
    return left;
}

static JagASTNode *parse_ternary(JagParser *parser) {
    JagASTNode *expr = parse_logical_or(parser);
    if (match(parser, TOKEN_QUESTION)) {
        JagSourceLoc loc = get_loc(parser);
        JagASTNode *then_expr = parse_expression(parser);
        consume(parser, TOKEN_COLON, "Expected ':' in ternary expression");
        JagASTNode *else_expr = parse_expression(parser);

        JagASTNode *node = jag_ast_create_node(AST_TERNARY, loc);
        node->as.ternary_expr.cond = expr;
        node->as.ternary_expr.then_expr = then_expr;
        node->as.ternary_expr.else_expr = else_expr;
        return node;
    }
    return expr;
}

static JagASTNode *parse_assignment(JagParser *parser) {
    JagASTNode *expr = parse_ternary(parser);

    if (match(parser, TOKEN_EQ) || match(parser, TOKEN_PLUS_EQ) ||
        match(parser, TOKEN_MINUS_EQ) || match(parser, TOKEN_STAR_EQ) ||
        match(parser, TOKEN_SLASH_EQ) || match(parser, TOKEN_PERCENT_EQ)) {

        JagSourceLoc loc = get_loc(parser);
        JagTokenType op = parser->previous.type;
        JagASTNode *value = parse_assignment(parser);

        JagASTNode *node = jag_ast_create_node(AST_ASSIGNMENT, loc);
        node->as.assignment.target = expr;
        node->as.assignment.op = op;
        node->as.assignment.value = value;
        return node;
    }

    return expr;
}

static JagASTNode *parse_expression(JagParser *parser) {
    return parse_assignment(parser);
}

static JagASTNode *parse_block(JagParser *parser) {
    JagSourceLoc loc = get_loc(parser);
    consume(parser, TOKEN_LBRACE, "Expected '{' to start block");

    JagASTNode *node = jag_ast_create_node(AST_BLOCK, loc);
    size_t cap = 4;
    node->as.block.stmts = malloc(cap * sizeof(JagASTNode *));
    node->as.block.count = 0;

    while (!check(parser, TOKEN_RBRACE) && !check(parser, TOKEN_EOF)) {
        if (node->as.block.count >= cap) {
            cap *= 2;
            node->as.block.stmts = realloc(node->as.block.stmts, cap * sizeof(JagASTNode *));
        }
        node->as.block.stmts[node->as.block.count++] = parse_declaration(parser);
    }

    consume(parser, TOKEN_RBRACE, "Expected '}' after block");
    return node;
}

static JagASTNode *parse_var_decl(JagParser *parser, bool is_fixed) {
    JagSourceLoc loc = get_loc(parser);
    advance_parser(parser);

    consume(parser, TOKEN_IDENTIFIER, "Expected variable name");
    char *name = copy_string(parser->previous.start, parser->previous.length);

    consume(parser, TOKEN_COLON, "Expected ':' after variable name");
    JagTypeSpec type = parse_type_spec(parser);

    JagASTNode *init = NULL;
    if (match(parser, TOKEN_EQ)) {
        init = parse_expression(parser);
    } else if (is_fixed) {
        error_at(parser, &parser->previous, "Fixed variable must be initialized");
    }

    consume(parser, TOKEN_SEMICOLON, "Expected ';' after variable declaration");

    JagASTNode *node = jag_ast_create_node(AST_VAR_DECL, loc);
    node->as.var_decl.name = name;
    node->as.var_decl.type = type;
    node->as.var_decl.init = init;
    node->as.var_decl.is_fixed = is_fixed;
    return node;
}

static JagASTNode *parse_fun_decl(JagParser *parser, bool is_async) {
    JagSourceLoc loc = get_loc(parser);
    advance_parser(parser);

    consume(parser, TOKEN_IDENTIFIER, "Expected function name");
    char *name = copy_string(parser->previous.start, parser->previous.length);

    consume(parser, TOKEN_LPAREN, "Expected '(' after function name");

    size_t param_cap = 4;
    JagParam *params = malloc(param_cap * sizeof(JagParam));
    size_t param_count = 0;

    if (!check(parser, TOKEN_RPAREN)) {
        do {
            if (param_count >= param_cap) {
                param_cap *= 2;
                params = realloc(params, param_cap * sizeof(JagParam));
            }
            consume(parser, TOKEN_IDENTIFIER, "Expected parameter name");
            params[param_count].name = copy_string(parser->previous.start, parser->previous.length);
            consume(parser, TOKEN_COLON, "Expected ':' after parameter name");
            params[param_count].type = parse_type_spec(parser);
            param_count++;
        } while (match(parser, TOKEN_COMMA));
    }

    consume(parser, TOKEN_RPAREN, "Expected ')' after parameters");

    JagTypeSpec return_type;
    if (match(parser, TOKEN_COLON)) {
        return_type = parse_type_spec(parser);
    } else {
        return_type.name = jag_strdup("void");
        return_type.is_array = false;
    }

    JagASTNode *body = parse_block(parser);

    JagASTNode *node = jag_ast_create_node(AST_FUN_DECL, loc);
    node->as.fun_decl.name = name;
    node->as.fun_decl.params = params;
    node->as.fun_decl.param_count = param_count;
    node->as.fun_decl.return_type = return_type;
    node->as.fun_decl.body = body;
    node->as.fun_decl.is_async = is_async;
    return node;
}

static JagASTNode *parse_struct_decl(JagParser *parser) {
    JagSourceLoc loc = get_loc(parser);
    advance_parser(parser);

    consume(parser, TOKEN_IDENTIFIER, "Expected struct name");
    char *name = copy_string(parser->previous.start, parser->previous.length);

    consume(parser, TOKEN_LBRACE, "Expected '{' in struct declaration");

    size_t field_cap = 4;
    JagField *fields = malloc(field_cap * sizeof(JagField));
    size_t field_count = 0;

    while (!check(parser, TOKEN_RBRACE) && !check(parser, TOKEN_EOF)) {
        if (field_count >= field_cap) {
            field_cap *= 2;
            fields = realloc(fields, field_cap * sizeof(JagField));
        }
        consume(parser, TOKEN_IDENTIFIER, "Expected field name");
        fields[field_count].name = copy_string(parser->previous.start, parser->previous.length);
        consume(parser, TOKEN_COLON, "Expected ':' after field name");
        fields[field_count].type = parse_type_spec(parser);
        fields[field_count].is_public = true;
        field_count++;
        match(parser, TOKEN_SEMICOLON);
    }

    consume(parser, TOKEN_RBRACE, "Expected '}' after struct body");

    JagASTNode *node = jag_ast_create_node(AST_STRUCT_DECL, loc);
    node->as.struct_decl.name = name;
    node->as.struct_decl.fields = fields;
    node->as.struct_decl.field_count = field_count;
    return node;
}

static JagASTNode *parse_class_decl(JagParser *parser) {
    JagSourceLoc loc = get_loc(parser);
    advance_parser(parser);

    consume(parser, TOKEN_IDENTIFIER, "Expected class name");
    char *name = copy_string(parser->previous.start, parser->previous.length);

    consume(parser, TOKEN_LBRACE, "Expected '{' in class declaration");

    size_t field_cap = 4;
    JagField *fields = malloc(field_cap * sizeof(JagField));
    size_t field_count = 0;

    size_t method_cap = 4;
    JagASTNode **methods = malloc(method_cap * sizeof(JagASTNode *));
    size_t method_count = 0;

    while (!check(parser, TOKEN_RBRACE) && !check(parser, TOKEN_EOF)) {
        bool is_public = true;
        if (match(parser, TOKEN_PUBLIC)) {
            is_public = true;
        } else if (match(parser, TOKEN_PRIVATE)) {
            is_public = false;
        }

        if (check(parser, TOKEN_FUN) || check(parser, TOKEN_ASYNC)) {
            if (method_count >= method_cap) {
                method_cap *= 2;
                methods = realloc(methods, method_cap * sizeof(JagASTNode *));
            }
            bool is_async = match(parser, TOKEN_ASYNC);
            methods[method_count++] = parse_fun_decl(parser, is_async);
        } else if (check(parser, TOKEN_IDENTIFIER)) {
            if (field_count >= field_cap) {
                field_cap *= 2;
                fields = realloc(fields, field_cap * sizeof(JagField));
            }
            advance_parser(parser);
            fields[field_count].name = copy_string(parser->previous.start, parser->previous.length);
            consume(parser, TOKEN_COLON, "Expected ':' after field name");
            fields[field_count].type = parse_type_spec(parser);
            fields[field_count].is_public = is_public;
            field_count++;
            consume(parser, TOKEN_SEMICOLON, "Expected ';' after class field");
        } else {
            error_at(parser, &parser->current, "Expected field or method in class body");
            advance_parser(parser);
        }
    }

    consume(parser, TOKEN_RBRACE, "Expected '}' after class body");

    JagASTNode *node = jag_ast_create_node(AST_CLASS_DECL, loc);
    node->as.class_decl.name = name;
    node->as.class_decl.fields = fields;
    node->as.class_decl.field_count = field_count;
    node->as.class_decl.methods = methods;
    node->as.class_decl.method_count = method_count;
    return node;
}

static JagASTNode *parse_enum_decl(JagParser *parser) {
    JagSourceLoc loc = get_loc(parser);
    advance_parser(parser);

    consume(parser, TOKEN_IDENTIFIER, "Expected enum name");
    char *name = copy_string(parser->previous.start, parser->previous.length);

    consume(parser, TOKEN_LBRACE, "Expected '{' in enum declaration");

    size_t cap = 4;
    char **enumerators = malloc(cap * sizeof(char *));
    size_t count = 0;

    if (!check(parser, TOKEN_RBRACE)) {
        do {
            if (count >= cap) {
                cap *= 2;
                enumerators = realloc(enumerators, cap * sizeof(char *));
            }
            consume(parser, TOKEN_IDENTIFIER, "Expected enumerator name");
            enumerators[count++] = copy_string(parser->previous.start, parser->previous.length);
        } while (match(parser, TOKEN_COMMA));
    }

    consume(parser, TOKEN_RBRACE, "Expected '}' after enum enumerators");

    JagASTNode *node = jag_ast_create_node(AST_ENUM_DECL, loc);
    node->as.enum_decl.name = name;
    node->as.enum_decl.enumerators = enumerators;
    node->as.enum_decl.enum_count = count;
    return node;
}

static JagASTNode *parse_data_decl(JagParser *parser) {
    JagSourceLoc loc = get_loc(parser);
    advance_parser(parser);

    consume(parser, TOKEN_IDENTIFIER, "Expected data variable name");
    char *name = copy_string(parser->previous.start, parser->previous.length);

    consume(parser, TOKEN_EQ, "Expected '=' in data declaration");
    JagASTNode *init = parse_expression(parser);
    consume(parser, TOKEN_SEMICOLON, "Expected ';' after data declaration");

    JagTypeSpec type;
    type.name = jag_strdup("data");
    type.is_array = false;

    JagASTNode *node = jag_ast_create_node(AST_VAR_DECL, loc);
    node->as.var_decl.name = name;
    node->as.var_decl.type = type;
    node->as.var_decl.init = init;
    node->as.var_decl.is_fixed = false;
    return node;
}

static JagASTNode *parse_if_stmt(JagParser *parser) {
    JagSourceLoc loc = get_loc(parser);
    advance_parser(parser);

    consume(parser, TOKEN_LPAREN, "Expected '(' after 'if'");
    JagASTNode *cond = parse_expression(parser);
    consume(parser, TOKEN_RPAREN, "Expected ')' after if condition");

    JagASTNode *then_branch = parse_block(parser);

    size_t elif_cap = 4;
    JagASTNode **elif_conds = malloc(elif_cap * sizeof(JagASTNode *));
    JagASTNode **elif_branches = malloc(elif_cap * sizeof(JagASTNode *));
    size_t elif_count = 0;

    while (match(parser, TOKEN_ELIF)) {
        if (elif_count >= elif_cap) {
            elif_cap *= 2;
            elif_conds = realloc(elif_conds, elif_cap * sizeof(JagASTNode *));
            elif_branches = realloc(elif_branches, elif_cap * sizeof(JagASTNode *));
        }
        consume(parser, TOKEN_LPAREN, "Expected '(' after 'elif'");
        elif_conds[elif_count] = parse_expression(parser);
        consume(parser, TOKEN_RPAREN, "Expected ')' after elif condition");
        elif_branches[elif_count] = parse_block(parser);
        elif_count++;
    }

    JagASTNode *else_branch = NULL;
    if (match(parser, TOKEN_ELSE)) {
        else_branch = parse_block(parser);
    }

    JagASTNode *node = jag_ast_create_node(AST_IF, loc);
    node->as.if_stmt.cond = cond;
    node->as.if_stmt.then_branch = then_branch;
    node->as.if_stmt.elif_conds = elif_conds;
    node->as.if_stmt.elif_branches = elif_branches;
    node->as.if_stmt.elif_count = elif_count;
    node->as.if_stmt.else_branch = else_branch;
    return node;
}

static JagASTNode *parse_loop_stmt(JagParser *parser) {
    JagSourceLoc loc = get_loc(parser);
    advance_parser(parser);

    consume(parser, TOKEN_LPAREN, "Expected '(' after 'loop'");
    JagASTNode *cond = parse_expression(parser);
    consume(parser, TOKEN_RPAREN, "Expected ')' after loop condition");

    JagASTNode *body = parse_block(parser);

    JagASTNode *node = jag_ast_create_node(AST_LOOP, loc);
    node->as.loop_stmt.cond = cond;
    node->as.loop_stmt.body = body;
    return node;
}

static JagASTNode *parse_do_loop_stmt(JagParser *parser) {
    JagSourceLoc loc = get_loc(parser);
    advance_parser(parser);
    consume(parser, TOKEN_LOOP, "Expected 'loop' after 'do'");

    JagASTNode *body = parse_block(parser);

    consume(parser, TOKEN_LOOP, "Expected 'while' or 'loop' after 'do loop' body");
    consume(parser, TOKEN_LPAREN, "Expected '(' after loop condition keyword");
    JagASTNode *cond = parse_expression(parser);
    consume(parser, TOKEN_RPAREN, "Expected ')' after do loop condition");
    consume(parser, TOKEN_SEMICOLON, "Expected ';' after do loop statement");

    JagASTNode *node = jag_ast_create_node(AST_DO_LOOP, loc);
    node->as.do_loop_stmt.body = body;
    node->as.do_loop_stmt.cond = cond;
    return node;
}

static JagASTNode *parse_for_in_stmt(JagParser *parser) {
    JagSourceLoc loc = get_loc(parser);
    advance_parser(parser);

    consume(parser, TOKEN_IDENTIFIER, "Expected loop variable name after 'for'");
    char *var_name = copy_string(parser->previous.start, parser->previous.length);

    consume(parser, TOKEN_IN, "Expected 'in' after loop variable");
    JagASTNode *collection = parse_expression(parser);
    JagASTNode *body = parse_block(parser);

    JagASTNode *node = jag_ast_create_node(AST_FOR_IN, loc);
    node->as.for_in_stmt.var_name = var_name;
    node->as.for_in_stmt.collection = collection;
    node->as.for_in_stmt.body = body;
    return node;
}

static JagASTNode *parse_return_stmt(JagParser *parser) {
    JagSourceLoc loc = get_loc(parser);
    advance_parser(parser);

    JagASTNode *expr = NULL;
    if (!check(parser, TOKEN_SEMICOLON)) {
        expr = parse_expression(parser);
    }
    consume(parser, TOKEN_SEMICOLON, "Expected ';' after return statement");

    JagASTNode *node = jag_ast_create_node(AST_RETURN, loc);
    node->as.return_stmt.expr = expr;
    return node;
}

static JagASTNode *parse_import_stmt(JagParser *parser) {
    JagSourceLoc loc = get_loc(parser);
    advance_parser(parser);

    char *path = NULL;
    if (match(parser, TOKEN_STRING_LITERAL)) {
        path = jag_strdup(parser->previous.as.string_val ? parser->previous.as.string_val : "");
    } else if (match(parser, TOKEN_IDENTIFIER)) {
        path = copy_string(parser->previous.start, parser->previous.length);
        while (match(parser, TOKEN_DOT)) {
            consume(parser, TOKEN_IDENTIFIER, "Expected identifier after '.' in import path");
            size_t old_len = strlen(path);
            size_t seg_len = parser->previous.length;
            path = realloc(path, old_len + 1 + seg_len + 1);
            strcat(path, ".");
            strncat(path, parser->previous.start, seg_len);
        }
    } else {
        error_at(parser, &parser->current, "Expected import file path");
        path = jag_strdup("");
    }

    consume(parser, TOKEN_SEMICOLON, "Expected ';' after import statement");

    JagASTNode *node = jag_ast_create_node(AST_IMPORT, loc);
    node->as.import_stmt.path = path;
    return node;
}

static JagASTNode *parse_export_stmt(JagParser *parser) {
    JagSourceLoc loc = get_loc(parser);
    advance_parser(parser);

    JagASTNode *decl = parse_declaration(parser);

    JagASTNode *node = jag_ast_create_node(AST_EXPORT, loc);
    node->as.export_stmt.decl = decl;
    return node;
}

static JagASTNode *parse_declaration(JagParser *parser) {
    if (check(parser, TOKEN_LIVE) && parser->lexer.source[parser->lexer.cursor] == ' ' &&
        (strncmp(&parser->lexer.source[parser->lexer.cursor + 1], "= \"1\"", 5) == 0 ||
         strncmp(&parser->lexer.source[parser->lexer.cursor + 1], "=\"1\"", 4) == 0)) {

        JagSourceLoc loc = get_loc(parser);
        advance_parser(parser);
        consume(parser, TOKEN_EQ, "Expected '=' in live activation");
        consume(parser, TOKEN_STRING_LITERAL, "Expected string \"1\" in live activation");
        char *val = jag_strdup(parser->previous.as.string_val ? parser->previous.as.string_val : "1");
        consume(parser, TOKEN_SEMICOLON, "Expected ';' after live activation");

        JagASTNode *node = jag_ast_create_node(AST_LIVE_ACTIVATION, loc);
        node->as.live_activation.value = val;
        return node;
    }

    if (check(parser, TOKEN_VAR)) return parse_var_decl(parser, false);
    if (check(parser, TOKEN_FIXED)) return parse_var_decl(parser, true);
    if (check(parser, TOKEN_FUN)) return parse_fun_decl(parser, false);
    if (check(parser, TOKEN_ASYNC)) {
        advance_parser(parser);
        return parse_fun_decl(parser, true);
    }
    if (check(parser, TOKEN_STRUCT)) return parse_struct_decl(parser);
    if (check(parser, TOKEN_CLASS)) return parse_class_decl(parser);
    if (check(parser, TOKEN_ENUM)) return parse_enum_decl(parser);
    if (check(parser, TOKEN_DATA)) return parse_data_decl(parser);
    if (check(parser, TOKEN_IMPORT)) return parse_import_stmt(parser);
    if (check(parser, TOKEN_EXPORT)) return parse_export_stmt(parser);

    return parse_statement(parser);
}

static JagASTNode *parse_statement(JagParser *parser) {
    if (check(parser, TOKEN_IF)) return parse_if_stmt(parser);
    if (check(parser, TOKEN_LOOP)) return parse_loop_stmt(parser);
    if (check(parser, TOKEN_DO)) return parse_do_loop_stmt(parser);
    if (check(parser, TOKEN_FOR)) return parse_for_in_stmt(parser);
    if (check(parser, TOKEN_RETURN)) return parse_return_stmt(parser);
    if (check(parser, TOKEN_LBRACE)) return parse_block(parser);

    JagSourceLoc loc = get_loc(parser);
    JagASTNode *expr = parse_expression(parser);
    consume(parser, TOKEN_SEMICOLON, "Expected ';' after expression statement");

    JagASTNode *node = jag_ast_create_node(AST_EXPR_STMT, loc);
    node->as.expr_stmt.expr = expr;
    return node;
}

void jag_parser_init(JagParser *parser, const char *source, const char *filename) {
    jag_lexer_init(&parser->lexer, source, filename);
    parser->had_error = false;
    parser->panic_mode = false;
    parser->error_msg[0] = '\0';
    advance_parser(parser);
}

JagASTNode *jag_parse_program(JagParser *parser) {
    JagSourceLoc loc = get_loc(parser);
    JagASTNode *program = jag_ast_create_node(AST_PROGRAM, loc);

    size_t cap = 8;
    program->as.program.stmts = malloc(cap * sizeof(JagASTNode *));
    program->as.program.count = 0;

    while (!check(parser, TOKEN_EOF)) {
        JagASTNode *decl = parse_declaration(parser);
        if (decl) {
            if (program->as.program.count >= cap) {
                cap *= 2;
                program->as.program.stmts = realloc(program->as.program.stmts, cap * sizeof(JagASTNode *));
            }
            program->as.program.stmts[program->as.program.count++] = decl;
        } else {
            advance_parser(parser);
        }
    }

    return program;
}
