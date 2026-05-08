#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
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
    if (parser->cur_token->type == exp_type) {
        Token* token = advance(parser);
        return token;
    }
    printf("Expected token type ");
    print_token_type(exp_type);
    printf(", but got token type ");
    print_token_type(parser->cur_token->type);
    printf(" on line %d", parser->cur_token->line);
    return NULL;
}

static void free_f(void* data) {
    AST_free(data);
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
            Token* token = advance(parser);
            ASTNode* node = AST_create_node(NODE_INT_LIT, token->line);
            node->as.int_val = atoi(token->value);
            free(token->value);
            free(token);
            left = node;
            break;
        }
        case TOKEN_FLOAT_LIT: {
            Token* token = advance(parser);
            ASTNode* node = AST_create_node(NODE_FLOAT_LIT, token->line);
            node->as.float_val = atof(token->value);
            free(token->value);
            free(token);
            left = node;
            break;
        }
        case TOKEN_STRING_LIT: {
            Token* token = advance(parser);
            ASTNode* node = AST_create_node(NODE_STR_LIT, token->line);
            node->as.str_val = token->value;
            free(token);
            left = node;
            break;
        }
        case TOKEN_CHAR_LIT: {
            Token* token = advance(parser);
            ASTNode* node = AST_create_node(NODE_CHAR_LIT, token->line);
            node->as.str_val = token->value;
            free(token);
            left = node;
            break;
        }
        case TOKEN_IDENT: {
            Token* token = advance(parser);
            ASTNode* node = AST_create_node(NODE_IDENT, token->line);
            node->as.str_val = token->value;
            free(token);
            left = node;
            break;
        }
        case TOKEN_TRUE: {
            Token* token = advance(parser);
            ASTNode* node = AST_create_node(NODE_BOOL_LIT, token->line);
            node->as.int_val = 1;
            free(token->value);
            free(token);
            left = node;
            break;
        }
        case TOKEN_FALSE: {
            Token* token = advance(parser);
            ASTNode* node = AST_create_node(NODE_BOOL_LIT, token->line);
            node->as.int_val = 0;
            free(token->value);
            free(token);
            left = node;
            break;
        }
        case TOKEN_NULL: {
            Token* token = advance(parser);
            ASTNode* node = AST_create_node(NODE_NULL_LIT, token->line);
            free(token->value);
            free(token);
            left = node;
            break;
        }
        case TOKEN_MINUS:
        case TOKEN_NOT: {
            Token* token = advance(parser);
            char* op = token->value;
            ASTNode* stmt = parse_expr(parser, 6);
            ASTNode* node = AST_create_node(NODE_UNOP, token->line);
            free(token);
            node->as.unop.op = op;
            node->as.unop.stmt = stmt;
            left = node;
            break;
        }
        case TOKEN_LPAREN: {
            Token* lparen = advance(parser);
            free(lparen->value);
            free(lparen);
            ASTNode* expr = parse_expr(parser, 0);
            Token* rparen = expect(parser, TOKEN_RPAREN);
            if (rparen) {
                free(rparen->value);
                free(rparen);
                left = expr;
                break;
            }
            if (expr) AST_free(expr);
            return NULL;
        }
        default: return NULL;
    }
    if (!left) return NULL;

    while (get_precedence(parser->cur_token->type) > min_prec) {
        const int prec = get_precedence(parser->cur_token->type);
        Token* op_token = advance(parser);
        char* op = op_token->value;
        ASTNode* right = parse_expr(parser, prec);
        ASTNode* binop = AST_create_node(NODE_BINOP, op_token->line);
        free(op_token);
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

static ASTNode* parse_if(Parser* parser) {
    Token* token_if = advance(parser);
    int line = token_if->line;
    free(token_if->value);
    free(token_if);

    Token* lparen = expect(parser, TOKEN_LPAREN);
    if (!lparen) return NULL;
    free(lparen->value);
    free(lparen);

    ASTNode* cond = parse_expression(parser);
    if (!cond) return NULL;

    Token* rparen = expect(parser, TOKEN_RPAREN);
    if (!rparen) {
        AST_free(cond);
        return NULL;
    }
    free(rparen->value);
    free(rparen);

    ASTNode* block = parse_block(parser);
    if (!block) {
        AST_free(cond);
        return NULL;
    }

    ASTNode* if_else = AST_create_node(NODE_IF_ELSE, line);
    if_else->as.if_else.stmt = cond;
    if_else->as.if_else.block1 = block;


    while (parser->cur_token->type == TOKEN_NEWLINE) {
        Token* nl = advance(parser);
        free(nl->value);
        free(nl);
    }
    if (parser->cur_token->type == TOKEN_ELSE) {
        Token* else_token = advance(parser);
        free(else_token->value);
        free(else_token);
        if (parser->cur_token->type == TOKEN_IF) {
            if_else->as.if_else.block2 = parse_if(parser);
        } else {
            if_else->as.if_else.block2 = parse_block(parser);
        }
    } else {
        if_else->as.if_else.block2 = NULL;
    }
    return if_else;
}

static ASTNode* parse_while(Parser* parser) {
    Token* while_token = advance(parser);
    int line = while_token->line;
    free(while_token->value);
    free(while_token);

    Token* lparen = expect(parser, TOKEN_LPAREN);
    if (!lparen) return NULL;
    free(lparen->value);
    free(lparen);

    ASTNode* expr = parse_expression(parser);
    if (!expr) return NULL;

    Token* rparen = expect(parser, TOKEN_RPAREN);
    if (!rparen) {
        AST_free(expr);
        return NULL;
    }
    free(rparen->value);
    free(rparen);

    ASTNode* block = parse_block(parser);
    if (!block) {
        AST_free(expr);
        return NULL;
    }

    ASTNode* while_node = AST_create_node(NODE_WHILE, line);
    while_node->as._while.stmt = expr;
    while_node->as._while.block = block;

    return while_node;
}

static ASTNode* parse_statement(Parser* parser);

static ASTNode* parse_for(Parser* parser) {
    Token* for_token = advance(parser);
    int line = for_token->line;
    free(for_token->value);
    free(for_token);

    Token* lparen = expect(parser, TOKEN_LPAREN);
    if (!lparen) return NULL;
    free(lparen->value);
    free(lparen);

    const TokenType cur_ttype = parser->cur_token->type;
    if (cur_ttype == TOKEN_LPAREN) {
        Token* lparen_token = advance(parser);
        free(lparen_token->value);
        free(lparen_token);

        List* vnames = LIST_create(NULL, free,NULL);
        while (parser->cur_token->type != TOKEN_RPAREN &&
               parser->cur_token->type != TOKEN_EOF) {
            Token* idf_token = expect(parser, TOKEN_IDENT);
            if (!idf_token) {
                LIST_free(vnames);
                return NULL;
            }
            LIST_append(vnames, idf_token->value);
            free(idf_token);

            Token* comma = expect(parser, TOKEN_COMMA);
            if (!comma) {
                if (parser->cur_token->type == TOKEN_RPAREN)
                    break;
                LIST_free(vnames);
                return NULL;
            }
            free(comma->value);
            free(comma);
        }

        Token* rparen = advance(parser);
        free(rparen->value);
        free(rparen);

        Token* in_token = expect(parser, TOKEN_IN);
        if (!in_token) {
            LIST_free(vnames);
            return NULL;
        }
        free(in_token->value);
        free(in_token);

        ASTNode* coll = parse_expression(parser);
        if (!coll) {
            LIST_free(vnames);
            return NULL;
        }

        rparen = expect(parser, TOKEN_RPAREN);
        if (!rparen) {
            AST_free(coll);
            LIST_free(vnames);
            return NULL;
        }
        free(rparen->value);
        free(rparen);

        ASTNode* block = parse_block(parser);
        if (!block) {
            AST_free(coll);
            LIST_free(vnames);
            return NULL;
        }

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

        Token* in_token = expect(parser, TOKEN_IN);
        if (!in_token) {
            free(idf_name);
            return NULL;
        }
        free(in_token->value);
        free(in_token);

        ASTNode* coll = parse_expression(parser);
        if (!coll) {
            free(idf_name);
            return NULL;
        }

        Token* rparen = expect(parser, TOKEN_RPAREN);
        if (!rparen) {
            AST_free(coll);
            free(idf_name);
            return NULL;
        }
        free(rparen->value);
        free(rparen);

        ASTNode* block = parse_block(parser);
        if (!block) {
            AST_free(coll);
            free(idf_name);
            return NULL;
        }

        ASTNode* foreach_node = AST_create_node(NODE_FOREACH, line);
        List* vnames = LIST_create(NULL, free,NULL);
        LIST_append(vnames, idf_name);
        foreach_node->as.foreach.vnames = vnames;
        foreach_node->as.foreach.expression = coll;
        foreach_node->as.foreach.block = block;
        return foreach_node;
    }
    ASTNode* init = parse_statement(parser);
    if (!init) return NULL;

    Token* semicolon = expect(parser, TOKEN_SEMICOLON);
    if (!semicolon) {
        AST_free(init);
        return NULL;
    }
    free(semicolon->value);
    free(semicolon);

    ASTNode* cond = parse_expression(parser);
    if (!cond) {
        AST_free(init);
        return NULL;
    }

    semicolon = expect(parser, TOKEN_SEMICOLON);
    if (!semicolon) {
        AST_free(init);
        AST_free(cond);
        return NULL;
    }
    free(semicolon->value);
    free(semicolon);

    ASTNode* post = parse_expression(parser);
    if (!post) {
        AST_free(init);
        AST_free(cond);
        return NULL;
    }

    Token* rparen = expect(parser, TOKEN_RPAREN);
    if (!rparen) {
        AST_free(init);
        AST_free(cond);
        AST_free(post);
        return NULL;
    }
    free(rparen->value);
    free(rparen);

    ASTNode* block = parse_block(parser);
    if (!block) {
        AST_free(init);
        AST_free(cond);
        AST_free(post);
        return NULL;
    }

    ASTNode* for_node = AST_create_node(NODE_FOR, line);
    for_node->as._for.init = init;
    for_node->as._for.cond = cond;
    for_node->as._for.post = post;
    for_node->as._for.block = block;
    return for_node;
}


static ASTNode* parse_import(Parser* parser) {
    Token* imp_token = advance(parser);
    ASTNode* node = AST_create_node(NODE_IMPORT, imp_token->line);
    free(imp_token->value);
    free(imp_token);
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
        Token* from_token = expect(parser, TOKEN_FROM);
        if (from_token) {
            free(from_token->value);
            free(from_token);
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
    Token* struct_token = advance(parser);
    const int line = struct_token->line;
    free(struct_token->value);
    free(struct_token);

    Token* ident_token = expect(parser, TOKEN_IDENT);
    if (!ident_token) return NULL;
    char* ident_name = ident_token->value;
    free(ident_token);

    Token* lbrace_token = expect(parser, TOKEN_LBRACE);
    if (!lbrace_token) {
        free(ident_name);
        return NULL;
    }
    free(lbrace_token->value);
    free(lbrace_token);

    List* field_decls = LIST_create(NULL, free_f,NULL);
    while (parser->cur_token->type != TOKEN_RBRACE &&
           parser->cur_token->type != TOKEN_EOF) {
        while (parser->cur_token->type == TOKEN_NEWLINE) {
            Token* nl_token = advance(parser);
            free(nl_token->value);
            free(nl_token);
        }
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

        ASTNode* decl_node = AST_create_node(NODE_VAR_DECL, f_line);
        decl_node->as.dec_ass.stmt = NULL;
        decl_node->as.dec_ass.type_name = type_name;
        decl_node->as.dec_ass.name = field_name;
        LIST_append(field_decls, decl_node);
        if (parser->cur_token->type == TOKEN_COMMA) {
            Token* comma_token = advance(parser);
            free(comma_token->value);
            free(comma_token);
            continue;
        }
        if (parser->cur_token->type == TOKEN_RBRACE ||
            parser->cur_token->type == TOKEN_NEWLINE) {
            continue;
        }
        LIST_free(field_decls);
        free(ident_name);
        return NULL;
    }
    Token* rbrace_token = expect(parser, TOKEN_RBRACE);
    if (!rbrace_token) {
        free(ident_name);
        LIST_free(field_decls);
        return NULL;
    }
    free(rbrace_token->value);
    free(rbrace_token);

    ASTNode* struct_node = AST_create_node(NODE_STRUCT_DECL, line);
    struct_node->as._struct.name = ident_name;
    struct_node->as._struct.field_decls = field_decls;
    return struct_node;
}

static ASTNode* parse_ref(Parser* parser) {
    return NULL;
}

static ASTNode* parse_enum(Parser* parser) {
    Token* enum_token = advance(parser);
    const int line = enum_token->line;
    free(enum_token->value);
    free(enum_token);

    Token* e_name_token = expect(parser, TOKEN_IDENT);
    if (!e_name_token) return NULL;
    char* e_name = e_name_token->value;
    free(e_name_token);

    Token* lbrace_token = expect(parser, TOKEN_LBRACE);
    if (!lbrace_token) {
        free(e_name);
        return NULL;
    }
    free(lbrace_token->value);
    free(lbrace_token);

    List* variants = LIST_create(NULL, free,NULL);
    while (parser->cur_token->type != TOKEN_RBRACE &&
           parser->cur_token->type != TOKEN_EOF) {
        while (parser->cur_token->type == TOKEN_NEWLINE) {
            Token* nl_token = advance(parser);
            free(nl_token->value);
            free(nl_token);
        }
        if (parser->cur_token->type == TOKEN_RBRACE) break;

        Token* v_token = expect(parser, TOKEN_IDENT);
        if (!v_token) {
            free(e_name);
            LIST_free(variants);
            return NULL;
        }
        char* v_name = v_token->value;
        free(v_token);
        LIST_append(variants, v_name);

        if (parser->cur_token->type == TOKEN_LPAREN) {
            while (parser->cur_token->type != TOKEN_RPAREN &&
                   parser->cur_token->type != TOKEN_EOF) {
                Token* token = advance(parser);
                free(token->value);
                free(token);
                // TODO  just ignoring for now
            }
            Token* rparen = advance(parser);
            free(rparen->value);
            free(rparen);
        }
        if (parser->cur_token->type == TOKEN_COMMA) {
            Token* comma_token = advance(parser);
            free(comma_token->value);
            free(comma_token);
            continue;
        }
        if (parser->cur_token->type == TOKEN_RBRACE ||
            parser->cur_token->type == TOKEN_NEWLINE) {
            continue;
        }
        free(e_name);
        LIST_free(variants);
        return NULL;
    }
    Token* brace = expect(parser, TOKEN_RBRACE);
    if (!brace) {
        free(e_name);
        LIST_free(variants);
        return NULL;
    }
    free(brace->value);
    free(brace);

    ASTNode* enum_node = AST_create_node(NODE_ENUM_DECL, line);
    enum_node->as._enum.name = e_name;
    enum_node->as._enum.variants = variants;
    return enum_node;
}

static ASTNode* parse_return(Parser* parser) {
    Token* token = advance(parser);
    ASTNode* expr = parse_expression(parser);
    ASTNode* node = AST_create_node(NODE_RETURN_STM, token->line);
    node->as.rtrn.stmt = expr;
    free(token->value);
    free(token);
    return node;
}

static ASTNode* parse_switch(Parser* parser) {
    Token* switch_token = advance(parser);
    const int line = switch_token->line;
    free(switch_token->value);
    free(switch_token);

    Token* lparen = expect(parser, TOKEN_LPAREN);
    if (!lparen) return NULL;
    free(lparen->value);
    free(lparen);

    ASTNode* expr = parse_expression(parser);
    if (!expr) return NULL;

    Token* rparen = expect(parser, TOKEN_RPAREN);
    if (!rparen) {
        AST_free(expr);
        return NULL;
    }
    free(rparen->value);
    free(rparen);

    Token* lbrace = expect(parser, TOKEN_LBRACE);
    if (!lbrace) {
        AST_free(expr);
        return NULL;
    }
    free(lbrace->value);
    free(lbrace);

    List* cases = LIST_create(NULL, free_f,NULL);
    while (parser->cur_token->type != TOKEN_RBRACE &&
           parser->cur_token->type != TOKEN_EOF) {
        while (parser->cur_token->type == TOKEN_NEWLINE) {
            Token* nl_token = advance(parser);
            free(nl_token->value);
            free(nl_token);
        }

        if (parser->cur_token->type == TOKEN_RBRACE) break;

        if (parser->cur_token->type == TOKEN_CASE) {
            Token* case_token = advance(parser);
            const int c_line = case_token->line;
            free(case_token->value);
            free(case_token);

            ASTNode* c_expr = parse_expression(parser);
            if (!c_expr) {
                AST_free(expr);
                LIST_free(cases);
                return NULL;
            }

            ASTNode* body = parse_block(parser);
            if (!body) {
                AST_free(c_expr);
                AST_free(expr);
                LIST_free(cases);
                return NULL;
            }

            ASTNode* node_case = AST_create_node(NODE_CASE, c_line);
            node_case->as._case.is_default = 0;
            node_case->as._case.expr = c_expr;
            node_case->as._case.block = body;
            LIST_append(cases, node_case);
        } else if (parser->cur_token->type == TOKEN_DEFAULT) {
            Token* default_token = advance(parser);
            const int d_line = default_token->line;
            free(default_token->value);
            free(default_token);

            ASTNode* body = parse_block(parser);
            if (!body) {
                AST_free(expr);
                LIST_free(cases);
                return NULL;
            }

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
    Token* rbrace = expect(parser, TOKEN_RBRACE);
    if (!rbrace) {
        LIST_free(cases);
        AST_free(expr);
        return NULL;
    }
    free(rbrace->value);
    free(rbrace);

    ASTNode* block = AST_create_node(NODE_BLOCK, line);
    block->as.block.stmts = cases;

    ASTNode* switch_node = AST_create_node(NODE_SWITCH, line);
    switch_node->as._switch.expr = expr;
    switch_node->as._switch.block = block;
    return switch_node;
}

static int is_type_keyword(const TokenType type) {
    switch (type) {
        case TOKEN_INT:
        case TOKEN_I8:
        case TOKEN_I16:
        case TOKEN_I32:
        case TOKEN_I64:
        case TOKEN_U8:
        case TOKEN_U16:
        case TOKEN_U32:
        case TOKEN_U64:
        case TOKEN_FLOAT:
        case TOKEN_F32:
        case TOKEN_F64:
        case TOKEN_BOOL:
        case TOKEN_CHAR:
        case TOKEN_BYTE:
        case TOKEN_STRING:
        case TOKEN_VOID: return 1;
        default: return 0;
    }
}

static ASTNode* parse_var_decl(Parser* parser) {
    const int line = parser->cur_token->line;
    int is_const = 0;
    if (parser->cur_token->type == TOKEN_CONST) {
        is_const = 1;
        Token* token = advance(parser);
        free(token->value);
        free(token);
    }
    const int is_type = is_type_keyword(parser->cur_token->type);
    const int is_ident = parser->cur_token->type == TOKEN_IDENT;
    char* type_name = NULL;
    if (is_type) {
        Token* type_token = advance(parser);
        type_name = type_token->value;
        free(type_token);
    } else if (is_ident && parser->peek_token->type == TOKEN_DECLARE) {
        type_name = NULL;
    } else if (is_ident) {
        Token* type_token = advance(parser);
        type_name = type_token->value;
        free(type_token);
    } else return NULL;

    Token* ident_token = expect(parser, TOKEN_IDENT);
    if (!ident_token) {
        free(type_name);
        return NULL;
    }
    char* ident_name = ident_token->value;
    free(ident_token);

    Token* dec_ass = advance(parser);
    if (dec_ass->type != TOKEN_DECLARE &&
        dec_ass->type != TOKEN_ASSIGN) {
        free(dec_ass->value);
        free(dec_ass);
        free(type_name);
        free(ident_name);
        return NULL;
    }
    free(dec_ass->value);
    free(dec_ass);

    ASTNode* expr = parse_expression(parser);
    if (!expr) {
        free(type_name);
        free(ident_name);
        return NULL;
    }

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

    Token* assign_token = expect(parser, TOKEN_ASSIGN);
    if (!assign_token) {
        free(ident_name);
        return NULL;
    }
    free(assign_token->value);
    free(assign_token);

    ASTNode* expr = parse_expression(parser);
    if (!expr) {
        free(ident_name);
        return NULL;
    }
    ASTNode* var_assign = AST_create_node(NODE_VAR_ASSIGN, line);
    var_assign->as.dec_ass.name = ident_name;
    var_assign->as.dec_ass.stmt = expr;
    var_assign->as.dec_ass.type_name = NULL;
    var_assign->as.dec_ass.is_const = 0;
    return var_assign;
}

static ASTNode* parse_statement(Parser* parser) {
    switch (parser->cur_token->type) {
        case TOKEN_IF: return parse_if(parser);
        case TOKEN_WHILE: return parse_while(parser);
        case TOKEN_FOR: return parse_for(parser);
        case TOKEN_IMPORT: return parse_import(parser);
        case TOKEN_STRUCT: return parse_struct(parser);
        case TOKEN_REF: return parse_ref(parser);
        case TOKEN_ENUM: return parse_enum(parser);
        case TOKEN_RETURN: return parse_return(parser);
        case TOKEN_SWITCH: return parse_switch(parser);
        case TOKEN_BREAK: {
            Token* token = advance(parser);
            ASTNode* node = AST_create_node(NODE_BRK, token->line);
            free(token->value);
            free(token);
            return node;
        }
        case TOKEN_CONTINUE: {
            Token* token = advance(parser);
            ASTNode* node = AST_create_node(NODE_CONT, token->line);
            free(token->value);
            free(token);
            return node;
        }
        case TOKEN_NEWLINE: {
            Token* token = advance(parser);
            free(token->value);
            free(token);
            return NULL;
        }
        case TOKEN_CONST: return parse_var_decl(parser);
        case TOKEN_IDENT: {
            if (parser->peek_token->type == TOKEN_DECLARE) return parse_var_decl(parser);
            if (parser->peek_token->type == TOKEN_ASSIGN) return parse_var_assign(parser);
            printf("Syntax error");
            Token* token = advance(parser);
            free(token->value);
            free(token);
            return NULL;
        }
        case TOKEN_INT:
        case TOKEN_I8:
        case TOKEN_I16:
        case TOKEN_I32:
        case TOKEN_I64:
        case TOKEN_U8:
        case TOKEN_U16:
        case TOKEN_U32:
        case TOKEN_U64:
        case TOKEN_FLOAT:
        case TOKEN_F32:
        case TOKEN_F64:
        case TOKEN_BOOL:
        case TOKEN_CHAR:
        case TOKEN_BYTE:
        case TOKEN_STRING:
        case TOKEN_VOID: return parse_var_decl(parser);
        default: {
            printf("Syntax error");
            Token* token = advance(parser);
            free(token->value);
            free(token);
            return NULL;
        }
    }
}

static ASTNode* parse_block(Parser* parser) {
    Token* lbrace = expect(parser, TOKEN_LBRACE);
    if (lbrace) {
        ASTNode* block = AST_create_node(NODE_BLOCK, lbrace->line);
        free(lbrace->value);
        free(lbrace);

        List* stmts = LIST_create(NULL, free_f,NULL);

        while (parser->cur_token->type != TOKEN_RBRACE
               && parser->cur_token->type != TOKEN_EOF) {
            ASTNode* st = parse_statement(parser);
            if (st) LIST_append(stmts, st);
        }
        block->as.block.stmts = stmts;
        Token* rbrace = expect(parser, TOKEN_RBRACE);
        if (rbrace) {
            free(rbrace->value);
            free(rbrace);
            return block;
        }
        AST_free(block);
    }
    return NULL;
}

List* PARSER_parse(Parser* parser) {
    List* list = LIST_create(NULL, free_f,NULL);
    while (parser->cur_token->type != TOKEN_EOF) {
        ASTNode* node = parse_statement(parser);
        if (node) LIST_append(list, node);
    }
    return list;
}
