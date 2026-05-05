#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "lib/types.h"
#include "lexer.h"


struct Lexer {
    const char* source;
    int pos;
    int line;
};


static void print_token_type(const TokenType type) {
    switch (type) {
        // special
        case TOKEN_NEWLINE: printf("TOKEN_NEWLINE");
            break;
        case TOKEN_IDENT: printf("TOKEN_IDENT");
            break;
        case TOKEN_EOF: printf("TOKEN_EOF");
            break;
        case TOKEN_UNKNOWN: printf("TOKEN_UNKNOWN");
            break;

        // literals
        case TOKEN_INT_LIT: printf("TOKEN_INT_LIT");
            break;
        case TOKEN_FLOAT_LIT: printf("TOKEN_FLOAT_LIT");
            break;
        case TOKEN_STRING_LIT: printf("TOKEN_STRING_LIT");
            break;
        case TOKEN_CHAR_LIT: printf("TOKEN_CHAR_LIT");
            break;
        case TOKEN_TRUE: printf("TOKEN_TRUE");
            break;
        case TOKEN_FALSE: printf("TOKEN_FALSE");
            break;
        case TOKEN_NULL: printf("TOKEN_NULL");
            break;

        // types
        case TOKEN_INT: printf("TOKEN_INT");
            break;
        case TOKEN_I8: printf("TOKEN_I8");
            break;
        case TOKEN_I16: printf("TOKEN_I16");
            break;
        case TOKEN_I32: printf("TOKEN_I32");
            break;
        case TOKEN_I64: printf("TOKEN_I64");
            break;
        case TOKEN_U8: printf("TOKEN_U8");
            break;
        case TOKEN_U16: printf("TOKEN_U16");
            break;
        case TOKEN_U32: printf("TOKEN_U32");
            break;
        case TOKEN_U64: printf("TOKEN_U64");
            break;
        case TOKEN_FLOAT: printf("TOKEN_FLOAT");
            break;
        case TOKEN_F32: printf("TOKEN_F32");
            break;
        case TOKEN_F64: printf("TOKEN_F64");
            break;
        case TOKEN_BOOL: printf("TOKEN_BOOL");
            break;
        case TOKEN_CHAR: printf("TOKEN_CHAR");
            break;
        case TOKEN_BYTE: printf("TOKEN_BYTE");
            break;
        case TOKEN_STRING: printf("TOKEN_STRING");
            break;
        case TOKEN_VOID: printf("TOKEN_VOID");
            break;

        // keywords
        case TOKEN_CONST: printf("TOKEN_CONST");
            break;
        case TOKEN_IF: printf("TOKEN_IF");
            break;
        case TOKEN_ELSE: printf("TOKEN_ELSE");
            break;
        case TOKEN_WHILE: printf("TOKEN_WHILE");
            break;
        case TOKEN_FOR: printf("TOKEN_FOR");
            break;
        case TOKEN_IN: printf("TOKEN_IN");
            break;
        case TOKEN_BREAK: printf("TOKEN_BREAK");
            break;
        case TOKEN_CONTINUE: printf("TOKEN_CONTINUE");
            break;
        case TOKEN_RETURN: printf("TOKEN_RETURN");
            break;
        case TOKEN_TYPE: printf("TOKEN_TYPE");
            break;
        case TOKEN_STRUCT: printf("TOKEN_STRUCT");
            break;
        case TOKEN_ENUM: printf("TOKEN_ENUM");
            break;
        case TOKEN_SWITCH: printf("TOKEN_SWITCH");
            break;
        case TOKEN_CASE: printf("TOKEN_CASE");
            break;
        case TOKEN_DEFAULT: printf("TOKEN_DEFAULT");
            break;
        case TOKEN_PUB: printf("TOKEN_PUB");
            break;
        case TOKEN_IMPORT: printf("TOKEN_IMPORT");
            break;
        case TOKEN_STATIC: printf("TOKEN_STATIC");
            break;
        case TOKEN_WEAK: printf("TOKEN_WEAK");
            break;
        case TOKEN_THIS: printf("TOKEN_THIS");
            break;
        case TOKEN_ON: printf("TOKEN_ON");
            break;
        case TOKEN_FREE: printf("TOKEN_FREE");
            break;
        case TOKEN_GET: printf("TOKEN_GET");
            break;
        case TOKEN_SET: printf("TOKEN_SET");
            break;

        // operators
        case TOKEN_ASSIGN: printf("TOKEN_ASSIGN");
            break;
        case TOKEN_DECLARE: printf("TOKEN_DECLARE");
            break;
        case TOKEN_PLUS: printf("TOKEN_PLUS");
            break;
        case TOKEN_MINUS: printf("TOKEN_MINUS");
            break;
        case TOKEN_STAR: printf("TOKEN_STAR");
            break;
        case TOKEN_SLASH: printf("TOKEN_SLASH");
            break;
        case TOKEN_PERCENT: printf("TOKEN_PERCENT");
            break;
        case TOKEN_EQ: printf("TOKEN_EQ");
            break;
        case TOKEN_NEQ: printf("TOKEN_NEQ");
            break;
        case TOKEN_LT: printf("TOKEN_LT");
            break;
        case TOKEN_GT: printf("TOKEN_GT");
            break;
        case TOKEN_LTE: printf("TOKEN_LTE");
            break;
        case TOKEN_GTE: printf("TOKEN_GTE");
            break;
        case TOKEN_AND: printf("TOKEN_AND");
            break;
        case TOKEN_OR: printf("TOKEN_OR");
            break;
        case TOKEN_NOT: printf("TOKEN_NOT");
            break;
        case TOKEN_QUESTION: printf("TOKEN_QUESTION");
            break;
        case TOKEN_DOT: printf("TOKEN_DOT");
            break;

        // delimiters
        case TOKEN_LPAREN: printf("TOKEN_LPAREN");
            break;
        case TOKEN_RPAREN: printf("TOKEN_RPAREN");
            break;
        case TOKEN_LBRACE: printf("TOKEN_LBRACE");
            break;
        case TOKEN_RBRACE: printf("TOKEN_RBRACE");
            break;
        case TOKEN_LBRACKET: printf("TOKEN_LBRACKET");
            break;
        case TOKEN_RBRACKET: printf("TOKEN_RBRACKET");
            break;
        case TOKEN_COMMA: printf("TOKEN_COMMA");
            break;
        case TOKEN_COLON: printf("TOKEN_COLON");
            break;

        default:
            printf("UNKNOWN_TOKEN_TYPE");
            break;
    }
}

void print_token(const Token* token) {
    printf("[");
    print_token_type(token->type);
    if (token->type == TOKEN_NEWLINE) printf("\\n");
    else printf(" %s", token->value);
    printf(" %d", token->line);
    printf("] ");
    if (token->type == TOKEN_NEWLINE) printf("\n");
}


static Token* make_token(Lexer* lexer,
                         const TokenType type,
                         const char* value,
                         const int advance) {
    Token* token = malloc(sizeof(Token));
    token->type = type;
    token->line = lexer->line;
    token->value = STRING(value);
    lexer->pos += advance;
    return token;
}

Lexer* LEXER_create(const char* source) {
    Lexer* lexer = malloc(sizeof(Lexer));
    lexer->source = source;
    lexer->pos = 0;
    lexer->line = 1;
    return lexer;
}

void LEXER_free(Lexer* lexer) {
    free(lexer);
}

static void skip_whitespaces(Lexer* lexer) {
    const char* source = lexer->source;
    while (source[lexer->pos] == ' ' || source[lexer->pos] == '\t' || source[lexer->pos] == '\r') {
        lexer->pos++;
    }
}

static Token* scan_single(Lexer* lexer) {
    const char* source = lexer->source;
    switch (source[lexer->pos]) {
        case '+': return make_token(lexer, TOKEN_PLUS, "+", 1);
        case '-': return make_token(lexer, TOKEN_MINUS, "-", 1);
        case '*': return make_token(lexer, TOKEN_STAR, "*", 1);
        case '/': return make_token(lexer, TOKEN_SLASH, "/", 1);
        case '%': return make_token(lexer, TOKEN_PERCENT, "%", 1);
        case '(': return make_token(lexer, TOKEN_LPAREN, "(", 1);
        case ')': return make_token(lexer, TOKEN_RPAREN, ")", 1);
        case '{': return make_token(lexer, TOKEN_LBRACE, "{", 1);
        case '}': return make_token(lexer, TOKEN_RBRACE, "}", 1);
        case '[': return make_token(lexer, TOKEN_LBRACKET, "[", 1);
        case ']': return make_token(lexer, TOKEN_RBRACKET, "]", 1);
        case '?': return make_token(lexer, TOKEN_QUESTION, "?", 1);
        case '.': return make_token(lexer, TOKEN_DOT, ".", 1);
        case ',': return make_token(lexer, TOKEN_COMMA, ",", 1);
        case '\n': {
            Token* token = make_token(lexer, TOKEN_NEWLINE, "\n", 1);
            lexer->line++;
            return token;
        }
        case '\0': return make_token(lexer, TOKEN_EOF, "\\0", 0);
        default: return NULL;
    }
}

static Token* scan_multi(Lexer* lexer) {
    const char* source = lexer->source;
    const char buf[2] = {source[lexer->pos], '\0'};
    switch (source[lexer->pos]) {
        case '=':
            if (source[lexer->pos + 1] == '=')
                return make_token(lexer, TOKEN_EQ, "==", 2);
            return make_token(lexer, TOKEN_ASSIGN, "=", 1);
        case ':':
            if (source[lexer->pos + 1] == '=')
                return make_token(lexer, TOKEN_DECLARE, ":=", 2);
            return make_token(lexer, TOKEN_COLON, ":", 1);
        case '!':
            if (source[lexer->pos + 1] == '=')
                return make_token(lexer, TOKEN_NEQ, "!=", 2);
            return make_token(lexer, TOKEN_NOT, "!", 1);
        case '<':
            if (source[lexer->pos + 1] == '=')
                return make_token(lexer, TOKEN_LTE, "<=", 2);
            return make_token(lexer, TOKEN_LT, "<", 1);
        case '>':
            if (source[lexer->pos + 1] == '=')
                return make_token(lexer, TOKEN_GTE, ">=", 2);
            return make_token(lexer, TOKEN_GT, ">", 1);
        case '&':
            if (source[lexer->pos + 1] == '&')
                return make_token(lexer, TOKEN_AND, "&&", 2);
            return make_token(lexer, TOKEN_UNKNOWN, buf, 1);
        case '|':
            if (source[lexer->pos + 1] == '|')
                return make_token(lexer, TOKEN_OR, "||", 2);
            return make_token(lexer, TOKEN_UNKNOWN, buf, 1);

        default: return NULL;
    }
}


static Token* scan_word(Lexer* lexer) {
    const char* source = lexer->source;
    if (!(isalpha(source[lexer->pos]) || source[lexer->pos] == '_')) return NULL;

    const int start = lexer->pos;
    int count = 0;

    while (isalnum(source[lexer->pos+count]) || source[lexer->pos + count] == '_') {
        count++;
    }
    lexer->pos += count;

    const char* word = source + start;


#define CHECK(str, type) \
    if (count == sizeof(str)-1 && memcmp(word, str, count) == 0) \
        return make_token(lexer,type, str, 0);


    CHECK("const", TOKEN_CONST);
    CHECK("if", TOKEN_IF);
    CHECK("else", TOKEN_ELSE);
    CHECK("while", TOKEN_WHILE);
    CHECK("for", TOKEN_FOR);
    CHECK("in", TOKEN_IN);
    CHECK("break", TOKEN_BREAK);
    CHECK("continue", TOKEN_CONTINUE);
    CHECK("return", TOKEN_RETURN);
    CHECK("type", TOKEN_TYPE);
    CHECK("struct", TOKEN_STRUCT);
    CHECK("enum", TOKEN_ENUM);
    CHECK("switch", TOKEN_SWITCH);
    CHECK("case", TOKEN_CASE);
    CHECK("default", TOKEN_DEFAULT);
    CHECK("pub", TOKEN_PUB);
    CHECK("import", TOKEN_IMPORT);
    CHECK("static", TOKEN_STATIC);
    CHECK("weak", TOKEN_WEAK);
    CHECK("this", TOKEN_THIS);
    CHECK("on", TOKEN_ON);
    CHECK("free", TOKEN_FREE);
    CHECK("get", TOKEN_GET);
    CHECK("set", TOKEN_SET);
    CHECK("true", TOKEN_TRUE);
    CHECK("false", TOKEN_FALSE);
    CHECK("null", TOKEN_NULL);

    CHECK("int", TOKEN_INT);
    CHECK("i8", TOKEN_I8);
    CHECK("i16", TOKEN_I16);
    CHECK("i32", TOKEN_I32);
    CHECK("i64", TOKEN_I64);
    CHECK("u8", TOKEN_U8);
    CHECK("u16", TOKEN_U16);
    CHECK("u32", TOKEN_U32);
    CHECK("u64", TOKEN_U64);

    CHECK("float", TOKEN_FLOAT);
    CHECK("f32", TOKEN_F32);
    CHECK("f64", TOKEN_F64);
    CHECK("bool", TOKEN_BOOL);
    CHECK("char", TOKEN_CHAR);
    CHECK("byte", TOKEN_BYTE);
    CHECK("string", TOKEN_STRING);
    CHECK("void", TOKEN_VOID);
#undef CHECK

    // stack allocated str slice copy
    char buf[count + 1];
    memcpy(buf, word, count);
    buf[count] = '\0';

    return make_token(lexer, TOKEN_IDENT, buf, 0);
}

static Token* scan_number(Lexer* lexer) {
    const char* source = lexer->source;
    if (!isdigit(source[lexer->pos])) return NULL;
    int count = 0;
    int dot_occ = 0;
    int start = lexer->pos;

    while (isdigit(source[lexer->pos+count]) || (!dot_occ && source[lexer->pos + count] == '.')) {
        if (source[lexer->pos + count] == '.') {
            if (!isdigit(source[lexer->pos + count + 1])) break;
            dot_occ = 1;
        }
        ++count;
    }

    const char* word = source + start;

    // stack allocated str slice copy
    char buf[count + 1];
    memcpy(buf, word, count);
    buf[count] = '\0';

    if (dot_occ) return make_token(lexer, TOKEN_FLOAT_LIT, buf, count);
    return make_token(lexer, TOKEN_INT_LIT, buf, count);
}

static Token* scan_string(Lexer* lexer) {
    const char* source = lexer->source;
    const int start = lexer->pos;
    if (source[start] != '"') return NULL;

    int count = 1;
    while (1) {
        const char c = source[lexer->pos + count];
        if (c == '\n' || c == '\0') {
            const char buf[2] = {lexer->source[lexer->pos], '\0'};
            return make_token(lexer, TOKEN_UNKNOWN, buf, 1);
        }
        if (c == '\\') {
            const char c1 = source[lexer->pos + count + 1];
            if (c1 == '\n' || c1 == '\0') return make_token(lexer, TOKEN_UNKNOWN, "\"", 1);
            count += 2;
            continue;
        }
        if (c == '"') {
            count++;
            break;
        }
        count++;
    }

    const char* word = source + start;

    // stack allocated str slice copy
    char buf[count - 1];
    memcpy(buf, word + 1, count - 2);
    buf[count - 2] = '\0';

    return make_token(lexer, TOKEN_STRING_LIT, buf, count);
}

static Token* scan_char(Lexer* lexer) {
    const char* source = lexer->source;
    const int start = lexer->pos;
    if (source[start] != '\'') return NULL;
    const char c = source[start + 1];
    const char c1 = source[start + 2];
    if (c == '\\') {
        if (c1 == '\n' || c1 == '\0' || source[start + 3] != '\'')
            return make_token(lexer, TOKEN_UNKNOWN, "'", 1);
        const char buf[3] = {'\\', c1, '\0'};
        return make_token(lexer, TOKEN_CHAR_LIT, buf, 4);
    }
    if (c == '\n' || c == '\0' || c == '\'' || c1 != '\'') return make_token(lexer, TOKEN_UNKNOWN, "'", 1);
    const char buf1[2] = {c, '\0'};
    return make_token(lexer, TOKEN_CHAR_LIT, buf1, 3);
}

static void skip_comments(Lexer* lexer) {
    const char* source = lexer->source;
    const int s = lexer->pos;
    if (source[s] == '/' && source[s + 1] == '/') {
        lexer->pos += 2;
        while (source[lexer->pos] != '\n' && source[lexer->pos] != '\0')
            lexer->pos++;

    }
}

Token* LEXER_next(Lexer* lexer) {
    skip_whitespaces(lexer);
    skip_comments(lexer);

    Token* token = scan_single(lexer);
    if (token) return token;
    token = scan_multi(lexer);
    if (token) return token;
    token = scan_word(lexer);
    if (token) return token;
    token = scan_number(lexer);
    if (token) return token;
    token = scan_string(lexer);
    if (token) return token;
    token = scan_char(lexer);
    if (token) return token;

    const char buf[2] = {lexer->source[lexer->pos], '\0'};
    return make_token(lexer, TOKEN_UNKNOWN, buf, 1);
}
