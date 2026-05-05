#include <stdio.h>
#include <stdlib.h>
#include "lib/arena/arena.h"
#include "lib/file/file.h"
#include "compiler/lexer/lexer.h"

int main() {
    const char* code = "x := 42\nint y = 3";
    Lexer* lexer = LEXER_create(code);
    const Token* token = LEXER_next(lexer);
    while (token->type != TOKEN_EOF) {
        print_token(token);
        token = LEXER_next(lexer);
    }
    print_token(token);
    return 0;
}
