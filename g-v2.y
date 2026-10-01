%{
#include <stdio.h>
#include <stdlib.h>
#include "ast.h"
#include "semantico.h"
#include "gerador_codigo.h"

extern int yylineno;
extern int ultima_linha_token;
extern int linha_token_anterior;
extern char* yytext;
extern int yylex();
extern FILE *yyin;

ASTNode *ast_root = NULL;

static ASTNode *param_vector(ASTNode *param);
void yyerror(const char* s);
%}
%code requires {
#include "ast.h"
}
%union {
    ASTNode *node;
    ASTType type;
    TokenInfo token;
}
%locations
%token GLOBAL
%token FUNCAO
%token PRINCIPAL
%token <token> IDENTIFICADOR
%token INT
%token CAR
%token RETORNE
%token LEIA
%token ESCREVA
%token <token> CADEIACARACTERES
%token NOVALINHA
%token SE
%token ENTAO
%token FIMSE
%token SENAO
%token ENQUANTO
%token OU
%token E
%token IGUAL
%token DIFERENTE
%token MAIORIGUAL
%token MENORIGUAL
%token <token> CARCONST
%token <token> INTCONST
%type <node> programa declvarglobais varsection listadeclvar listvar
%type <node> declfunc listafuncoes listaparametros listaparametrostail
%type <node> declprograma bloco listacomando comando expr lvalueexpr
%type <node> orexpr andexpr eqexpr desigexpr addexpr mulexpr unexpr primexpr listexpr
%type <type> tipo
%%
programa: declvarglobais declfunc declprograma
        {
            ast_root = ast_new(AST_PROGRAM, @1.first_line, "programa", AST_TYPE_NONE, $1, $2, $3);
            $$ = ast_root;
        }
        ;

declvarglobais: GLOBAL varsection { $$ = $2; }
              | /* vazio */ { $$ = NULL; }
              ;

varsection: '[' listadeclvar ']' { $$ = $2; };

listadeclvar: listvar ':' tipo ';' listadeclvar
            {
                ast_set_decl_type($1, $3);
                $$ = ast_append($1, $5);
            }
            | listvar ':' tipo ';'
            {
                ast_set_decl_type($1, $3);
                $$ = $1;
            }
            ;

listvar: IDENTIFICADOR ',' listvar
       { $$ = ast_append(ast_leaf(AST_DECL, $1), $3); }
       | IDENTIFICADOR '[' INTCONST ']' ',' listvar
       { $$ = ast_append(ast_new_token(AST_DECL, $1, AST_TYPE_NONE, ast_leaf(AST_INT_CONST, $3), NULL, NULL), $6); }
       | IDENTIFICADOR
       { $$ = ast_leaf(AST_DECL, $1); }
       | IDENTIFICADOR '[' INTCONST ']'
       { $$ = ast_new_token(AST_DECL, $1, AST_TYPE_NONE, ast_leaf(AST_INT_CONST, $3), NULL, NULL); }
       ;

declfunc: FUNCAO '[' IDENTIFICADOR '(' listaparametros ')' ':' tipo bloco listafuncoes ']'
        {
            ASTNode *func = ast_new_token(AST_FUNCTION, $3, $8, $5, $9, NULL);
            $$ = ast_append(func, $10);
        }
        | /* vazio */ { $$ = NULL; }
        ;

listafuncoes: IDENTIFICADOR '(' listaparametros ')' ':' tipo bloco listafuncoes
            {
                ASTNode *func = ast_new_token(AST_FUNCTION, $1, $6, $3, $7, NULL);
                $$ = ast_append(func, $8);
            }
            | /* vazio */ { $$ = NULL; }
            ;

listaparametros: listaparametrostail { $$ = $1; }
               | /* vazio */ { $$ = NULL; }
               ;

listaparametrostail: IDENTIFICADOR ':' tipo
                   { $$ = ast_new_token(AST_PARAM, $1, $3, NULL, NULL, NULL); }
                   | IDENTIFICADOR '[' ']' ':' tipo
                   { $$ = param_vector(ast_new_token(AST_PARAM, $1, $5, NULL, NULL, NULL)); }
                   | IDENTIFICADOR ':' tipo ',' listaparametrostail
                   { $$ = ast_append(ast_new_token(AST_PARAM, $1, $3, NULL, NULL, NULL), $5); }
                   | IDENTIFICADOR '[' ']' ':' tipo ',' listaparametrostail
                   { $$ = ast_append(param_vector(ast_new_token(AST_PARAM, $1, $5, NULL, NULL, NULL)), $7); }
                   ;

declprograma: PRINCIPAL bloco { $$ = $2; };

bloco: '{' listacomando '}'
     { $$ = ast_new(AST_BLOCK, @1.first_line, NULL, AST_TYPE_NONE, NULL, $2, NULL); }
     | varsection '{' listacomando '}'
     { $$ = ast_new(AST_BLOCK, @1.first_line, NULL, AST_TYPE_NONE, $1, $3, NULL); }
     ;

tipo: INT { $$ = AST_TYPE_INT; }
    | CAR { $$ = AST_TYPE_CAR; }
    ;

listacomando: comando
            { $$ = $1; }
            | comando listacomando
            { $$ = ast_append($1, $2); }
            ;

comando: ';'
       { $$ = ast_new(AST_EMPTY_CMD, @1.first_line, NULL, AST_TYPE_NONE, NULL, NULL, NULL); }
       | expr ';'
       { $$ = $1; }
       | RETORNE expr ';'
       { $$ = ast_new(AST_RETURN, @1.first_line, "retorne", AST_TYPE_NONE, $2, NULL, NULL); }
       | LEIA lvalueexpr ';'
       { $$ = ast_new(AST_READ, @1.first_line, "leia", AST_TYPE_NONE, $2, NULL, NULL); }
       | ESCREVA expr ';'
       { $$ = ast_new(AST_WRITE, @1.first_line, "escreva", AST_TYPE_NONE, $2, NULL, NULL); }
       | ESCREVA CADEIACARACTERES ';'
       { $$ = ast_new(AST_WRITE, @1.first_line, "escreva", AST_TYPE_NONE, ast_leaf(AST_STRING, $2), NULL, NULL); }
       | NOVALINHA ';'
       { $$ = ast_new(AST_NEWLINE, @1.first_line, "novalinha", AST_TYPE_NONE, NULL, NULL, NULL); }
       | SE '(' expr ')' ENTAO comando FIMSE
       { $$ = ast_new(AST_IF, @1.first_line, "se", AST_TYPE_NONE, $3, $6, NULL); }
       | SE '(' expr ')' ENTAO comando SENAO comando FIMSE
       { $$ = ast_new(AST_IF, @1.first_line, "se", AST_TYPE_NONE, $3, $6, $8); }
       | ENQUANTO '(' expr ')' comando
       { $$ = ast_new(AST_WHILE, @1.first_line, "enquanto", AST_TYPE_NONE, $3, $5, NULL); }
       | bloco
       { $$ = $1; }
       ;

expr: lvalueexpr '=' expr
    { $$ = ast_new(AST_ASSIGN, @2.first_line, "=", AST_TYPE_NONE, $1, $3, NULL); }
    | orexpr
    { $$ = $1; }
    ;

lvalueexpr: IDENTIFICADOR '[' expr ']'
          { $$ = ast_new_token(AST_INDEX, $1, AST_TYPE_NONE, $3, NULL, NULL); }
          | IDENTIFICADOR
          { $$ = ast_leaf(AST_IDENTIFIER, $1); }
          ;

orexpr: orexpr OU andexpr
      { $$ = ast_new(AST_BINARY, @2.first_line, "||", AST_TYPE_INT, $1, $3, NULL); }
      | andexpr
      { $$ = $1; }
      ;

andexpr: andexpr E eqexpr
       { $$ = ast_new(AST_BINARY, @2.first_line, "&", AST_TYPE_INT, $1, $3, NULL); }
       | eqexpr
       { $$ = $1; }
       ;

eqexpr: eqexpr IGUAL desigexpr
      { $$ = ast_new(AST_BINARY, @2.first_line, "==", AST_TYPE_INT, $1, $3, NULL); }
      | eqexpr DIFERENTE desigexpr
      { $$ = ast_new(AST_BINARY, @2.first_line, "!=", AST_TYPE_INT, $1, $3, NULL); }
      | desigexpr
      { $$ = $1; }
      ;

desigexpr: desigexpr '<' addexpr
         { $$ = ast_new(AST_BINARY, @2.first_line, "<", AST_TYPE_INT, $1, $3, NULL); }
         | desigexpr '>' addexpr
         { $$ = ast_new(AST_BINARY, @2.first_line, ">", AST_TYPE_INT, $1, $3, NULL); }
         | desigexpr MAIORIGUAL addexpr
         { $$ = ast_new(AST_BINARY, @2.first_line, ">=", AST_TYPE_INT, $1, $3, NULL); }
         | desigexpr MENORIGUAL addexpr
         { $$ = ast_new(AST_BINARY, @2.first_line, "<=", AST_TYPE_INT, $1, $3, NULL); }
         | addexpr
         { $$ = $1; }
         ;

addexpr: addexpr '+' mulexpr
       { $$ = ast_new(AST_BINARY, @2.first_line, "+", AST_TYPE_NONE, $1, $3, NULL); }
       | addexpr '-' mulexpr
       { $$ = ast_new(AST_BINARY, @2.first_line, "-", AST_TYPE_NONE, $1, $3, NULL); }
       | mulexpr
       { $$ = $1; }
       ;

mulexpr: mulexpr '*' unexpr
       { $$ = ast_new(AST_BINARY, @2.first_line, "*", AST_TYPE_NONE, $1, $3, NULL); }
       | mulexpr '/' unexpr
       { $$ = ast_new(AST_BINARY, @2.first_line, "/", AST_TYPE_NONE, $1, $3, NULL); }
       | unexpr
       { $$ = $1; }
       ;

unexpr: '-' primexpr
      { $$ = ast_new(AST_UNARY, @1.first_line, "-", AST_TYPE_NONE, $2, NULL, NULL); }
      | '!' primexpr
      { $$ = ast_new(AST_UNARY, @1.first_line, "!", AST_TYPE_INT, $2, NULL, NULL); }
      | primexpr
      { $$ = $1; }
      ;

primexpr: IDENTIFICADOR '(' listexpr ')'
        { $$ = ast_new_token(AST_CALL, $1, AST_TYPE_NONE, $3, NULL, NULL); }
        | IDENTIFICADOR '(' ')'
        { $$ = ast_new_token(AST_CALL, $1, AST_TYPE_NONE, NULL, NULL, NULL); }
        | IDENTIFICADOR '[' expr ']'
        { $$ = ast_new_token(AST_INDEX, $1, AST_TYPE_NONE, $3, NULL, NULL); }
        | IDENTIFICADOR
        { $$ = ast_leaf(AST_IDENTIFIER, $1); }
        | CARCONST
        { $$ = ast_leaf(AST_CAR_CONST, $1); }
        | INTCONST
        { $$ = ast_leaf(AST_INT_CONST, $1); }
        | '(' expr ')'
        { $$ = $2; }
        ;

listexpr: expr
        { $$ = $1; }
        | listexpr ',' expr
        { $$ = ast_append($1, $3); }
        ;
%%
static ASTNode *param_vector(ASTNode *param){
    if(param->type == AST_TYPE_INT){
        param->type = AST_TYPE_INT_VECTOR;
    } else if(param->type == AST_TYPE_CAR){
        param->type = AST_TYPE_CAR_VECTOR;
    }
    return param;
}

void yyerror(const char* s){
    int linha_erro = ultima_linha_token;

    (void)s;
    if(linha_token_anterior > 0 && linha_token_anterior < ultima_linha_token){
        linha_erro = ultima_linha_token - 1;
    }

    printf("ERRO: ERRO SINTATICO %d\n", linha_erro);
    exit(1);
}

int main(int argc, char **argv){
    int result;

    if(argc < 2){
        fprintf(stderr, "Uso: %s arquivo\n", argv[0]);
        return 1;
    }

    yyin = fopen(argv[1], "r");
    if(yyin == NULL){
        perror(argv[1]);
        return 1;
    }

    result = yyparse();
    if(result == 0){
        semantico_analisar(ast_root);
        gerador_codigo_gerar(ast_root, argv[1]);
    }
    ast_free(ast_root);
    fclose(yyin);

    return result;
}
