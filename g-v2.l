%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "g-v2.tab.h"

static int string_start_line;
static int can_start_signed_const = 1;
int ultima_linha_token = 1;
int linha_token_anterior = 1;

static TokenInfo make_token(void){
    TokenInfo token;
    token.text = malloc(strlen(yytext) + 1);
    if(token.text == NULL){
        fprintf(stderr, "Erro interno: memoria insuficiente\n");
        exit(1);
    }
    strcpy(token.text, yytext);
    token.line = yylineno;
    return token;
}

#define YY_USER_ACTION yylloc.first_line = yylineno; yylloc.last_line = yylineno;
#define RETURN_TOKEN(token) do { \
    linha_token_anterior = ultima_linha_token; \
    ultima_linha_token = yylineno; \
    return token; \
} while(0)
%}

%x comentario
%x string_aberta
%option yylineno
%option noyywrap
%option noinput nounput
%%
"/*"                       {BEGIN(comentario);}
<comentario>[^*\n]+        {}
<comentario>"*"+[^/\n]     {}
<comentario>\n             {}
<comentario>"*"+"/"        {BEGIN(INITIAL);}
<comentario><<EOF>> {
    printf("ERRO: COMENTARIO NAO TERMINA %d\n", yylineno);
    exit(1);
}
"||"            {can_start_signed_const = 1; RETURN_TOKEN(OU);}
"&"             {can_start_signed_const = 1; RETURN_TOKEN(E);}
"=="            {can_start_signed_const = 1; RETURN_TOKEN(IGUAL);}
"!="            {can_start_signed_const = 1; RETURN_TOKEN(DIFERENTE);}
">="            {can_start_signed_const = 1; RETURN_TOKEN(MAIORIGUAL);}
"<="            {can_start_signed_const = 1; RETURN_TOKEN(MENORIGUAL);}
"{"             {can_start_signed_const = 1; RETURN_TOKEN('{');}
"}"             {can_start_signed_const = 0; RETURN_TOKEN('}');}
"["             {can_start_signed_const = 1; RETURN_TOKEN('[');}
"]"             {can_start_signed_const = 0; RETURN_TOKEN(']');}
":"             {can_start_signed_const = 1; RETURN_TOKEN(':');}
";"             {can_start_signed_const = 1; RETURN_TOKEN(';');}
","             {can_start_signed_const = 1; RETURN_TOKEN(',');}
"("             {can_start_signed_const = 1; RETURN_TOKEN('(');}
")"             {can_start_signed_const = 0; RETURN_TOKEN(')');}
"="             {can_start_signed_const = 1; RETURN_TOKEN('=');}
">"             {can_start_signed_const = 1; RETURN_TOKEN('>');}
"<"             {can_start_signed_const = 1; RETURN_TOKEN('<');}
"+"             {can_start_signed_const = 1; RETURN_TOKEN('+');}
"-"             {can_start_signed_const = 1; RETURN_TOKEN('-');}
"*"             {can_start_signed_const = 1; RETURN_TOKEN('*');}
"/"             {can_start_signed_const = 1; RETURN_TOKEN('/');}
"!"             {can_start_signed_const = 1; RETURN_TOKEN('!');}
"principal"     {can_start_signed_const = 1; RETURN_TOKEN(PRINCIPAL);}
"global"        {can_start_signed_const = 1; RETURN_TOKEN(GLOBAL);}
"funcao"        {can_start_signed_const = 1; RETURN_TOKEN(FUNCAO);}
"int"           {can_start_signed_const = 0; RETURN_TOKEN(INT);}
"car"           {can_start_signed_const = 0; RETURN_TOKEN(CAR);}
"retorne"       {can_start_signed_const = 1; RETURN_TOKEN(RETORNE);}
"leia"          {can_start_signed_const = 1; RETURN_TOKEN(LEIA);}
"escreva"       {can_start_signed_const = 1; RETURN_TOKEN(ESCREVA);}
"novalinha"     {can_start_signed_const = 1; RETURN_TOKEN(NOVALINHA);}
"se"            {can_start_signed_const = 1; RETURN_TOKEN(SE);}
"entao"         {can_start_signed_const = 1; RETURN_TOKEN(ENTAO);}
"senao"         {can_start_signed_const = 1; RETURN_TOKEN(SENAO);}
"fimse"         {can_start_signed_const = 1; RETURN_TOKEN(FIMSE);}
"enquanto"      {can_start_signed_const = 1; RETURN_TOKEN(ENQUANTO);}
[+-][0-9]+ {
    if(can_start_signed_const){
        can_start_signed_const = 0;
        yylval.token = make_token();
        RETURN_TOKEN(INTCONST);
    }
    yyless(1);
    can_start_signed_const = 1;
    RETURN_TOKEN(yytext[0]);
}
[0-9]+                         {can_start_signed_const = 0; yylval.token = make_token(); RETURN_TOKEN(INTCONST);}
\'([^\\\n]|(\\.))\'             {can_start_signed_const = 0; yylval.token = make_token(); RETURN_TOKEN(CARCONST);}
\"                              {string_start_line = yylineno; yymore(); BEGIN(string_aberta);}
<string_aberta>[^\n"]*\"        {can_start_signed_const = 0; yylval.token = make_token(); BEGIN(INITIAL); RETURN_TOKEN(CADEIACARACTERES);}
<string_aberta>[^\n"]*\n {
    printf("ERRO: CADEIA DE CARACTERES OCUPA MAIS DE UMA LINHA %d\n", string_start_line);
    exit(1);
}
<string_aberta><<EOF>> {
    printf("ERRO: CADEIA DE CARACTERES OCUPA MAIS DE UMA LINHA %d\n", string_start_line);
    exit(1);
}
[a-zA-Z_][a-zA-Z0-9_]*          {can_start_signed_const = 0; yylval.token = make_token(); RETURN_TOKEN(IDENTIFICADOR);}
[ \t\r]+                        {}
\n                              {}
<<EOF>> {
    linha_token_anterior = ultima_linha_token;
    ultima_linha_token = yylineno;
    return -1;
}
. {
    printf("ERRO: CARACTERE INVALIDO %d\n", yylineno);
    exit(1);
}
%%
