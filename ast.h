#ifndef AST_H
#define AST_H

typedef enum {
    AST_TYPE_NONE,
    AST_TYPE_INT,
    AST_TYPE_CAR,
    AST_TYPE_INT_VECTOR,
    AST_TYPE_CAR_VECTOR
} ASTType;

typedef enum {
    AST_PROGRAM,
    AST_BLOCK,
    AST_DECL,
    AST_FUNCTION,
    AST_PARAM,
    AST_RETURN,
    AST_CALL,
    AST_INDEX,
    AST_EMPTY_CMD,
    AST_READ,
    AST_WRITE,
    AST_NEWLINE,
    AST_IF,
    AST_WHILE,
    AST_ASSIGN,
    AST_BINARY,
    AST_UNARY,
    AST_IDENTIFIER,
    AST_INT_CONST,
    AST_CAR_CONST,
    AST_STRING
} ASTKind;

typedef struct ASTNode {
    ASTKind kind;
    ASTType type;
    int line;
    char *lexeme;
    struct ASTNode *first;
    struct ASTNode *second;
    struct ASTNode *third;
    struct ASTNode *next;
} ASTNode;

typedef struct {
    char *text;
    int line;
} TokenInfo;

ASTNode *ast_new(ASTKind kind, int line, const char *lexeme, ASTType type,
                 ASTNode *first, ASTNode *second, ASTNode *third);
ASTNode *ast_new_token(ASTKind kind, TokenInfo token, ASTType type,
                       ASTNode *first, ASTNode *second, ASTNode *third);
ASTNode *ast_leaf(ASTKind kind, TokenInfo token);
ASTNode *ast_append(ASTNode *list, ASTNode *node);
void ast_set_decl_type(ASTNode *decls, ASTType type);
void ast_free(ASTNode *node);

#endif
