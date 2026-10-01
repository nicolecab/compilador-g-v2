#ifndef TABELA_SIMBOLOS_H
#define TABELA_SIMBOLOS_H

#include "ast.h"

typedef enum {
    SIMBOLO_VARIAVEL,
    SIMBOLO_PARAMETRO,
    SIMBOLO_FUNCAO
} SimboloCategoria;

typedef struct ParametroSimbolo {
    char *nome;
    ASTType tipo;
    int posicao;
    struct ParametroSimbolo *proximo;
} ParametroSimbolo;

typedef struct EntradaTabelaSimbolos {
    char *nome;
    ASTType tipo;
    int linha;
    int posicao;
    int numero_argumentos;
    SimboloCategoria categoria;
    ParametroSimbolo *parametros;
    struct EntradaTabelaSimbolos *proxima;
} EntradaTabelaSimbolos;

typedef struct TabelaSimbolos {
    EntradaTabelaSimbolos *entradas;
    struct TabelaSimbolos *proxima;
} TabelaSimbolos;

typedef struct {
    TabelaSimbolos *topo;
} PilhaTabelaSimbolos;

void pilha_tabela_iniciar(PilhaTabelaSimbolos *pilha);
void pilha_tabela_empilhar(PilhaTabelaSimbolos *pilha);
void pilha_tabela_remover_escopo(PilhaTabelaSimbolos *pilha);
void pilha_tabela_liberar(PilhaTabelaSimbolos *pilha);

EntradaTabelaSimbolos *pilha_tabela_inserir_variavel(PilhaTabelaSimbolos *pilha,
                                                     const char *nome,
                                                     ASTType tipo,
                                                     int posicao,
                                                     int linha);
EntradaTabelaSimbolos *pilha_tabela_inserir_parametro(PilhaTabelaSimbolos *pilha,
                                                      const char *nome,
                                                      ASTType tipo,
                                                      int posicao,
                                                      int linha);
EntradaTabelaSimbolos *pilha_tabela_inserir_funcao(PilhaTabelaSimbolos *pilha,
                                                   const char *nome,
                                                   int numero_argumentos,
                                                   ASTType tipo_retorno,
                                                   ASTNode *parametros,
                                                   int linha);
EntradaTabelaSimbolos *pilha_tabela_pesquisar(PilhaTabelaSimbolos *pilha,
                                              const char *nome);
EntradaTabelaSimbolos *pilha_tabela_pesquisar_escopo_atual(PilhaTabelaSimbolos *pilha,
                                                           const char *nome);

#endif
