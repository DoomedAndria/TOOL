#include <stdio.h>
#include <stdlib.h>
#include "lib/arena/arena.h"
#include "lib/file/file.h"
#include "compiler/lexer/lexer.h"

static void lex_print(const char* source) {
    printf("Source: \"%s\"\n", source);
    Lexer* lexer = LEXER_create(source);
    Token* token;
    while (1) {
        token = LEXER_next(lexer);
        print_token(token);
        int done = token->type == TOKEN_EOF;
        free(token->value);
        free(token);
        if (done) break;
    }
    LEXER_free(lexer);
    printf("\n\n");
}

int main() {
    // strings
    lex_print("\"hello\"");
    lex_print("\"\"");
    lex_print("\"hello world\"");

    // unterminated string — hits newline
    lex_print("\"hello\nworld\"");

    // unterminated string — hits EOF
    lex_print("\"hello");

    // string in expression
    lex_print("x := \"foo\"");

    return 0;
}
