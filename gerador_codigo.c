#include "gerador_codigo.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct SimboloCodigo {
    char *nome;
    char *rotulo;
    ASTType tipo;
    int tamanho;
    int parametro;
    int deslocamento;
    struct SimboloCodigo *proximo;
} SimboloCodigo;

typedef struct EscopoCodigo {
    SimboloCodigo *simbolos;
    struct EscopoCodigo *proximo;
} EscopoCodigo;

typedef struct {
    FILE *saida;
    EscopoCodigo *escopos;
    int proximo_rotulo;
    int proxima_string;
} ContextoCodigo;

static int contar_lista(ASTNode *node){
    int total = 0;
    while(node != NULL){
        total++;
        node = node->next;
    }
    return total;
}

static char *copiar_texto(const char *texto){
    char *copia;
    size_t tamanho;

    tamanho = strlen(texto) + 1;
    copia = malloc(tamanho);
    if(copia == NULL){
        fprintf(stderr, "Erro interno: memoria insuficiente\n");
        exit(1);
    }

    memcpy(copia, texto, tamanho);
    return copia;
}

static char *formatar_texto(const char *formato, int valor){
    char buffer[64];

    snprintf(buffer, sizeof(buffer), formato, valor);
    return copiar_texto(buffer);
}

static char *nome_saida(const char *entrada){
    const char *barra = strrchr(entrada, '/');
    const char *ponto = strrchr(entrada, '.');
    char *saida;
    size_t tamanho_base;
    size_t tamanho_saida;

    if(ponto != NULL && (barra == NULL || ponto > barra)){
        tamanho_base = (size_t)(ponto - entrada);
    } else {
        tamanho_base = strlen(entrada);
    }

    tamanho_saida = tamanho_base + strlen(".asm") + 1;
    saida = malloc(tamanho_saida);
    if(saida == NULL){
        fprintf(stderr, "Erro interno: memoria insuficiente\n");
        exit(1);
    }

    memcpy(saida, entrada, tamanho_base);
    strcpy(saida + tamanho_base, ".asm");

    return saida;
}

static void empilhar_escopo(ContextoCodigo *ctx){
    EscopoCodigo *escopo = malloc(sizeof(EscopoCodigo));

    if(escopo == NULL){
        fprintf(stderr, "Erro interno: memoria insuficiente\n");
        exit(1);
    }

    escopo->simbolos = NULL;
    escopo->proximo = ctx->escopos;
    ctx->escopos = escopo;
}

static void remover_escopo(ContextoCodigo *ctx){
    EscopoCodigo *escopo = ctx->escopos;
    SimboloCodigo *simbolo;
    SimboloCodigo *proximo;

    if(escopo == NULL){
        return;
    }

    simbolo = escopo->simbolos;
    while(simbolo != NULL){
        proximo = simbolo->proximo;
        free(simbolo->nome);
        free(simbolo->rotulo);
        free(simbolo);
        simbolo = proximo;
    }

    ctx->escopos = escopo->proximo;
    free(escopo);
}

static SimboloCodigo *inserir_simbolo(ContextoCodigo *ctx, ASTNode *decl){
    SimboloCodigo *simbolo = malloc(sizeof(SimboloCodigo));

    if(simbolo == NULL){
        fprintf(stderr, "Erro interno: memoria insuficiente\n");
        exit(1);
    }

    simbolo->nome = copiar_texto(decl->lexeme);
    simbolo->rotulo = formatar_texto("_gv2_var_%d", ctx->proximo_rotulo++);
    simbolo->tipo = decl->type;
    simbolo->tamanho = 1;
    simbolo->parametro = 0;
    simbolo->deslocamento = 0;
    if(decl->first != NULL && decl->first->kind == AST_INT_CONST){
        simbolo->tamanho = atoi(decl->first->lexeme);
        if(simbolo->tamanho < 1){
            simbolo->tamanho = 1;
        }
    }
    simbolo->proximo = ctx->escopos->simbolos;
    ctx->escopos->simbolos = simbolo;

    fprintf(ctx->saida, ".data\n");
    if(simbolo->tipo == AST_TYPE_CAR){
        fprintf(ctx->saida, "%s: .byte 0\n", simbolo->rotulo);
    } else if(simbolo->tipo == AST_TYPE_CAR_VECTOR){
        fprintf(ctx->saida, "%s: .space %d\n", simbolo->rotulo, simbolo->tamanho);
    } else if(simbolo->tipo == AST_TYPE_INT_VECTOR){
        fprintf(ctx->saida, "%s: .space %d\n", simbolo->rotulo, simbolo->tamanho * 4);
    } else {
        fprintf(ctx->saida, "%s: .word 0\n", simbolo->rotulo);
    }
    fprintf(ctx->saida, ".text\n");

    return simbolo;
}

static SimboloCodigo *inserir_parametro(ContextoCodigo *ctx, ASTNode *param, int deslocamento){
    SimboloCodigo *simbolo = malloc(sizeof(SimboloCodigo));

    if(simbolo == NULL){
        fprintf(stderr, "Erro interno: memoria insuficiente\n");
        exit(1);
    }

    simbolo->nome = copiar_texto(param->lexeme);
    simbolo->rotulo = NULL;
    simbolo->tipo = param->type;
    simbolo->tamanho = 1;
    simbolo->parametro = 1;
    simbolo->deslocamento = deslocamento;
    simbolo->proximo = ctx->escopos->simbolos;
    ctx->escopos->simbolos = simbolo;

    return simbolo;
}

static void gerar_parametros(ASTNode *param, ContextoCodigo *ctx){
    int total = contar_lista(param);
    int posicao = 0;

    while(param != NULL){
        inserir_parametro(ctx, param, (total - posicao - 1) * 4);
        posicao++;
        param = param->next;
    }
}

static SimboloCodigo *buscar_simbolo(ContextoCodigo *ctx, const char *nome){
    EscopoCodigo *escopo = ctx->escopos;
    SimboloCodigo *simbolo;

    while(escopo != NULL){
        simbolo = escopo->simbolos;
        while(simbolo != NULL){
            if(strcmp(simbolo->nome, nome) == 0){
                return simbolo;
            }
            simbolo = simbolo->proximo;
        }
        escopo = escopo->proximo;
    }

    return NULL;
}

static int valor_caractere(const char *lexema){
    const char *p = lexema + 1;

    if(*p == '\\'){
        p++;
        switch(*p){
            case 'n': return '\n';
            case 't': return '\t';
            case 'r': return '\r';
            case '0': return '\0';
            case '\\': return '\\';
            case '\'': return '\'';
            case '"': return '"';
            default: return (unsigned char)*p;
        }
    }

    return (unsigned char)*p;
}

static void emitir_string(ContextoCodigo *ctx, const char *lexema){
    const char *p = lexema;

    fputc('"', ctx->saida);
    if(*p == '"'){
        p++;
    }
    while(*p != '\0'){
        if(*p == '"' && p[1] == '\0'){
            break;
        }
        if(*p == '"'){
            fputs("\\\"", ctx->saida);
        } else if(*p == '\\'){
            fputc('\\', ctx->saida);
            p++;
            if(*p == '\0'){
                break;
            }
            fputc(*p, ctx->saida);
        } else {
            fputc(*p, ctx->saida);
        }
        p++;
    }
    fputc('"', ctx->saida);
}

static void gerar_expr(ASTNode *no, ContextoCodigo *ctx);
static void gerar_comandos(ASTNode *comando, ContextoCodigo *ctx);

static void carregar_base_vetor(ContextoCodigo *ctx, SimboloCodigo *simbolo){
    if(simbolo->parametro){
        fprintf(ctx->saida, "    lw $t1, %d($fp)\n", simbolo->deslocamento);
    } else {
        fprintf(ctx->saida, "    la $t1, %s\n", simbolo->rotulo);
    }
}

static void salvar_t0(ContextoCodigo *ctx){
    fprintf(ctx->saida, "    addiu $sp, $sp, -4\n");
    fprintf(ctx->saida, "    sw $t0, 0($sp)\n");
}

static void restaurar_t1(ContextoCodigo *ctx){
    fprintf(ctx->saida, "    lw $t1, 0($sp)\n");
    fprintf(ctx->saida, "    addiu $sp, $sp, 4\n");
}

static void gerar_binario(ASTNode *no, ContextoCodigo *ctx){
    gerar_expr(no->first, ctx);
    salvar_t0(ctx);
    gerar_expr(no->second, ctx);
    restaurar_t1(ctx);

    if(strcmp(no->lexeme, "+") == 0){
        fprintf(ctx->saida, "    add $t0, $t1, $t0\n");
    } else if(strcmp(no->lexeme, "-") == 0){
        fprintf(ctx->saida, "    sub $t0, $t1, $t0\n");
    } else if(strcmp(no->lexeme, "*") == 0){
        fprintf(ctx->saida, "    mul $t0, $t1, $t0\n");
    } else if(strcmp(no->lexeme, "/") == 0){
        fprintf(ctx->saida, "    div $t1, $t0\n");
        fprintf(ctx->saida, "    mflo $t0\n");
    } else if(strcmp(no->lexeme, "<") == 0){
        fprintf(ctx->saida, "    slt $t0, $t1, $t0\n");
    } else if(strcmp(no->lexeme, ">") == 0){
        fprintf(ctx->saida, "    slt $t0, $t0, $t1\n");
    } else if(strcmp(no->lexeme, "<=") == 0){
        fprintf(ctx->saida, "    slt $t0, $t0, $t1\n");
        fprintf(ctx->saida, "    xori $t0, $t0, 1\n");
    } else if(strcmp(no->lexeme, ">=") == 0){
        fprintf(ctx->saida, "    slt $t0, $t1, $t0\n");
        fprintf(ctx->saida, "    xori $t0, $t0, 1\n");
    } else if(strcmp(no->lexeme, "==") == 0){
        fprintf(ctx->saida, "    seq $t0, $t1, $t0\n");
    } else if(strcmp(no->lexeme, "!=") == 0){
        fprintf(ctx->saida, "    sne $t0, $t1, $t0\n");
    } else if(strcmp(no->lexeme, "||") == 0){
        fprintf(ctx->saida, "    sne $t1, $t1, $zero\n");
        fprintf(ctx->saida, "    sne $t0, $t0, $zero\n");
        fprintf(ctx->saida, "    or $t0, $t1, $t0\n");
    } else if(strcmp(no->lexeme, "&") == 0){
        fprintf(ctx->saida, "    sne $t1, $t1, $zero\n");
        fprintf(ctx->saida, "    sne $t0, $t0, $zero\n");
        fprintf(ctx->saida, "    and $t0, $t1, $t0\n");
    }
}

static void gerar_expr(ASTNode *no, ContextoCodigo *ctx){
    SimboloCodigo *simbolo;

    switch(no->kind){
        case AST_IDENTIFIER:
            simbolo = buscar_simbolo(ctx, no->lexeme);
            if(simbolo->parametro){
                fprintf(ctx->saida, "    lw $t0, %d($fp)\n", simbolo->deslocamento);
            } else if(simbolo->tipo == AST_TYPE_CAR){
                fprintf(ctx->saida, "    lb $t0, %s\n", simbolo->rotulo);
            } else if(simbolo->tipo == AST_TYPE_CAR_VECTOR || simbolo->tipo == AST_TYPE_INT_VECTOR){
                fprintf(ctx->saida, "    la $t0, %s\n", simbolo->rotulo);
            } else {
                fprintf(ctx->saida, "    lw $t0, %s\n", simbolo->rotulo);
            }
            break;
        case AST_INDEX:
            simbolo = buscar_simbolo(ctx, no->lexeme);
            gerar_expr(no->first, ctx);
            if(simbolo->tipo == AST_TYPE_INT_VECTOR){
                fprintf(ctx->saida, "    sll $t0, $t0, 2\n");
            }
            carregar_base_vetor(ctx, simbolo);
            fprintf(ctx->saida, "    addu $t1, $t1, $t0\n");
            if(simbolo->tipo == AST_TYPE_CAR_VECTOR){
                fprintf(ctx->saida, "    lb $t0, 0($t1)\n");
            } else {
                fprintf(ctx->saida, "    lw $t0, 0($t1)\n");
            }
            break;
        case AST_INT_CONST:
            fprintf(ctx->saida, "    li $t0, %s\n", no->lexeme);
            break;
        case AST_CAR_CONST:
            fprintf(ctx->saida, "    li $t0, %d\n", valor_caractere(no->lexeme));
            break;
        case AST_ASSIGN:
            gerar_expr(no->second, ctx);
            if(no->first->kind == AST_INDEX){
                salvar_t0(ctx);
                simbolo = buscar_simbolo(ctx, no->first->lexeme);
                gerar_expr(no->first->first, ctx);
                if(simbolo->tipo == AST_TYPE_INT_VECTOR){
                    fprintf(ctx->saida, "    sll $t0, $t0, 2\n");
                }
                carregar_base_vetor(ctx, simbolo);
                fprintf(ctx->saida, "    addu $t0, $t1, $t0\n");
                restaurar_t1(ctx);
                if(simbolo->tipo == AST_TYPE_CAR_VECTOR){
                    fprintf(ctx->saida, "    sb $t1, 0($t0)\n");
                } else {
                    fprintf(ctx->saida, "    sw $t1, 0($t0)\n");
                }
            } else {
                simbolo = buscar_simbolo(ctx, no->first->lexeme);
                if(simbolo->parametro){
                    fprintf(ctx->saida, "    sw $t0, %d($fp)\n", simbolo->deslocamento);
                } else if(simbolo->tipo == AST_TYPE_CAR){
                    fprintf(ctx->saida, "    sb $t0, %s\n", simbolo->rotulo);
                } else {
                    fprintf(ctx->saida, "    sw $t0, %s\n", simbolo->rotulo);
                }
            }
            break;
        case AST_CALL:
        {
            int total = contar_lista(no->first);
            ASTNode *arg = no->first;
            fprintf(ctx->saida, "    addiu $sp, $sp, -4\n");
            fprintf(ctx->saida, "    sw $fp, 0($sp)\n");
            while(arg != NULL){
                gerar_expr(arg, ctx);
                fprintf(ctx->saida, "    addiu $sp, $sp, -4\n");
                fprintf(ctx->saida, "    sw $t0, 0($sp)\n");
                arg = arg->next;
            }
            fprintf(ctx->saida, "    jal %s\n", no->lexeme);
            if(total > 0){
                fprintf(ctx->saida, "    addiu $sp, $sp, %d\n", total * 4);
            }
            fprintf(ctx->saida, "    lw $fp, 0($sp)\n");
            fprintf(ctx->saida, "    addiu $sp, $sp, 4\n");
            fprintf(ctx->saida, "    move $t0, $v0\n");
            break;
        }
        case AST_RETURN:
            gerar_expr(no->first, ctx);
            fprintf(ctx->saida, "    move $v0, $t0\n");
            fprintf(ctx->saida, "    lw $ra, 0($sp)\n");
            fprintf(ctx->saida, "    addiu $sp, $sp, 4\n");
            fprintf(ctx->saida, "    jr $ra\n");
            break;
        case AST_BINARY:
            gerar_binario(no, ctx);
            break;
        case AST_UNARY:
            gerar_expr(no->first, ctx);
            if(strcmp(no->lexeme, "-") == 0){
                fprintf(ctx->saida, "    sub $t0, $zero, $t0\n");
            } else {
                fprintf(ctx->saida, "    seq $t0, $t0, $zero\n");
            }
            break;
        default:
            break;
    }
}

static void gerar_declaracoes(ASTNode *decl, ContextoCodigo *ctx){
    ASTNode *atual = decl;

    while(atual != NULL){
        inserir_simbolo(ctx, atual);
        atual = atual->next;
    }
}

static void gerar_bloco(ASTNode *bloco, ContextoCodigo *ctx){
    empilhar_escopo(ctx);
    gerar_declaracoes(bloco->first, ctx);
    gerar_comandos(bloco->second, ctx);
    remover_escopo(ctx);
}

static void gerar_leitura(ASTNode *no, ContextoCodigo *ctx){
    ASTNode *alvo = no->first;
    SimboloCodigo *simbolo = buscar_simbolo(ctx, alvo->lexeme);

    if(simbolo->tipo == AST_TYPE_CAR){
        fprintf(ctx->saida, "    li $v0, 12\n");
        fprintf(ctx->saida, "    syscall\n");
        if(simbolo->parametro){
            fprintf(ctx->saida, "    sw $v0, %d($fp)\n", simbolo->deslocamento);
        } else {
            fprintf(ctx->saida, "    sb $v0, %s\n", simbolo->rotulo);
        }
    } else if(alvo->kind == AST_INDEX){
        gerar_expr(alvo->first, ctx);
        if(simbolo->tipo == AST_TYPE_INT_VECTOR){
            fprintf(ctx->saida, "    sll $t0, $t0, 2\n");
        }
        carregar_base_vetor(ctx, simbolo);
        fprintf(ctx->saida, "    addu $t1, $t1, $t0\n");
        fprintf(ctx->saida, "    li $v0, %d\n", simbolo->tipo == AST_TYPE_CAR_VECTOR ? 12 : 5);
        fprintf(ctx->saida, "    syscall\n");
        fprintf(ctx->saida, "    %s $v0, 0($t1)\n", simbolo->tipo == AST_TYPE_CAR_VECTOR ? "sb" : "sw");
    } else {
        fprintf(ctx->saida, "    li $v0, 5\n");
        fprintf(ctx->saida, "    syscall\n");
        if(simbolo->parametro){
            fprintf(ctx->saida, "    sw $v0, %d($fp)\n", simbolo->deslocamento);
        } else {
            fprintf(ctx->saida, "    sw $v0, %s\n", simbolo->rotulo);
        }
    }
}

static void gerar_escrita(ASTNode *no, ContextoCodigo *ctx){
    char *rotulo_string;

    if(no->first != NULL && no->first->kind == AST_STRING){
        rotulo_string = formatar_texto("_gv2_str_%d", ctx->proxima_string++);
        fprintf(ctx->saida, ".data\n%s: .asciiz ", rotulo_string);
        emitir_string(ctx, no->first->lexeme);
        fprintf(ctx->saida, "\n.text\n");
        fprintf(ctx->saida, "    la $a0, %s\n", rotulo_string);
        fprintf(ctx->saida, "    li $v0, 4\n");
        fprintf(ctx->saida, "    syscall\n");
        free(rotulo_string);
        return;
    }

    gerar_expr(no->first, ctx);
    fprintf(ctx->saida, "    move $a0, $t0\n");
    if(no->first->type == AST_TYPE_CAR){
        fprintf(ctx->saida, "    li $v0, 11\n");
    } else {
        fprintf(ctx->saida, "    li $v0, 1\n");
    }
    fprintf(ctx->saida, "    syscall\n");
}

static void gerar_se(ASTNode *no, ContextoCodigo *ctx){
    int id = ctx->proximo_rotulo++;

    gerar_expr(no->first, ctx);
    if(no->third != NULL){
        fprintf(ctx->saida, "    beq $t0, $zero, _gv2_senao_%d\n", id);
        gerar_comandos(no->second, ctx);
        fprintf(ctx->saida, "    j _gv2_fimse_%d\n", id);
        fprintf(ctx->saida, "_gv2_senao_%d:\n", id);
        gerar_comandos(no->third, ctx);
        fprintf(ctx->saida, "_gv2_fimse_%d:\n", id);
    } else {
        fprintf(ctx->saida, "    beq $t0, $zero, _gv2_fimse_%d\n", id);
        gerar_comandos(no->second, ctx);
        fprintf(ctx->saida, "_gv2_fimse_%d:\n", id);
    }
}

static void gerar_enquanto(ASTNode *no, ContextoCodigo *ctx){
    int id = ctx->proximo_rotulo++;

    fprintf(ctx->saida, "_gv2_enquanto_%d:\n", id);
    gerar_expr(no->first, ctx);
    fprintf(ctx->saida, "    beq $t0, $zero, _gv2_fimenquanto_%d\n", id);
    gerar_comandos(no->second, ctx);
    fprintf(ctx->saida, "    j _gv2_enquanto_%d\n", id);
    fprintf(ctx->saida, "_gv2_fimenquanto_%d:\n", id);
}

static void gerar_comandos(ASTNode *comando, ContextoCodigo *ctx){
    ASTNode *atual = comando;

    while(atual != NULL){
        switch(atual->kind){
            case AST_BLOCK:
                gerar_bloco(atual, ctx);
                break;
            case AST_READ:
                gerar_leitura(atual, ctx);
                break;
            case AST_WRITE:
                gerar_escrita(atual, ctx);
                break;
            case AST_NEWLINE:
                fprintf(ctx->saida, "    li $a0, 10\n");
                fprintf(ctx->saida, "    li $v0, 11\n");
                fprintf(ctx->saida, "    syscall\n");
                break;
            case AST_IF:
                gerar_se(atual, ctx);
                break;
            case AST_WHILE:
                gerar_enquanto(atual, ctx);
                break;
            case AST_ASSIGN:
            case AST_BINARY:
            case AST_UNARY:
            case AST_IDENTIFIER:
            case AST_INDEX:
            case AST_CALL:
            case AST_RETURN:
            case AST_INT_CONST:
            case AST_CAR_CONST:
                gerar_expr(atual, ctx);
                break;
            case AST_EMPTY_CMD:
            default:
                break;
        }
        atual = atual->next;
    }
}

void gerador_codigo_gerar(ASTNode *raiz, const char *arquivo_entrada){
    ContextoCodigo ctx;
    char *arquivo_saida = nome_saida(arquivo_entrada);

    ctx.saida = fopen(arquivo_saida, "w");
    if(ctx.saida == NULL){
        perror(arquivo_saida);
        free(arquivo_saida);
        exit(1);
    }
    ctx.escopos = NULL;
    ctx.proximo_rotulo = 0;
    ctx.proxima_string = 0;

    fprintf(ctx.saida, ".text\n");
    fprintf(ctx.saida, ".globl main\n");
    if(raiz != NULL && raiz->kind == AST_PROGRAM){
        empilhar_escopo(&ctx);
        gerar_declaracoes(raiz->first, &ctx);
        ASTNode *funcao = raiz->second;
        while(funcao != NULL){
            fprintf(ctx.saida, "%s:\n", funcao->lexeme);
            empilhar_escopo(&ctx);
            gerar_parametros(funcao->first, &ctx);
            fprintf(ctx.saida, "    move $fp, $sp\n");
            fprintf(ctx.saida, "    addiu $sp, $sp, -4\n");
            fprintf(ctx.saida, "    sw $ra, 0($sp)\n");
            gerar_declaracoes(funcao->second->first, &ctx);
            gerar_comandos(funcao->second->second, &ctx);
            fprintf(ctx.saida, "    lw $ra, 0($sp)\n");
            fprintf(ctx.saida, "    addiu $sp, $sp, 4\n");
            remover_escopo(&ctx);
            fprintf(ctx.saida, "    jr $ra\n");
            funcao = funcao->next;
        }
    }
    fprintf(ctx.saida, "main:\n");
    if(raiz != NULL && raiz->kind == AST_PROGRAM){
        gerar_bloco(raiz->third, &ctx);
        remover_escopo(&ctx);
    }
    fprintf(ctx.saida, "    li $v0, 10\n");
    fprintf(ctx.saida, "    syscall\n");

    while(ctx.escopos != NULL){
        remover_escopo(&ctx);
    }
    fclose(ctx.saida);
    free(arquivo_saida);
}
