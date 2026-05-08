#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "compiler/ast/ast.h"

Parser* PARSER_create(Lexer* lexer) {
    Parser* parser = malloc(sizeof(Parser));
    parser->lexer = lexer;
    parser->cur_token = LEXER_next(lexer);
    parser->peek_token = LEXER_next(lexer);
    return parser;
}

void PARSER_free(Parser* parser) {
    free(parser->cur_token->value);
    free(parser->cur_token);
    free(parser->peek_token->value);
    free(parser->peek_token);
    free(parser);
}

static Token* advance(Parser* parser) {
    Token* token = parser->cur_token;
    parser->cur_token = parser->peek_token;
    parser->peek_token = LEXER_next(parser->lexer);
    return token;
}

static Token* expect(Parser* parser, const TokenType exp_type) {
    if (parser->cur_token->type == exp_type)
        return advance(parser);
    printf("Expected token type ");
    print_token_type(exp_type);
    printf(", but got token type ");
    print_token_type(parser->cur_token->type);
    printf(" on line %d", parser->cur_token->line);
    return NULL;
}

static void free_token(Token* t) {
    free(t->value);
    free(t);
}

static int consume(Parser* parser, const TokenType type) {
    Token* t = expect(parser, type);
    if (!t) return 0;
    free_token(t);
    return 1;
}

static void skip_token(Parser* parser, const TokenType type, const int count) {
    if (count == -1) {
        while (parser->cur_token->type == type) {
            Token* t = advance(parser);
            free_token(t);
        }
    } else {
        for (int i = 0; i < count && parser->cur_token->type == type; i++) {
            Token* t = advance(parser);
            free_token(t);
        }
    }
}

static void free_f(void* data) {
    AST_free(data);
}

static ASTNode* parse_expression(Parser* parser);

static ASTNode* make_var_decl(const int line, char* type_name, char* name) {
    ASTNode* node = AST_create_node(NODE_VAR_DECL, line);
    node->as.dec_ass.type_name = type_name;
    node->as.dec_ass.name = name;
    node->as.dec_ass.stmt = NULL;
    node->as.dec_ass.is_const = 0;
    return node;
}

static int get_precedence(const TokenType type) {
    switch (type) {
        case TOKEN_OR: return 1;
        case TOKEN_AND: return 2;
        case TOKEN_EQ:
        case TOKEN_NEQ: return 3;
        case TOKEN_LT:
        case TOKEN_GT:
        case TOKEN_LTE:
        case TOKEN_GTE: return 4;
        case TOKEN_PLUS:
        case TOKEN_MINUS: return 5;
        case TOKEN_STAR:
        case TOKEN_SLASH:
        case TOKEN_PERCENT: return 6;
        default: return 0;
    }
}

static ASTNode* parse_expr(Parser* parser, const int min_prec) {
    ASTNode* left;
    switch (parser->cur_token->type) {
        case TOKEN_INT_LIT: {
            Token* t = advance(parser);
            ASTNode* node = AST_create_node(NODE_INT_LIT, t->line);
            node->as.int_val = (int)strtol(t->value, NULL, 10);
            free_token(t);
            left = node;
            break;
        }
        case TOKEN_FLOAT_LIT: {
            Token* t = advance(parser);
            ASTNode* node = AST_create_node(NODE_FLOAT_LIT, t->line);
            node->as.float_val = strtof(t->value, NULL);
            free_token(t);
            left = node;
            break;
        }
        case TOKEN_STRING_LIT: {
            Token* t = advance(parser);
            ASTNode* node = AST_create_node(NODE_STR_LIT, t->line);
            node->as.str_val = t->value;
            free(t);
            left = node;
            break;
        }
        case TOKEN_CHAR_LIT: {
            Token* t = advance(parser);
            ASTNode* node = AST_create_node(NODE_CHAR_LIT, t->line);
            node->as.str_val = t->value;
            free(t);
            left = node;
            break;
        }
        case TOKEN_IDENT: {
            Token* t = advance(parser);
            ASTNode* node = AST_create_node(NODE_IDENT, t->line);
            node->as.str_val = t->value;
            free(t);
            left = node;
            break;
        }
        case TOKEN_TRUE: {
            Token* t = advance(parser);
            ASTNode* node = AST_create_node(NODE_BOOL_LIT, t->line);
            node->as.int_val = 1;
            free_token(t);
            left = node;
            break;
        }
        case TOKEN_FALSE: {
            Token* t = advance(parser);
            ASTNode* node = AST_create_node(NODE_BOOL_LIT, t->line);
            node->as.int_val = 0;
            free_token(t);
            left = node;
            break;
        }
        case TOKEN_NULL: {
            Token* t = advance(parser);
            ASTNode* node = AST_create_node(NODE_NULL_LIT, t->line);
            free_token(t);
            left = node;
            break;
        }
        case TOKEN_MINUS:
        case TOKEN_NOT: {
            Token* t = advance(parser);
            char* op = t->value;
            const int line = t->line;
            free(t);
            ASTNode* stmt = parse_expr(parser, 6);
            ASTNode* node = AST_create_node(NODE_UNOP, line);
            node->as.unop.op = op;
            node->as.unop.stmt = stmt;
            left = node;
            break;
        }
        case TOKEN_LPAREN: {
            free_token(advance(parser));
            ASTNode* expr = parse_expr(parser, 0);
            Token* rparen = expect(parser, TOKEN_RPAREN);
            if (rparen) {
                free_token(rparen);
                left = expr;
                break;
            }
            if (expr) AST_free(expr);
            return NULL;
        }
        default: return NULL;
    }
    if (!left) return NULL;

    // postfix: calls, index access, field access
    while (1) {
        if (parser->cur_token->type == TOKEN_LPAREN) {
            const int call_line = left->line;
            free_token(advance(parser));
            List* args = LIST_create(NULL, free_f, NULL);
            while (parser->cur_token->type != TOKEN_RPAREN &&
                   parser->cur_token->type != TOKEN_EOF) {
                skip_token(parser, TOKEN_NEWLINE, -1);
                if (parser->cur_token->type == TOKEN_RPAREN) break;
                ASTNode* arg = parse_expr(parser, 0);
                if (!arg) { LIST_free(args); AST_free(left); return NULL; }
                LIST_append(args, arg);
                skip_token(parser, TOKEN_COMMA, 1);
            }
            if (!consume(parser, TOKEN_RPAREN)) { LIST_free(args); AST_free(left); return NULL; }
            ASTNode* call = AST_create_node(NODE_FUNC_CALL, call_line);
            call->as.f_call.callee = left;
            call->as.f_call.exps = args;
            left = call;
        } else if (parser->cur_token->type == TOKEN_LBRACKET) {
            const int idx_line = parser->cur_token->line;
            free_token(advance(parser));
            ASTNode* index = parse_expr(parser, 0);
            if (!index) { AST_free(left); return NULL; }
            if (!consume(parser, TOKEN_RBRACKET)) { AST_free(index); AST_free(left); return NULL; }
            ASTNode* idx_node = AST_create_node(NODE_IDX_ACC, idx_line);
            idx_node->as.idx_acc.obj = left;
            idx_node->as.idx_acc.index = index;
            left = idx_node;
        } else if (parser->cur_token->type == TOKEN_DOT) {
            free_token(advance(parser));
            Token* field_token = expect(parser, TOKEN_IDENT);
            if (!field_token) { AST_free(left); return NULL; }
            char* field_name = field_token->value;
            free(field_token);
            ASTNode* fld_node = AST_create_node(NODE_FLD_ACC, left->line);
            fld_node->as.fld_acc.obj = left;
            fld_node->as.fld_acc.field_name = field_name;
            left = fld_node;
        } else {
            break;
        }
    }

    while (get_precedence(parser->cur_token->type) > min_prec) {
        const int prec = get_precedence(parser->cur_token->type);
        Token* op_token = advance(parser);
        char* op = op_token->value;
        const int line = op_token->line;
        free(op_token);
        ASTNode* right = parse_expr(parser, prec);
        ASTNode* binop = AST_create_node(NODE_BINOP, line);
        binop->as.binop.left = left;
        binop->as.binop.right = right;
        binop->as.binop.op = op;
        left = binop;
    }
    return left;
}

static ASTNode* parse_expression(Parser* parser) {
    return parse_expr(parser, 0);
}

static ASTNode* parse_block(Parser* parser);
static int is_type_keyword(TokenType type);
static ASTNode* parse_func_decl(Parser* parser);

static ASTNode* parse_if(Parser* parser) {
    const int line = parser->cur_token->line;
    free_token(advance(parser));

    if (!consume(parser, TOKEN_LPAREN)) return NULL;

    ASTNode* cond = parse_expression(parser);
    if (!cond) return NULL;

    if (!consume(parser, TOKEN_RPAREN)) { AST_free(cond); return NULL; }

    ASTNode* block = parse_block(parser);
    if (!block) { AST_free(cond); return NULL; }

    ASTNode* if_else = AST_create_node(NODE_IF_ELSE, line);
    if_else->as.if_else.stmt = cond;
    if_else->as.if_else.block1 = block;

    skip_token(parser, TOKEN_NEWLINE, -1);
    if (parser->cur_token->type == TOKEN_ELSE) {
        free_token(advance(parser));
        if (parser->cur_token->type == TOKEN_IF)
            if_else->as.if_else.block2 = parse_if(parser);
        else
            if_else->as.if_else.block2 = parse_block(parser);
    } else {
        if_else->as.if_else.block2 = NULL;
    }
    return if_else;
}

static ASTNode* parse_while(Parser* parser) {
    const int line = parser->cur_token->line;
    free_token(advance(parser));

    if (!consume(parser, TOKEN_LPAREN)) return NULL;

    ASTNode* expr = parse_expression(parser);
    if (!expr) return NULL;

    if (!consume(parser, TOKEN_RPAREN)) { AST_free(expr); return NULL; }

    ASTNode* block = parse_block(parser);
    if (!block) { AST_free(expr); return NULL; }

    ASTNode* while_node = AST_create_node(NODE_WHILE, line);
    while_node->as._while.stmt = expr;
    while_node->as._while.block = block;
    return while_node;
}

static ASTNode* parse_statement(Parser* parser);

static ASTNode* parse_for(Parser* parser) {
    const int line = parser->cur_token->line;
    free_token(advance(parser));

    if (!consume(parser, TOKEN_LPAREN)) return NULL;

    const TokenType cur_ttype = parser->cur_token->type;
    if (cur_ttype == TOKEN_LPAREN) {
        free_token(advance(parser));

        List* vnames = LIST_create(NULL, free, NULL);
        while (parser->cur_token->type != TOKEN_RPAREN &&
               parser->cur_token->type != TOKEN_EOF) {
            skip_token(parser, TOKEN_NEWLINE, -1);
            if (parser->cur_token->type == TOKEN_RPAREN) break;
            Token* idf_token = expect(parser, TOKEN_IDENT);
            if (!idf_token) { LIST_free(vnames); return NULL; }
            LIST_append(vnames, idf_token->value);
            free(idf_token);

            Token* comma = expect(parser, TOKEN_COMMA);
            if (!comma) {
                if (parser->cur_token->type == TOKEN_RPAREN) break;
                LIST_free(vnames);
                return NULL;
            }
            free_token(comma);
        }
        free_token(advance(parser));

        if (!consume(parser, TOKEN_IN)) { LIST_free(vnames); return NULL; }

        ASTNode* coll = parse_expression(parser);
        if (!coll) { LIST_free(vnames); return NULL; }

        if (!consume(parser, TOKEN_RPAREN)) { AST_free(coll); LIST_free(vnames); return NULL; }

        ASTNode* block = parse_block(parser);
        if (!block) { AST_free(coll); LIST_free(vnames); return NULL; }

        ASTNode* foreach_node = AST_create_node(NODE_FOREACH, line);
        foreach_node->as.foreach.vnames = vnames;
        foreach_node->as.foreach.expression = coll;
        foreach_node->as.foreach.block = block;
        return foreach_node;
    }
    if (cur_ttype == TOKEN_IDENT && parser->peek_token->type == TOKEN_IN) {
        Token* idf_token = advance(parser);
        char* idf_name = idf_token->value;
        free(idf_token);

        if (!consume(parser, TOKEN_IN)) { free(idf_name); return NULL; }

        ASTNode* coll = parse_expression(parser);
        if (!coll) { free(idf_name); return NULL; }

        if (!consume(parser, TOKEN_RPAREN)) { AST_free(coll); free(idf_name); return NULL; }

        ASTNode* block = parse_block(parser);
        if (!block) { AST_free(coll); free(idf_name); return NULL; }

        ASTNode* foreach_node = AST_create_node(NODE_FOREACH, line);
        List* vnames = LIST_create(NULL, free, NULL);
        LIST_append(vnames, idf_name);
        foreach_node->as.foreach.vnames = vnames;
        foreach_node->as.foreach.expression = coll;
        foreach_node->as.foreach.block = block;
        return foreach_node;
    }

    ASTNode* init = parse_statement(parser);
    if (!init) return NULL;

    if (!consume(parser, TOKEN_SEMICOLON)) { AST_free(init); return NULL; }

    ASTNode* cond = parse_expression(parser);
    if (!cond) { AST_free(init); return NULL; }

    if (!consume(parser, TOKEN_SEMICOLON)) { AST_free(init); AST_free(cond); return NULL; }

    ASTNode* post = parse_expression(parser);
    if (!post) { AST_free(init); AST_free(cond); return NULL; }

    if (!consume(parser, TOKEN_RPAREN)) { AST_free(init); AST_free(cond); AST_free(post); return NULL; }

    ASTNode* block = parse_block(parser);
    if (!block) { AST_free(init); AST_free(cond); AST_free(post); return NULL; }

    ASTNode* for_node = AST_create_node(NODE_FOR, line);
    for_node->as._for.init = init;
    for_node->as._for.cond = cond;
    for_node->as._for.post = post;
    for_node->as._for.block = block;
    return for_node;
}

static ASTNode* parse_import(Parser* parser) {
    const int line = parser->cur_token->line;
    free_token(advance(parser));
    ASTNode* node = AST_create_node(NODE_IMPORT, line);

    if (parser->cur_token->type == TOKEN_STRING_LIT) {
        Token* path_token = advance(parser);
        node->as.imprt.path = path_token->value;
        node->as.imprt.alias = NULL;
        free(path_token);
        return node;
    }
    if (parser->cur_token->type == TOKEN_IDENT) {
        Token* alias_token = advance(parser);
        char* alias = alias_token->value;
        free(alias_token);
        if (consume(parser, TOKEN_FROM)) {
            Token* path_token = expect(parser, TOKEN_STRING_LIT);
            if (path_token) {
                node->as.imprt.path = path_token->value;
                node->as.imprt.alias = alias;
                free(path_token);
                return node;
            }
        }
        free(alias);
    }
    AST_free(node);
    return NULL;
}

static ASTNode* parse_struct(Parser* parser) {
    const int line = parser->cur_token->line;
    free_token(advance(parser));

    Token* ident_token = expect(parser, TOKEN_IDENT);
    if (!ident_token) return NULL;
    char* ident_name = ident_token->value;
    free(ident_token);

    if (!consume(parser, TOKEN_LBRACE)) { free(ident_name); return NULL; }

    List* field_decls = LIST_create(NULL, free_f, NULL);
    while (parser->cur_token->type != TOKEN_RBRACE &&
           parser->cur_token->type != TOKEN_EOF) {
        skip_token(parser, TOKEN_NEWLINE, -1);
        if (parser->cur_token->type == TOKEN_RBRACE) break;

        Token* type_token = advance(parser);
        char* type_name = type_token->value;
        const int f_line = type_token->line;
        free(type_token);

        Token* field_token = expect(parser, TOKEN_IDENT);
        if (!field_token) {
            LIST_free(field_decls);
            free(type_name);
            free(ident_name);
            return NULL;
        }
        char* field_name = field_token->value;
        free(field_token);

        LIST_append(field_decls, make_var_decl(f_line, type_name, field_name));
        skip_token(parser, TOKEN_COMMA, 1);

        if (parser->cur_token->type == TOKEN_RBRACE ||
            parser->cur_token->type == TOKEN_NEWLINE) continue;
        LIST_free(field_decls);
        free(ident_name);
        return NULL;
    }

    if (!consume(parser, TOKEN_RBRACE)) { free(ident_name); LIST_free(field_decls); return NULL; }

    ASTNode* struct_node = AST_create_node(NODE_STRUCT_DECL, line);
    struct_node->as._struct.name = ident_name;
    struct_node->as._struct.field_decls = field_decls;
    return struct_node;
}

static ASTNode* parse_ref(Parser* parser) {
    const int line = parser->cur_token->line;
    free_token(advance(parser));

    Token* name_token = expect(parser, TOKEN_IDENT);
    if (!name_token) return NULL;
    char* name = name_token->value;
    free(name_token);

    if (!consume(parser, TOKEN_LBRACE)) { free(name); return NULL; }

    List* fields = LIST_create(NULL, free_f, NULL);
    List* methods = LIST_create(NULL, free_f, NULL);

    while (parser->cur_token->type != TOKEN_RBRACE &&
           parser->cur_token->type != TOKEN_EOF) {
        skip_token(parser, TOKEN_NEWLINE, -1);
        if (parser->cur_token->type == TOKEN_RBRACE) break;

        const TokenType ct = parser->cur_token->type;

        if (ct == TOKEN_FREE) {
            const int d_line = parser->cur_token->line;
            free_token(advance(parser));
            ASTNode* body = parse_block(parser);
            if (!body) { free(name); LIST_free(fields); LIST_free(methods); return NULL; }
            ASTNode* destructor = AST_create_node(NODE_FUNC_DECL, d_line);
            destructor->as.f_dec.name = strdup("__free__");
            destructor->as.f_dec.var_decls = LIST_create(NULL, free_f, NULL);
            destructor->as.f_dec.ret_types = LIST_create(NULL, free_f, NULL);
            destructor->as.f_dec.block = body;
            destructor->as.f_dec.is_pub = 0;
            destructor->as.f_dec.is_static = 0;
            LIST_append(methods, destructor);

        } else if (ct == TOKEN_FN || ct == TOKEN_PUB || ct == TOKEN_STATIC) {
            ASTNode* method = parse_func_decl(parser);
            if (!method) { free(name); LIST_free(fields); LIST_free(methods); return NULL; }
            LIST_append(methods, method);

        } else if (ct == TOKEN_IDENT && parser->peek_token->type == TOKEN_LPAREN) {
            // constructor: TypeName(params) { block }
            const int c_line = parser->cur_token->line;
            free_token(advance(parser));
            if (!consume(parser, TOKEN_LPAREN)) { free(name); LIST_free(fields); LIST_free(methods); return NULL; }

            List* params = LIST_create(NULL, free_f, NULL);
            while (parser->cur_token->type != TOKEN_RPAREN &&
                   parser->cur_token->type != TOKEN_EOF) {
                skip_token(parser, TOKEN_NEWLINE, -1);
                if (parser->cur_token->type == TOKEN_RPAREN) break;

                if (!(is_type_keyword(parser->cur_token->type) || parser->cur_token->type == TOKEN_IDENT)) {
                    free(name); LIST_free(fields); LIST_free(methods); LIST_free(params); return NULL;
                }
                Token* p_type_token = advance(parser);
                char* p_type = p_type_token->value;
                const int p_line = p_type_token->line;
                free(p_type_token);

                Token* p_ident = expect(parser, TOKEN_IDENT);
                if (!p_ident) { free(p_type); free(name); LIST_free(fields); LIST_free(methods); LIST_free(params); return NULL; }
                char* p_name = p_ident->value;
                free(p_ident);

                LIST_append(params, make_var_decl(p_line, p_type, p_name));
                skip_token(parser, TOKEN_COMMA, 1);
            }

            if (!consume(parser, TOKEN_RPAREN)) { free(name); LIST_free(fields); LIST_free(methods); LIST_free(params); return NULL; }

            ASTNode* body = parse_block(parser);
            if (!body) { free(name); LIST_free(fields); LIST_free(methods); LIST_free(params); return NULL; }

            ASTNode* constructor = AST_create_node(NODE_FUNC_DECL, c_line);
            constructor->as.f_dec.name = strdup("__init__");
            constructor->as.f_dec.var_decls = params;
            constructor->as.f_dec.ret_types = LIST_create(NULL, free_f, NULL);
            constructor->as.f_dec.block = body;
            constructor->as.f_dec.is_pub = 1;
            constructor->as.f_dec.is_static = 0;
            LIST_append(methods, constructor);

        } else if (is_type_keyword(ct) || ct == TOKEN_IDENT) {
            // field: type name
            Token* type_token = advance(parser);
            char* f_type = type_token->value;
            const int f_line = type_token->line;
            free(type_token);

            Token* f_ident = expect(parser, TOKEN_IDENT);
            if (!f_ident) { free(f_type); free(name); LIST_free(fields); LIST_free(methods); return NULL; }
            char* f_name = f_ident->value;
            free(f_ident);

            LIST_append(fields, make_var_decl(f_line, f_type, f_name));
            skip_token(parser, TOKEN_COMMA, 1);

        } else {
            free(name); LIST_free(fields); LIST_free(methods); return NULL;
        }
    }

    if (!consume(parser, TOKEN_RBRACE)) { free(name); LIST_free(fields); LIST_free(methods); return NULL; }

    ASTNode* node = AST_create_node(NODE_TYPE_DECL, line);
    node->as.type_decl.name = name;
    node->as.type_decl.fields = fields;
    node->as.type_decl.methods = methods;
    return node;
}

static ASTNode* parse_enum(Parser* parser) {
    const int line = parser->cur_token->line;
    free_token(advance(parser));

    Token* e_name_token = expect(parser, TOKEN_IDENT);
    if (!e_name_token) return NULL;
    char* e_name = e_name_token->value;
    free(e_name_token);

    if (!consume(parser, TOKEN_LBRACE)) { free(e_name); return NULL; }

    List* variants = LIST_create(NULL, free, NULL);
    while (parser->cur_token->type != TOKEN_RBRACE &&
           parser->cur_token->type != TOKEN_EOF) {
        skip_token(parser, TOKEN_NEWLINE, -1);
        if (parser->cur_token->type == TOKEN_RBRACE) break;

        Token* v_token = expect(parser, TOKEN_IDENT);
        if (!v_token) { free(e_name); LIST_free(variants); return NULL; }
        char* v_name = v_token->value;
        free(v_token);
        LIST_append(variants, v_name);

        if (parser->cur_token->type == TOKEN_LPAREN) {
            while (parser->cur_token->type != TOKEN_RPAREN &&
                   parser->cur_token->type != TOKEN_EOF)
                free_token(advance(parser));
            free_token(advance(parser));
        }

        skip_token(parser, TOKEN_COMMA, 1);
        if (parser->cur_token->type == TOKEN_RBRACE ||
            parser->cur_token->type == TOKEN_NEWLINE) continue;
        free(e_name);
        LIST_free(variants);
        return NULL;
    }

    if (!consume(parser, TOKEN_RBRACE)) { free(e_name); LIST_free(variants); return NULL; }

    ASTNode* enum_node = AST_create_node(NODE_ENUM_DECL, line);
    enum_node->as._enum.name = e_name;
    enum_node->as._enum.variants = variants;
    return enum_node;
}

static ASTNode* parse_return(Parser* parser) {
    const int line = parser->cur_token->line;
    free_token(advance(parser));
    ASTNode* expr = parse_expression(parser);
    ASTNode* node = AST_create_node(NODE_RETURN_STM, line);
    node->as.rtrn.stmt = expr;
    return node;
}

static ASTNode* parse_switch(Parser* parser) {
    const int line = parser->cur_token->line;
    free_token(advance(parser));

    if (!consume(parser, TOKEN_LPAREN)) return NULL;

    ASTNode* expr = parse_expression(parser);
    if (!expr) return NULL;

    if (!consume(parser, TOKEN_RPAREN)) { AST_free(expr); return NULL; }
    if (!consume(parser, TOKEN_LBRACE)) { AST_free(expr); return NULL; }

    List* cases = LIST_create(NULL, free_f, NULL);
    while (parser->cur_token->type != TOKEN_RBRACE &&
           parser->cur_token->type != TOKEN_EOF) {
        skip_token(parser, TOKEN_NEWLINE, -1);
        if (parser->cur_token->type == TOKEN_RBRACE) break;

        if (parser->cur_token->type == TOKEN_CASE) {
            const int c_line = parser->cur_token->line;
            free_token(advance(parser));

            ASTNode* c_expr = parse_expression(parser);
            if (!c_expr) { AST_free(expr); LIST_free(cases); return NULL; }

            ASTNode* body = parse_block(parser);
            if (!body) { AST_free(c_expr); AST_free(expr); LIST_free(cases); return NULL; }

            ASTNode* node_case = AST_create_node(NODE_CASE, c_line);
            node_case->as._case.is_default = 0;
            node_case->as._case.expr = c_expr;
            node_case->as._case.block = body;
            LIST_append(cases, node_case);
        } else if (parser->cur_token->type == TOKEN_DEFAULT) {
            const int d_line = parser->cur_token->line;
            free_token(advance(parser));

            ASTNode* body = parse_block(parser);
            if (!body) { AST_free(expr); LIST_free(cases); return NULL; }

            ASTNode* default_node = AST_create_node(NODE_CASE, d_line);
            default_node->as._case.is_default = 1;
            default_node->as._case.expr = NULL;
            default_node->as._case.block = body;
            LIST_append(cases, default_node);
        } else {
            AST_free(expr);
            LIST_free(cases);
            return NULL;
        }
    }

    if (!consume(parser, TOKEN_RBRACE)) { LIST_free(cases); AST_free(expr); return NULL; }

    ASTNode* block = AST_create_node(NODE_BLOCK, line);
    block->as.block.stmts = cases;

    ASTNode* switch_node = AST_create_node(NODE_SWITCH, line);
    switch_node->as._switch.expr = expr;
    switch_node->as._switch.block = block;
    return switch_node;
}

static int is_type_keyword(const TokenType type) {
    switch (type) {
        case TOKEN_INT: case TOKEN_I8: case TOKEN_I16: case TOKEN_I32: case TOKEN_I64:
        case TOKEN_U8:  case TOKEN_U16: case TOKEN_U32: case TOKEN_U64:
        case TOKEN_FLOAT: case TOKEN_F32: case TOKEN_F64:
        case TOKEN_BOOL: case TOKEN_CHAR: case TOKEN_BYTE: case TOKEN_STRING:
        case TOKEN_VOID: return 1;
        default: return 0;
    }
}

static ASTNode* parse_var_decl(Parser* parser) {
    const int line = parser->cur_token->line;
    int is_const = 0;
    if (parser->cur_token->type == TOKEN_CONST) {
        is_const = 1;
        free_token(advance(parser));
    }
    const int is_type = is_type_keyword(parser->cur_token->type);
    const int is_ident = parser->cur_token->type == TOKEN_IDENT;
    char* type_name = NULL;
    if (is_type) {
        Token* t = advance(parser);
        type_name = t->value;
        free(t);
    } else if (is_ident && parser->peek_token->type == TOKEN_DECLARE) {
        type_name = NULL;
    } else if (is_ident) {
        Token* t = advance(parser);
        type_name = t->value;
        free(t);
    } else return NULL;

    Token* ident_token = expect(parser, TOKEN_IDENT);
    if (!ident_token) { free(type_name); return NULL; }
    char* ident_name = ident_token->value;
    free(ident_token);

    Token* dec_ass = advance(parser);
    if (dec_ass->type != TOKEN_DECLARE && dec_ass->type != TOKEN_ASSIGN) {
        free_token(dec_ass);
        free(type_name);
        free(ident_name);
        return NULL;
    }
    free_token(dec_ass);

    ASTNode* expr = parse_expression(parser);
    if (!expr) { free(type_name); free(ident_name); return NULL; }

    ASTNode* var_decl = AST_create_node(NODE_VAR_DECL, line);
    var_decl->as.dec_ass.type_name = type_name;
    var_decl->as.dec_ass.name = ident_name;
    var_decl->as.dec_ass.is_const = is_const;
    var_decl->as.dec_ass.stmt = expr;
    return var_decl;
}

static ASTNode* parse_var_assign(Parser* parser) {
    Token* ident_token = advance(parser);
    const int line = ident_token->line;
    char* ident_name = ident_token->value;
    free(ident_token);

    if (!consume(parser, TOKEN_ASSIGN)) { free(ident_name); return NULL; }

    ASTNode* expr = parse_expression(parser);
    if (!expr) { free(ident_name); return NULL; }

    ASTNode* var_assign = AST_create_node(NODE_VAR_ASSIGN, line);
    var_assign->as.dec_ass.name = ident_name;
    var_assign->as.dec_ass.stmt = expr;
    return var_assign;
}

static ASTNode* parse_func_decl(Parser* parser) {
    const int line = parser->cur_token->line;
    int is_pub = 0, is_static = 0;

    if (parser->cur_token->type == TOKEN_PUB) { is_pub = 1; free_token(advance(parser)); }
    if (parser->cur_token->type == TOKEN_STATIC) { is_static = 1; free_token(advance(parser)); }
    if (!consume(parser, TOKEN_FN)) return NULL;

    Token* ident_token = expect(parser, TOKEN_IDENT);
    if (!ident_token) return NULL;
    char* ident_name = ident_token->value;
    free(ident_token);

    if (!consume(parser, TOKEN_LPAREN)) { free(ident_name); return NULL; }

    List* params = LIST_create(NULL, free_f, NULL);
    while (parser->cur_token->type != TOKEN_RPAREN && parser->cur_token->type != TOKEN_EOF) {
        skip_token(parser, TOKEN_NEWLINE, -1);
        if (parser->cur_token->type == TOKEN_RPAREN) break;

        if (!(is_type_keyword(parser->cur_token->type) || parser->cur_token->type == TOKEN_IDENT)) {
            free(ident_name); LIST_free(params); return NULL;
        }
        Token* type_token = advance(parser);
        const int p_line = type_token->line;
        char* p_type = type_token->value;
        free(type_token);

        Token* p_ident = expect(parser, TOKEN_IDENT);
        if (!p_ident) { free(p_type); free(ident_name); LIST_free(params); return NULL; }
        char* p_name = p_ident->value;
        free(p_ident);

        LIST_append(params, make_var_decl(p_line, p_type, p_name));
        skip_token(parser, TOKEN_COMMA, 1);
    }

    if (!consume(parser, TOKEN_RPAREN)) { free(ident_name); LIST_free(params); return NULL; }

    List* ret_types = LIST_create(NULL, free_f, NULL);
    if (parser->cur_token->type == TOKEN_ARROW) {
        free_token(advance(parser));

        if (parser->cur_token->type == TOKEN_LPAREN) {
            free_token(advance(parser));

            while (parser->cur_token->type != TOKEN_RPAREN && parser->cur_token->type != TOKEN_EOF) {
                skip_token(parser, TOKEN_NEWLINE, -1);
                if (parser->cur_token->type == TOKEN_RPAREN) break;

                if (!(is_type_keyword(parser->cur_token->type) || parser->cur_token->type == TOKEN_IDENT)) {
                    LIST_free(ret_types); free(ident_name); LIST_free(params); return NULL;
                }
                Token* r_type_token = advance(parser);
                const int r_line = r_type_token->line;
                char* r_type = r_type_token->value;
                free(r_type_token);

                char* r_name = NULL;
                if (parser->cur_token->type == TOKEN_COLON) {
                    skip_token(parser, TOKEN_COLON, 1);
                    Token* r_ident = expect(parser, TOKEN_IDENT);
                    if (!r_ident) {
                        free(r_type); LIST_free(ret_types); free(ident_name); LIST_free(params); return NULL;
                    }
                    r_name = r_ident->value;
                    free(r_ident);
                }

                LIST_append(ret_types, make_var_decl(r_line, r_type, r_name));
                skip_token(parser, TOKEN_COMMA, 1);
            }

            if (!consume(parser, TOKEN_RPAREN)) {
                free(ident_name); LIST_free(params); LIST_free(ret_types); return NULL;
            }
        } else {
            if (!(is_type_keyword(parser->cur_token->type) || parser->cur_token->type == TOKEN_IDENT)) {
                free(ident_name); LIST_free(params); LIST_free(ret_types); return NULL;
            }
            Token* r_type_token = advance(parser);
            LIST_append(ret_types, make_var_decl(r_type_token->line, r_type_token->value, NULL));
            free(r_type_token);
        }
    }

    ASTNode* body = parse_block(parser);
    if (!body) { free(ident_name); LIST_free(params); LIST_free(ret_types); return NULL; }

    ASTNode* func_decl = AST_create_node(NODE_FUNC_DECL, line);
    func_decl->as.f_dec.name = ident_name;
    func_decl->as.f_dec.block = body;
    func_decl->as.f_dec.var_decls = params;
    func_decl->as.f_dec.is_pub = is_pub;
    func_decl->as.f_dec.is_static = is_static;
    func_decl->as.f_dec.ret_types = ret_types;
    return func_decl;
}

static ASTNode* parse_expr_stmt(Parser* parser) {
    ASTNode* lhs = parse_expression(parser);
    if (!lhs) return NULL;

    if (parser->cur_token->type != TOKEN_ASSIGN) return lhs;

    free_token(advance(parser));
    ASTNode* rhs = parse_expression(parser);
    if (!rhs) { AST_free(lhs); return NULL; }

    if (lhs->type == NODE_FLD_ACC) {
        ASTNode* node = AST_create_node(NODE_FLD_SET, lhs->line);
        node->as.fld_set.obj        = lhs->as.fld_acc.obj;
        node->as.fld_set.field_name = lhs->as.fld_acc.field_name;
        node->as.fld_set.value      = rhs;
        free(lhs);
        return node;
    }
    if (lhs->type == NODE_IDX_ACC) {
        ASTNode* node = AST_create_node(NODE_IDX_SET, lhs->line);
        node->as.idx_set.obj   = lhs->as.idx_acc.obj;
        node->as.idx_set.index = lhs->as.idx_acc.index;
        node->as.idx_set.value = rhs;
        free(lhs);
        return node;
    }

    printf("Error: invalid assignment target on line %d\n", lhs->line);
    AST_free(lhs);
    AST_free(rhs);
    return NULL;
}

static ASTNode* parse_statement(Parser* parser) {
    switch (parser->cur_token->type) {
        case TOKEN_IF:     return parse_if(parser);
        case TOKEN_WHILE:  return parse_while(parser);
        case TOKEN_FOR:    return parse_for(parser);
        case TOKEN_IMPORT: return parse_import(parser);
        case TOKEN_STRUCT: return parse_struct(parser);
        case TOKEN_REF:    return parse_ref(parser);
        case TOKEN_ENUM:   return parse_enum(parser);
        case TOKEN_RETURN: return parse_return(parser);
        case TOKEN_SWITCH: return parse_switch(parser);
        case TOKEN_PUB:
        case TOKEN_STATIC:
        case TOKEN_FN:     return parse_func_decl(parser);
        case TOKEN_BREAK: {
            ASTNode* node = AST_create_node(NODE_BRK, parser->cur_token->line);
            free_token(advance(parser));
            return node;
        }
        case TOKEN_CONTINUE: {
            ASTNode* node = AST_create_node(NODE_CONT, parser->cur_token->line);
            free_token(advance(parser));
            return node;
        }
        case TOKEN_NEWLINE:
            free_token(advance(parser));
            return NULL;
        case TOKEN_CONST: return parse_var_decl(parser);
        case TOKEN_THIS:
        case TOKEN_IDENT: {
            if (parser->peek_token->type == TOKEN_DECLARE) return parse_var_decl(parser);
            if (parser->peek_token->type == TOKEN_ASSIGN)  return parse_var_assign(parser);
            return parse_expr_stmt(parser);
        }
        case TOKEN_INT: case TOKEN_I8:  case TOKEN_I16: case TOKEN_I32: case TOKEN_I64:
        case TOKEN_U8:  case TOKEN_U16: case TOKEN_U32: case TOKEN_U64:
        case TOKEN_FLOAT: case TOKEN_F32: case TOKEN_F64:
        case TOKEN_BOOL: case TOKEN_CHAR: case TOKEN_BYTE: case TOKEN_STRING:
        case TOKEN_VOID: return parse_var_decl(parser);
        default:
            printf("Syntax error");
            free_token(advance(parser));
            return NULL;
    }
}

static ASTNode* parse_block(Parser* parser) {
    Token* lbrace = expect(parser, TOKEN_LBRACE);
    if (!lbrace) return NULL;
    ASTNode* block = AST_create_node(NODE_BLOCK, lbrace->line);
    free_token(lbrace);

    List* stmts = LIST_create(NULL, free_f, NULL);
    while (parser->cur_token->type != TOKEN_RBRACE &&
           parser->cur_token->type != TOKEN_EOF) {
        ASTNode* st = parse_statement(parser);
        if (st) LIST_append(stmts, st);
    }
    block->as.block.stmts = stmts;

    if (!consume(parser, TOKEN_RBRACE)) { AST_free(block); return NULL; }
    return block;
}

List* PARSER_parse(Parser* parser) {
    List* list = LIST_create(NULL, free_f, NULL);
    while (parser->cur_token->type != TOKEN_EOF) {
        ASTNode* node = parse_statement(parser);
        if (node) LIST_append(list, node);
    }
    return list;
}
