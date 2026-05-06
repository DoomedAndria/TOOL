#ifndef AST_H
#define AST_H
#include "../../lib/list/list.h"

typedef enum {
    NODE_VAR_DECL,
    NODE_VAR_ASSIGN,
    NODE_FUNC_DECL,
    NODE_RETURN_STM,
    NODE_IF_ELSE,
    NODE_SWITCH,
    NODE_CASE,
    NODE_WHILE,
    NODE_FOR,
    NODE_FOREACH,
    NODE_BRK,
    NODE_CONT,
    NODE_IMPORT,
    NODE_STRUCT_DECL,
    NODE_TYPE_DECL,
    NODE_ENUM_DECL,

    NODE_BINOP,
    NODE_UNOP,
    NODE_INT_LIT,
    NODE_FLOAT_LIT,
    NODE_STR_LIT,
    NODE_CHAR_LIT,
    NODE_BOOL_LIT,
    NODE_NULL_LIT,
    NODE_IDENT,
    NODE_FUNC_CALL,
    NODE_IDX_ACC,
    NODE_FLD_ACC,

    NODE_BLOCK,
} NodeType;

typedef struct ASTNode ASTNode;

struct ASTNode {
    NodeType type;
    int line;

    // NODE_NULL_LIT NODE_BRK, NODE_CONT


    union {
        // NODE_INT_LIT, NODE_BOOL_LIT
        int int_val;

        // NODE_FLOAT_LIT
        float float_val;

        // NODE_IDENT, NODE_STR_LIT, NODE_CHAR_LIT
        char* str_val;

        // NODE_BINOP
        struct {
            ASTNode* left;
            ASTNode* right;
            char* op;
        } binop;

        // NODE_UNOP
        struct {
            char* op;
            ASTNode* stmt;
        } unop;

        // NODE_VAR_DECL, NODE_VAR_ASSIGN
        struct {
            char* type_name;
            char* name;
            ASTNode* stmt;
        } dec_ass;

        // NODE_BLOCK
        struct {
            List* stmts;
        } block;

        // NODE_IF_ELSE
        struct {
            ASTNode* stmt;
            ASTNode* block1;
            ASTNode* block2;
        } if_else;

        // NODE_WHILE
        struct {
            ASTNode* stmt;
            ASTNode* block;
        } _while;

        // NODE_FOR (C-Style)
        struct {
            ASTNode* init;
            ASTNode* cond;
            ASTNode* post;
            ASTNode* block;
        } _for;

        // NODE_FOREACH
        struct {
            List* vnames;
            ASTNode* expression; // collection
            ASTNode* block;
        } foreach;

        // NODE_RETURN_STM
        struct {
            ASTNode* stmt;
        } rtrn;

        // NODE_FUNC_DECL
        struct {
            char* ret_type;
            char* name;
            List* var_decls;
            ASTNode* block;
        } f_dec;

        // NODE_FUNC_CALL
        struct {
            char* f_name;
            List* exps;
        } f_call;

        // NODE_SWITCH
        struct {
            ASTNode* expr;
            ASTNode* block;
        } _switch;

        // NODE_CASE
        struct {
            int is_default;
            ASTNode* expr;
            ASTNode* block;
        } _case;

        // NODE_STRUCT_DECL
        struct {
            char* name;
            List* field_decls;
        } _struct;

        // NODE_TYPE_DECL
        struct {
            char* name;
            List* fields;
            List* methods;
        } type_decl;

        // NODE_ENUM_DECL
        struct {
            char* name;
            List* variants;
        } _enum;

        // NODE_IMPORT
        struct {
            char* path;
            char* alias;
        } imprt;

        // NODE_IDX_ACC
        struct {
            ASTNode* obj;
            ASTNode* index;
        } idx_acc;

        // NODE_FLD_ACC
        struct {
            ASTNode* obj;
            char* field_name;
        } fld_acc;
    } as;
};

ASTNode* AST_create_node(NodeType type, int line);

void AST_free(ASTNode* node);
#endif
