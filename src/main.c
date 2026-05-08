#include "compiler/lexer/lexer.h"
#include "compiler/parser/parser.h"
#include "lib/list/list.h"

int main() {
    const char* src = "if (x) { break } else { continue }\nreturn 1 + 2 * 3\n";
    Lexer* lexer = LEXER_create(src);
    Parser* parser = PARSER_create(lexer);
    List* ast = PARSER_parse(parser);
    LIST_free(ast);
    PARSER_free(parser);
    LEXER_free(lexer);
    return 0;
}
