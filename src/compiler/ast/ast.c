#include "ast.h"

#include <stdlib.h>
#include <string.h>


ASTNode* AST_create_node(const NodeType type, const int line) {
    ASTNode* ast_node = malloc(sizeof(ASTNode));
    ast_node->type = type;
    ast_node->line = line;
    memset(&ast_node->as, 0, sizeof(ast_node->as));
    return ast_node;
}

void AST_free(ASTNode* node) {
    switch (node->type) {
        case NODE_BRK:
        case NODE_CONT:
        case NODE_NULL_LIT:
        case NODE_INT_LIT:
        case NODE_BOOL_LIT:
        case NODE_FLOAT_LIT:
            break;

        case NODE_STR_LIT:
        case NODE_CHAR_LIT:
        case NODE_IDENT:
            free(node->as.str_val);
            break;

        case NODE_BINOP:
            AST_free(node->as.binop.left);
            AST_free(node->as.binop.right);
            free(node->as.binop.op);
            break;

        case NODE_UNOP:
            AST_free(node->as.unop.stmt);
            free(node->as.unop.op);
            break;

        case NODE_VAR_ASSIGN:
        case NODE_VAR_DECL:
            if (node->as.dec_ass.stmt) AST_free(node->as.dec_ass.stmt);
            free(node->as.dec_ass.type_name);
            free(node->as.dec_ass.name);
            break;

        case NODE_BLOCK:
            LIST_free(node->as.block.stmts);
            break;

        case NODE_IF_ELSE:
            AST_free(node->as.if_else.stmt);
            AST_free(node->as.if_else.block1);
            if (node->as.if_else.block2)
                AST_free(node->as.if_else.block2);
            break;

        case NODE_WHILE:
            AST_free(node->as._while.block);
            AST_free(node->as._while.stmt);
            break;

        case NODE_FOR:
            AST_free(node->as._for.init);
            AST_free(node->as._for.cond);
            AST_free(node->as._for.post);
            AST_free(node->as._for.block);
            break;

        case NODE_FOREACH:
            LIST_free(node->as.foreach.vnames);
            AST_free(node->as.foreach.expression);
            AST_free(node->as.foreach.block);
            break;

        case NODE_RETURN_STM:
            AST_free(node->as.rtrn.stmt);
            break;

        case NODE_FUNC_DECL:
            AST_free(node->as.f_dec.block);
            LIST_free(node->as.f_dec.var_decls);
            free(node->as.f_dec.name);
            LIST_free(node->as.f_dec.ret_types);
            break;

        case NODE_FUNC_CALL:
            AST_free(node->as.f_call.callee);
            LIST_free(node->as.f_call.exps);
            break;

        case NODE_SWITCH:
            AST_free(node->as._switch.block);
            AST_free(node->as._switch.expr);
            break;

        case NODE_CASE:
            AST_free(node->as._case.block);
            if (!node->as._case.is_default)
                AST_free(node->as._case.expr);
            break;

        case NODE_STRUCT_DECL:
            LIST_free(node->as._struct.field_decls);
            free(node->as._struct.name);
            break;

        case NODE_REF_DECL:
            LIST_free(node->as.ref_decl.fields);
            LIST_free(node->as.ref_decl.methods);
            free(node->as.ref_decl.name);
            break;

        case NODE_ENUM_DECL:
            LIST_free(node->as._enum.variants);
            free(node->as._enum.name);
            break;

        case NODE_IMPORT:
            free(node->as.imprt.path);
            free(node->as.imprt.alias);
            break;

        case NODE_IDX_ACC:
            AST_free(node->as.idx_acc.obj);
            AST_free(node->as.idx_acc.index);
            break;

        case NODE_FLD_ACC:
            AST_free(node->as.fld_acc.obj);
            free(node->as.fld_acc.field_name);
            break;

        case NODE_FLD_SET:
            AST_free(node->as.fld_set.obj);
            free(node->as.fld_set.field_name);
            AST_free(node->as.fld_set.value);
            break;

        case NODE_IDX_SET:
            AST_free(node->as.idx_set.obj);
            AST_free(node->as.idx_set.index);
            AST_free(node->as.idx_set.value);
            break;
    }
    free(node);
}
