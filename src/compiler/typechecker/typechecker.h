#ifndef TYPECHECKER_H
#define TYPECHECKER_H
#include "lib/hash/hash.h"

typedef enum {
    SYM_VAR,
    SYM_FN,
    SYM_TYPE,
    SYM_PARAM
} SymbolKind;

typedef struct Symbol Symbol;

struct Symbol {
    char* name;
    char* type;
    SymbolKind kind;
    int is_const;
    int line;
};

Symbol* SYMBOL_create(const char* name,
                      const char* type,
                      SymbolKind kind,
                      int is_const,
                      int line);

void SYMBOL_print(const Symbol* symbol);

void SYMBOL_free(Symbol* symbol);

typedef struct Scope Scope;

struct Scope {
    HashTable* symbols;
    Scope* parent;
};

Scope* SCOPE_create(Scope* parent);

void SCOPE_free(Scope* scope);

int SCOPE_define(Scope* scope, Symbol* symbol);

Symbol* SCOPE_lookup(const Scope* scope, const char* name);

typedef struct TypeChecker TypeChecker;

struct TypeChecker {
    Scope* scope;
    const char* current_return_type;
    int errors;
};

TypeChecker* TC_create();

void TC_free(TypeChecker* tc);

int TC_check(TypeChecker* tc, List* ast);

#endif
