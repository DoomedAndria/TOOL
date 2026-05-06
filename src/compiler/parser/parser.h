#ifndef PARSER_H
#define PARSER_H
#include "compiler/lexer/lexer.h"
#include "lib/list/list.h"

typedef struct {
    Lexer* lexer;
    Token* cur_token;
    Token* peek_token;
} Parser;


Parser* PARSER_create(Lexer* lexer);

void PARSER_free(Parser* parser);

List* PARSER_parse(Parser* parser);

#endif
