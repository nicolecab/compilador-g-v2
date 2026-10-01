#include "tabela_simbolos.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *copiar_texto(const char *texto){
    char *copia;
    size_t tamanho;

    if(texto == NULL){
        return NULL;
    }

    tamanho = strlen(texto) + 1;
    copia = malloc(tamanho);
    if(copia == NULL){
        fprintf(stderr, "Erro interno: memoria insuficiente\n");
        exit(1);
    }

    memcpy(copia, texto, tamanho);
    return copia;
}

static void liberar_tabela(TabelaSimbolos *tabela){
    EntradaTabelaSimbolos *entrada = tabela->entradas;
    EntradaTabelaSimbolos *proxima;
    ParametroSimbolo *parametro;
    ParametroSimbolo *proximo_parametro;

    while(entrada != NULL){
        proxima = entrada->proxima;
        parametro = entrada->parametros;
        while(parametro != NULL){
            proximo_parametro = parametro->proximo;
            free(parametro->nome);
            free(parametro);
            parametro = proximo_parametro;
        }
        free(entrada->nome);
        free(entrada);
        entrada = proxima;
    }

    free(tabela);
}

void pilha_tabela_iniciar(PilhaTabelaSimbolos *pilha){
    pilha->topo = NULL;
}

void pilha_tabela_empilhar(PilhaTabelaSimbolos *pilha){
    TabelaSimbolos *tabela = malloc(sizeof(TabelaSimbolos));

    if(tabela == NULL){
        fprintf(stderr, "Erro interno: memoria insuficiente\n");
        exit(1);
    }

    tabela->entradas = NULL;
    tabela->proxima = pilha->topo;
    pilha->topo = tabela;
}

void pilha_tabela_remover_escopo(PilhaTabelaSimbolos *pilha){
    TabelaSimbolos *topo = pilha->topo;

    if(topo == NULL){
        return;
    }

    pilha->topo = topo->proxima;
    liberar_tabela(topo);
}

void pilha_tabela_liberar(PilhaTabelaSimbolos *pilha){
    while(pilha->topo != NULL){
        pilha_tabela_remover_escopo(pilha);
    }
}

EntradaTabelaSimbolos *pilha_tabela_pesquisar_escopo_atual(PilhaTabelaSimbolos *pilha,
                                                           const char *nome){
    EntradaTabelaSimbolos *entrada;

    if(pilha->topo == NULL){
        return NULL;
    }

    entrada = pilha->topo->entradas;
    while(entrada != NULL){
        if(strcmp(entrada->nome, nome) == 0){
            return entrada;
        }
        entrada = entrada->proxima;
    }

    return NULL;
}

EntradaTabelaSimbolos *pilha_tabela_pesquisar(PilhaTabelaSimbolos *pilha,
                                              const char *nome){
    TabelaSimbolos *tabela = pilha->topo;
    EntradaTabelaSimbolos *entrada;

    while(tabela != NULL){
        entrada = tabela->entradas;
        while(entrada != NULL){
            if(strcmp(entrada->nome, nome) == 0){
                return entrada;
            }
            entrada = entrada->proxima;
        }
        tabela = tabela->proxima;
    }

    return NULL;
}

static EntradaTabelaSimbolos *inserir_entrada(PilhaTabelaSimbolos *pilha,
                                              const char *nome,
                                              ASTType tipo,
                                              int posicao,
                                              int linha,
                                              SimboloCategoria categoria){
    EntradaTabelaSimbolos *entrada;

    if(pilha->topo == NULL){
        pilha_tabela_empilhar(pilha);
    }

    entrada = malloc(sizeof(EntradaTabelaSimbolos));
    if(entrada == NULL){
        fprintf(stderr, "Erro interno: memoria insuficiente\n");
        exit(1);
    }

    entrada->nome = copiar_texto(nome);
    entrada->tipo = tipo;
    entrada->linha = linha;
    entrada->posicao = posicao;
    entrada->numero_argumentos = 0;
    entrada->categoria = categoria;
    entrada->parametros = NULL;
    entrada->proxima = pilha->topo->entradas;
    pilha->topo->entradas = entrada;

    return entrada;
}

EntradaTabelaSimbolos *pilha_tabela_inserir_variavel(PilhaTabelaSimbolos *pilha,
                                                     const char *nome,
                                                     ASTType tipo,
                                                     int posicao,
                                                     int linha){
    return inserir_entrada(pilha, nome, tipo, posicao, linha, SIMBOLO_VARIAVEL);
}

EntradaTabelaSimbolos *pilha_tabela_inserir_parametro(PilhaTabelaSimbolos *pilha,
                                                      const char *nome,
                                                      ASTType tipo,
                                                      int posicao,
                                                      int linha){
    return inserir_entrada(pilha, nome, tipo, posicao, linha, SIMBOLO_PARAMETRO);
}

EntradaTabelaSimbolos *pilha_tabela_inserir_funcao(PilhaTabelaSimbolos *pilha,
                                                   const char *nome,
                                                   int numero_argumentos,
                                                   ASTType tipo_retorno,
                                                   ASTNode *parametros,
                                                   int linha){
    EntradaTabelaSimbolos *funcao = inserir_entrada(pilha, nome, tipo_retorno, 0, linha, SIMBOLO_FUNCAO);
    ParametroSimbolo **destino = &funcao->parametros;
    ASTNode *atual = parametros;
    int posicao = 1;

    funcao->numero_argumentos = numero_argumentos;
    while(atual != NULL){
        ParametroSimbolo *parametro = malloc(sizeof(ParametroSimbolo));
        if(parametro == NULL){
            fprintf(stderr, "Erro interno: memoria insuficiente\n");
            exit(1);
        }
        parametro->nome = copiar_texto(atual->lexeme);
        parametro->tipo = atual->type;
        parametro->posicao = posicao;
        parametro->proximo = NULL;
        *destino = parametro;
        destino = &parametro->proximo;
        posicao++;
        atual = atual->next;
    }

    return funcao;
}
