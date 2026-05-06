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

static int get_precedence(TokenType type) {
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

static ASTNode* parse_expr(Parser* parser, int min_prec) {
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


static ASTNode* parse_if(Parser* parser) {
    return NULL;
}

static ASTNode* parse_while(Parser* parser) {
    return NULL;
}

static ASTNode* parse_for(Parser* parser) {
    return NULL;
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
    return NULL;
}

static ASTNode* parse_type(Parser* parser) {
    return NULL;
}

static ASTNode* parse_enum(Parser* parser) {
    return NULL;
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
    return NULL;
}


static ASTNode* parse_statement(Parser* parser) {
    switch (parser->cur_token->type) {
        case TOKEN_IF: return parse_if(parser);
        case TOKEN_WHILE: return parse_while(parser);
        case TOKEN_FOR: return parse_for(parser);
        case TOKEN_IMPORT: return parse_import(parser);
        case TOKEN_STRUCT: return parse_struct(parser);
        case TOKEN_TYPE: return parse_type(parser);
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
            advance(parser);
            return NULL;
        }
        default: {
            printf("Syntax error");
            advance(parser);
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
        Token* rbrace = expect(parser,TOKEN_RBRACE);
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
