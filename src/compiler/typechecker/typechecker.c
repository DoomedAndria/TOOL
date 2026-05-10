#include <stdlib.h>
#include "typechecker.h"

#include <stdio.h>

#include "compiler/ast/ast.h"
#include "lib/types.h"

Symbol* SYMBOL_create(const char* name,
                      const char* type,
                      const SymbolKind kind,
                      const int is_const,
                      const int line) {
    Symbol* symbol = malloc(sizeof(Symbol));
    symbol->name = STRING(name);
    symbol->type = STRING(type);
    symbol->kind = kind;
    symbol->is_const = is_const;
    symbol->line = line;
    return symbol;
}

void SYMBOL_print(const Symbol* symbol) {
    char* sk = "unknown";
    switch (symbol->kind) {
        case SYM_VAR: sk = "VAR";
            break;
        case SYM_FN: sk = "FN";
            break;
        case SYM_TYPE: sk = "TYPE";
            break;
        case SYM_PARAM: sk = "PARAM";
            break;
    }
    printf("[%s] %s: %s%s (line %d)", sk,
           symbol->name,
           symbol->is_const ? " const " : " ",
           symbol->type, symbol->line);
}

void SYMBOL_free(Symbol* symbol) {
    free(symbol->name);
    free(symbol->type);
    free(symbol);
}

static void p_symbol(const void* data) {
    SYMBOL_print((Symbol *) data);
}

static void f_symbol(void* data) {
    SYMBOL_free(data);
}

Scope* SCOPE_create(Scope* parent) {
    Scope* scope = malloc(sizeof(Scope));
    HashTable* symbols = HASH_create_str(f_symbol, p_symbol);
    scope->symbols = symbols;
    scope->parent = parent;
    return scope;
}

void SCOPE_free(Scope* scope) {
    if (scope) {
        HASH_free(scope->symbols);
        free(scope);
    }
}

int SCOPE_define(Scope* scope, Symbol* symbol) {
    if (HASH_contains(scope->symbols, symbol->name)) {
        printf("Error: '%s' already declared in this scope (line %d)\n", symbol->name, symbol->line);
        return 0;
    }
    // avoiding double free
    HASH_insert(scope->symbols, STRING(symbol->name), symbol);
    return 1;
}

Symbol* SCOPE_lookup(const Scope* scope, const char* name) {
    Symbol* symbol = HASH_get(scope->symbols, name);
    if (!symbol && scope->parent)
        return SCOPE_lookup(scope->parent, name);
    return symbol;
}


TypeChecker* TC_create() {
    TypeChecker* typechecker = malloc(sizeof(TypeChecker));
    Scope* scope = SCOPE_create(NULL);
    typechecker->scope = scope;
    typechecker->errors = 0;
    return typechecker;
}

void TC_free(TypeChecker* tc) {
    SCOPE_free(tc->scope);
    free(tc);
}

static void tc_define(TypeChecker* tc,
                      const char* name,
                      const char* type,
                      const SymbolKind kind,
                      const int is_const,
                      const int line) {
    Symbol* s = SYMBOL_create(name, type, kind, is_const, line);
    if (!SCOPE_define(tc->scope, s)) {
        SYMBOL_free(s);
        tc->errors++;
    }
}

static void tc_push_scope(TypeChecker* tc) {
    tc->scope = SCOPE_create(tc->scope);
}

static void tc_pop_scope(TypeChecker* tc) {
    Scope* s = tc->scope;
    tc->scope = s->parent;
    SCOPE_free(s);
}

static void tc_check_block(TypeChecker* tc, ASTNode* block);

static void tc_check_block_stmts(TypeChecker* tc, ASTNode* block);

static void tc_check_node(TypeChecker* tc, ASTNode* node) {
    switch (node->type) {
        case NODE_VAR_DECL: {
            char* type_name = "unknown";
            char* node_type_name = node->as.dec_ass.type_name;
            if (node_type_name) {
                type_name = node_type_name;
            }
            tc_define(tc,
                      node->as.dec_ass.name,
                      type_name,
                      SYM_VAR,
                      node->as.dec_ass.is_const,
                      node->line);
            break;
        }
        case NODE_FUNC_DECL: {
            const char* type_name = "void";
            const List* params = node->as.f_dec.var_decls;
            if (LIST_size(node->as.f_dec.ret_types) == 1) {
                const ASTNode* ret_type_node = LIST_get(node->as.f_dec.ret_types, 0).data;
                type_name = ret_type_node->as.dec_ass.type_name;
            } else if (LIST_size(node->as.f_dec.ret_types) > 1)
                type_name = "multi";

            tc_define(tc,
                      node->as.f_dec.name,
                      type_name,
                      SYM_FN,
                      0,
                      node->line);

            tc_push_scope(tc);

            ListIter* iter = LIST_iter(params);
            while (LIST_iter_has_next(iter)) {
                const ASTNode* node1 = LIST_iter_next(iter);
                tc_define(tc,
                          node1->as.dec_ass.name,
                          node1->as.dec_ass.type_name,
                          SYM_PARAM,
                          0,
                          node1->line);
            }
            LIST_iter_free(iter);

            tc_check_block_stmts(tc, node->as.f_dec.block);

            tc_pop_scope(tc);
            break;
        }
        case NODE_IF_ELSE: {
            tc_check_block(tc, node->as.if_else.block1);
            if (node->as.if_else.block2)
                tc_check_block(tc, node->as.if_else.block2);
            break;
        }
        case NODE_WHILE: {
            tc_check_block(tc, node->as._while.block);
            break;
        }
        case NODE_FOR: {
            tc_check_block(tc, node->as._for.block);
            break;
        }
        case NODE_FOREACH: {
            tc_check_block(tc, node->as.foreach.block);
            break;
        }
        case NODE_SWITCH: {
            const List* cases = node->as._switch.block->as.block.stmts;
            ListIter* iter = LIST_iter(cases);
            while (LIST_iter_has_next(iter)) {
                const ASTNode* _case = LIST_iter_next(iter);
                tc_check_block(tc, _case->as._case.block);
            }
            LIST_iter_free(iter);
            break;
        }
        case NODE_REF_DECL: {
            tc_define(tc,
                      node->as.ref_decl.name,
                      node->as.ref_decl.name,
                      SYM_TYPE,
                      0,
                      node->line);
            const List* methods = node->as.ref_decl.methods;
            ListIter* iter = LIST_iter(methods);
            while (LIST_iter_has_next(iter)) {
                ASTNode* method = LIST_iter_next(iter);
                tc_check_node(tc, method);
            }
            LIST_iter_free(iter);
            break;
        }
        case NODE_STRUCT_DECL: {
            tc_define(tc,
                      node->as._struct.name,
                      node->as._struct.name,
                      SYM_TYPE,
                      0,
                      node->line);
            break;
        }
        case NODE_ENUM_DECL: {
            tc_define(tc,
                      node->as._enum.name,
                      node->as._enum.name,
                      SYM_TYPE,
                      0,
                      node->line);

            const List* variants = node->as._enum.variants;
            ListIter* iter = LIST_iter(variants);
            while (LIST_iter_has_next(iter)) {
                const char* var = LIST_iter_next(iter);
                tc_define(tc,
                          var,
                          node->as._enum.name,
                          SYM_VAR,
                          0,
                          node->line);
            }
            break;
        }
        case NODE_VAR_ASSIGN: {
            Symbol* symbol = SCOPE_lookup(tc->scope, node->as.dec_ass.name);
            if (!symbol) {
                printf("Error: '%s' is not declared (line %d)\n", node->as.dec_ass.name, node->line);
                tc->errors++;
            } else if (symbol->is_const) {
                printf("Error: cannot assign to const '%s' (line %d)\n", symbol->name, node->line);
                tc->errors++;
            }
            break;
        }
        case NODE_RETURN_STM: {
            if (node->as.rtrn.stmt)
                tc_check_node(tc, node->as.rtrn.stmt);
            break;
        }
        case NODE_IMPORT: {
            tc_define(tc,
                      node->as.imprt.alias ? node->as.imprt.alias : node->as.imprt.path,
                      node->as.imprt.alias ? node->as.imprt.alias : node->as.imprt.path,
                      SYM_TYPE,
                      0,
                      node->line);
            break;
        }
        default: return;
    }
}

static void tc_check_block_stmts(TypeChecker* tc, ASTNode* block) {
    ListIter* iter = LIST_iter(block->as.block.stmts);
    while (LIST_iter_has_next(iter))
        tc_check_node(tc, LIST_iter_next(iter));
    LIST_iter_free(iter);
}

static void tc_check_block(TypeChecker* tc, ASTNode* block) {
    tc_push_scope(tc);
    tc_check_block_stmts(tc, block);
    tc_pop_scope(tc);
}

int TC_check(TypeChecker* tc, List* ast) {
    ListIter* iter = LIST_iter(ast);
    while (LIST_iter_has_next(iter)) {
        ASTNode* node = LIST_iter_next(iter);
        tc_check_node(tc, node);
    }
    LIST_iter_free(iter);
    return tc->errors == 0;
}
