#include "semantico.h"

#include "tabela_simbolos.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    ASTType retorno_atual;
} ContextoSemantico;

static void erro_semantico(int linha){
    printf("ERRO: ERRO SEMANTICO %d\n", linha);
    exit(1);
}

static int eh_vetor(ASTType tipo){
    return tipo == AST_TYPE_INT_VECTOR || tipo == AST_TYPE_CAR_VECTOR;
}

static ASTType tipo_base(ASTType tipo){
    if(tipo == AST_TYPE_INT_VECTOR){
        return AST_TYPE_INT;
    }
    if(tipo == AST_TYPE_CAR_VECTOR){
        return AST_TYPE_CAR;
    }
    return tipo;
}

static int operador_aritmetico(const char *op){
    return strcmp(op, "+") == 0 || strcmp(op, "-") == 0 ||
           strcmp(op, "*") == 0 || strcmp(op, "/") == 0;
}

static int operador_relacional(const char *op){
    return strcmp(op, "<") == 0 || strcmp(op, ">") == 0 ||
           strcmp(op, ">=") == 0 || strcmp(op, "<=") == 0 ||
           strcmp(op, "==") == 0 || strcmp(op, "!=") == 0;
}

static int operador_logico(const char *op){
    return strcmp(op, "||") == 0 || strcmp(op, "&") == 0;
}

static int contar(ASTNode *lista){
    int total = 0;
    while(lista != NULL){
        total++;
        lista = lista->next;
    }
    return total;
}

static ASTType analisar_expr(ASTNode *no, PilhaTabelaSimbolos *pilha);
static void analisar_comandos(ASTNode *comando, PilhaTabelaSimbolos *pilha, ContextoSemantico *ctx);

static void declarar_variaveis(ASTNode *decl, PilhaTabelaSimbolos *pilha){
    ASTNode *atual = decl;
    int posicao = 1;

    while(atual != NULL){
        if(pilha_tabela_pesquisar_escopo_atual(pilha, atual->lexeme) != NULL){
            erro_semantico(atual->line);
        }
        pilha_tabela_inserir_variavel(pilha, atual->lexeme, atual->type, posicao, atual->line);
        posicao++;
        atual = atual->next;
    }
}

static void declarar_parametros(ASTNode *param, PilhaTabelaSimbolos *pilha){
    ASTNode *atual = param;
    int posicao = 1;

    while(atual != NULL){
        if(pilha_tabela_pesquisar_escopo_atual(pilha, atual->lexeme) != NULL){
            erro_semantico(atual->line);
        }
        pilha_tabela_inserir_parametro(pilha, atual->lexeme, atual->type, posicao, atual->line);
        posicao++;
        atual = atual->next;
    }
}

static void analisar_bloco_no_escopo_atual(ASTNode *bloco, PilhaTabelaSimbolos *pilha, ContextoSemantico *ctx){
    declarar_variaveis(bloco->first, pilha);
    analisar_comandos(bloco->second, pilha, ctx);
}

static void analisar_bloco(ASTNode *bloco, PilhaTabelaSimbolos *pilha, ContextoSemantico *ctx){
    pilha_tabela_empilhar(pilha);
    analisar_bloco_no_escopo_atual(bloco, pilha, ctx);
    pilha_tabela_remover_escopo(pilha);
}

static EntradaTabelaSimbolos *buscar_identificador(PilhaTabelaSimbolos *pilha, ASTNode *no){
    EntradaTabelaSimbolos *entrada = pilha_tabela_pesquisar(pilha, no->lexeme);

    if(entrada == NULL || entrada->categoria == SIMBOLO_FUNCAO){
        erro_semantico(no->line);
    }
    return entrada;
}

static ASTType analisar_index(ASTNode *no, PilhaTabelaSimbolos *pilha){
    EntradaTabelaSimbolos *entrada = buscar_identificador(pilha, no);
    ASTType tipo_indice = analisar_expr(no->first, pilha);

    if(!eh_vetor(entrada->tipo) || tipo_indice != AST_TYPE_INT){
        erro_semantico(no->line);
    }
    no->type = tipo_base(entrada->tipo);
    return no->type;
}

static ASTType analisar_call(ASTNode *no, PilhaTabelaSimbolos *pilha){
    EntradaTabelaSimbolos *funcao = pilha_tabela_pesquisar(pilha, no->lexeme);
    ParametroSimbolo *formal;
    ASTNode *real;

    if(funcao == NULL || funcao->categoria != SIMBOLO_FUNCAO){
        erro_semantico(no->line);
    }
    if(contar(no->first) != funcao->numero_argumentos){
        erro_semantico(no->line);
    }

    formal = funcao->parametros;
    real = no->first;
    while(formal != NULL && real != NULL){
        ASTType tipo_real = analisar_expr(real, pilha);
        if(tipo_real != formal->tipo){
            erro_semantico(real->line);
        }
        formal = formal->proximo;
        real = real->next;
    }

    no->type = funcao->tipo;
    return no->type;
}

static ASTType analisar_atribuicao(ASTNode *no, PilhaTabelaSimbolos *pilha){
    ASTType tipo_alvo = analisar_expr(no->first, pilha);
    ASTType tipo_expr = analisar_expr(no->second, pilha);

    if(tipo_alvo != tipo_expr){
        erro_semantico(no->line);
    }
    no->type = tipo_alvo;
    return no->type;
}

static ASTType analisar_binario(ASTNode *no, PilhaTabelaSimbolos *pilha){
    ASTType tipo_esq = analisar_expr(no->first, pilha);
    ASTType tipo_dir = analisar_expr(no->second, pilha);

    if(operador_aritmetico(no->lexeme)){
        if(tipo_esq != AST_TYPE_INT || tipo_dir != AST_TYPE_INT){
            erro_semantico(no->line);
        }
        no->type = AST_TYPE_INT;
        return no->type;
    }

    if(operador_relacional(no->lexeme)){
        if(tipo_esq != tipo_dir){
            erro_semantico(no->line);
        }
        no->type = AST_TYPE_INT;
        return no->type;
    }

    if(operador_logico(no->lexeme)){
        if(tipo_esq != AST_TYPE_INT || tipo_dir != AST_TYPE_INT){
            erro_semantico(no->line);
        }
        no->type = AST_TYPE_INT;
        return no->type;
    }

    erro_semantico(no->line);
    return AST_TYPE_NONE;
}

static ASTType analisar_unario(ASTNode *no, PilhaTabelaSimbolos *pilha){
    ASTType tipo_expr = analisar_expr(no->first, pilha);

    if(tipo_expr != AST_TYPE_INT){
        erro_semantico(no->line);
    }
    no->type = AST_TYPE_INT;
    return no->type;
}

static ASTType analisar_expr(ASTNode *no, PilhaTabelaSimbolos *pilha){
    EntradaTabelaSimbolos *entrada;

    if(no == NULL){
        return AST_TYPE_NONE;
    }

    switch(no->kind){
        case AST_IDENTIFIER:
            entrada = buscar_identificador(pilha, no);
            no->type = entrada->tipo;
            return no->type;
        case AST_INDEX:
            return analisar_index(no, pilha);
        case AST_CALL:
            return analisar_call(no, pilha);
        case AST_INT_CONST:
            return AST_TYPE_INT;
        case AST_CAR_CONST:
            return AST_TYPE_CAR;
        case AST_ASSIGN:
            return analisar_atribuicao(no, pilha);
        case AST_BINARY:
            return analisar_binario(no, pilha);
        case AST_UNARY:
            return analisar_unario(no, pilha);
        default:
            erro_semantico(no->line);
            return AST_TYPE_NONE;
    }
}

static void analisar_comandos(ASTNode *comando, PilhaTabelaSimbolos *pilha, ContextoSemantico *ctx){
    ASTNode *atual = comando;

    while(atual != NULL){
        switch(atual->kind){
            case AST_BLOCK:
                analisar_bloco(atual, pilha, ctx);
                break;
            case AST_EMPTY_CMD:
            case AST_NEWLINE:
                break;
            case AST_READ:
                if(eh_vetor(analisar_expr(atual->first, pilha))){
                    erro_semantico(atual->line);
                }
                break;
            case AST_WRITE:
                if(atual->first != NULL && atual->first->kind != AST_STRING){
                    analisar_expr(atual->first, pilha);
                }
                break;
            case AST_RETURN:
                if(ctx->retorno_atual == AST_TYPE_NONE ||
                   analisar_expr(atual->first, pilha) != ctx->retorno_atual){
                    erro_semantico(atual->line);
                }
                break;
            case AST_IF:
                analisar_expr(atual->first, pilha);
                analisar_comandos(atual->second, pilha, ctx);
                analisar_comandos(atual->third, pilha, ctx);
                break;
            case AST_WHILE:
                analisar_expr(atual->first, pilha);
                analisar_comandos(atual->second, pilha, ctx);
                break;
            case AST_ASSIGN:
            case AST_BINARY:
            case AST_UNARY:
            case AST_IDENTIFIER:
            case AST_INDEX:
            case AST_CALL:
            case AST_INT_CONST:
            case AST_CAR_CONST:
                analisar_expr(atual, pilha);
                break;
            default:
                erro_semantico(atual->line);
                break;
        }

        atual = atual->next;
    }
}

static void declarar_funcoes(ASTNode *funcao, PilhaTabelaSimbolos *pilha){
    ASTNode *atual = funcao;

    while(atual != NULL){
        if(pilha_tabela_pesquisar_escopo_atual(pilha, atual->lexeme) != NULL){
            erro_semantico(atual->line);
        }
        pilha_tabela_inserir_funcao(pilha, atual->lexeme, contar(atual->first),
                                    atual->type, atual->first, atual->line);
        atual = atual->next;
    }
}

static void analisar_funcoes(ASTNode *funcao, PilhaTabelaSimbolos *pilha){
    ASTNode *atual = funcao;

    while(atual != NULL){
        ContextoSemantico ctx;

        ctx.retorno_atual = atual->type;
        pilha_tabela_empilhar(pilha);
        declarar_parametros(atual->first, pilha);
        analisar_bloco_no_escopo_atual(atual->second, pilha, &ctx);
        pilha_tabela_remover_escopo(pilha);
        atual = atual->next;
    }
}

void semantico_analisar(ASTNode *raiz){
    PilhaTabelaSimbolos pilha;
    ContextoSemantico ctx;

    pilha_tabela_iniciar(&pilha);
    ctx.retorno_atual = AST_TYPE_NONE;

    if(raiz != NULL){
        if(raiz->kind != AST_PROGRAM){
            erro_semantico(raiz->line);
        }
        pilha_tabela_empilhar(&pilha);
        declarar_variaveis(raiz->first, &pilha);
        declarar_funcoes(raiz->second, &pilha);
        analisar_funcoes(raiz->second, &pilha);
        analisar_bloco(raiz->third, &pilha, &ctx);
        pilha_tabela_remover_escopo(&pilha);
    }

    pilha_tabela_liberar(&pilha);
}
