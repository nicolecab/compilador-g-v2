#include "ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *copy_text(const char *text){
    char *copy;
    size_t size;

    if(text == NULL){
        return NULL;
    }

    size = strlen(text) + 1;
    copy = malloc(size);
    if(copy == NULL){
        fprintf(stderr, "Erro interno: memoria insuficiente\n");
        exit(1);
    }

    memcpy(copy, text, size);
    return copy;
}

ASTNode *ast_new(ASTKind kind, int line, const char *lexeme, ASTType type,
                 ASTNode *first, ASTNode *second, ASTNode *third){
    ASTNode *node = malloc(sizeof(ASTNode));

    if(node == NULL){
        fprintf(stderr, "Erro interno: memoria insuficiente\n");
        exit(1);
    }

    node->kind = kind;
    node->type = type;
    node->line = line;
    node->lexeme = copy_text(lexeme);
    node->first = first;
    node->second = second;
    node->third = third;
    node->next = NULL;

    return node;
}

ASTNode *ast_new_token(ASTKind kind, TokenInfo token, ASTType type,
                       ASTNode *first, ASTNode *second, ASTNode *third){
    ASTNode *node = ast_new(kind, token.line, token.text, type, first, second, third);

    free(token.text);
    return node;
}

ASTNode *ast_leaf(ASTKind kind, TokenInfo token){
    ASTType type = AST_TYPE_NONE;

    if(kind == AST_INT_CONST){
        type = AST_TYPE_INT;
    } else if(kind == AST_CAR_CONST){
        type = AST_TYPE_CAR;
    }

    return ast_new_token(kind, token, type, NULL, NULL, NULL);
}

ASTNode *ast_append(ASTNode *list, ASTNode *node){
    ASTNode *current;

    if(list == NULL){
        return node;
    }
    if(node == NULL){
        return list;
    }

    current = list;
    while(current->next != NULL){
        current = current->next;
    }
    current->next = node;

    return list;
}

void ast_set_decl_type(ASTNode *decls, ASTType type){
    ASTNode *current = decls;

    while(current != NULL){
        if(current->first != NULL && current->first->kind == AST_INT_CONST){
            current->type = (type == AST_TYPE_INT) ? AST_TYPE_INT_VECTOR : AST_TYPE_CAR_VECTOR;
        } else {
            current->type = type;
        }
        current = current->next;
    }
}

void ast_free(ASTNode *node){
    ASTNode *next;

    while(node != NULL){
        next = node->next;
        ast_free(node->first);
        ast_free(node->second);
        ast_free(node->third);
        free(node->lexeme);
        free(node);
        node = next;
    }
}
