#ifndef LEXER_H
#define LEXER_H

typedef enum {
    // special
    TOKEN_NEWLINE,
    TOKEN_IDENT,
    TOKEN_EOF,
    TOKEN_UNKNOWN,

    // literals
    TOKEN_INT_LIT,
    TOKEN_FLOAT_LIT,
    TOKEN_STRING_LIT,
    TOKEN_CHAR_LIT,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_NULL,

    // types
    TOKEN_INT,
    TOKEN_I8,
    TOKEN_I16,
    TOKEN_I32,
    TOKEN_I64,
    TOKEN_U8,
    TOKEN_U16,
    TOKEN_U32,
    TOKEN_U64,
    TOKEN_FLOAT,
    TOKEN_F32,
    TOKEN_F64,
    TOKEN_BOOL,
    TOKEN_CHAR,
    TOKEN_BYTE,
    TOKEN_STRING,
    TOKEN_VOID,

    //keywords
    TOKEN_FROM,
    TOKEN_CONST,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_FOR,
    TOKEN_IN,
    TOKEN_BREAK,
    TOKEN_CONTINUE,
    TOKEN_RETURN,
    TOKEN_TYPE,
    TOKEN_STRUCT,
    TOKEN_ENUM,
    TOKEN_SWITCH,
    TOKEN_CASE,
    TOKEN_DEFAULT,
    TOKEN_PUB,
    TOKEN_IMPORT,
    TOKEN_STATIC,
    TOKEN_WEAK,
    TOKEN_THIS,
    TOKEN_ON,
    TOKEN_FREE,
    TOKEN_GET,
    TOKEN_SET,

    // operators
    TOKEN_ASSIGN,
    TOKEN_DECLARE,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_PERCENT,
    TOKEN_EQ,
    TOKEN_NEQ,
    TOKEN_LT,
    TOKEN_GT,
    TOKEN_LTE,
    TOKEN_GTE,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_NOT,
    TOKEN_QUESTION,
    TOKEN_DOT,

    // delimiters
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,
    TOKEN_COMMA,
    TOKEN_COLON,
    TOKEN_SEMICOLON,
} TokenType;

typedef struct Token {
    TokenType type;
    char* value;
    int line;
} Token;

typedef struct Lexer Lexer;

void print_token_type(TokenType type);

void print_token(const Token* token);

Lexer* LEXER_create(const char* source);

Token* LEXER_next(Lexer* lexer);

void LEXER_free(Lexer* lexer);
#endif
