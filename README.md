# Compilador G-V2

Este repositório contém a implementação de um compilador didático para a linguagem G-V2, desenvolvido como um projeto de estudo sobre construção de compiladores.

O objetivo do projeto é implementar, de forma integrada, as principais etapas de um compilador: análise léxica, análise sintática, construção de árvore sintática abstrata, tabela de símbolos, análise semântica e geração de código.

O compilador recebe programas escritos em G-V2 e gera código em assembly MIPS.

## Sobre a Linguagem

G-V2 é uma linguagem imperativa simples, com suporte a:

- tipos primitivos `int` e `car`;
- variáveis globais;
- variáveis locais;
- funções com parâmetros e valor de retorno;
- vetores de inteiros e caracteres;
- atribuições;
- expressões aritméticas, relacionais e lógicas;
- comandos condicionais;
- comandos de repetição;
- comandos de entrada e saída;
- cadeias de caracteres para saída.

Exemplo de programa:

```g
global [
    x:int;
]

funcao [
    soma(a:int, b:int):int {
        retorne a + b;
    }
]

principal {
    x = soma(1, 2);
    escreva "Resultado: ";
    escreva x;
    novalinha;
}
```

## Estrutura do Compilador

O compilador está organizado nas seguintes etapas:

1. **Análise léxica**  
   Implementada com Flex no arquivo `g-v2.l`.

2. **Análise sintática e construção da AST**  
   Implementada com Bison no arquivo `g-v2.y`.

3. **Árvore Sintática Abstrata**  
   Implementada em `ast.h` e `ast.c`.

4. **Tabela de símbolos**  
   Implementada como uma pilha de escopos em `tabela_simbolos.h` e `tabela_simbolos.c`.

5. **Análise semântica**  
   Implementada em `semantico.h` e `semantico.c`.

6. **Geração de código**  
   Implementada em `gerador_codigo.h` e `gerador_codigo.c`, gerando assembly MIPS.

## Estrutura dos Arquivos

```text
.
├── g-v2.l
├── g-v2.y
├── ast.c
├── ast.h
├── tabela_simbolos.c
├── tabela_simbolos.h
├── semantico.c
├── semantico.h
├── gerador_codigo.c
├── gerador_codigo.h
└── Makefile
```

## Requisitos

Para compilar o projeto, são necessários:

- GCC;
- Flex;
- Bison;
- Make.

## Compilação

Para compilar o compilador:

```bash
make
```

O comando gera o executável:

```bash
g-v2
```

## Execução

Para executar o compilador sobre um arquivo fonte:

```bash
./g-v2 caminho/do/arquivo.txt
```

Se o programa de entrada estiver correto, será gerado um arquivo `.asm` com o mesmo nome base do arquivo de entrada.

Exemplo:

```bash
./g-v2 exemplos/programa.txt
```

Saída esperada:

```text
exemplos/programa.asm
```

## Limpeza

Para remover arquivos gerados durante a compilação:

```bash
make clean
```

## Observações

Este projeto tem finalidade educacional e busca tornar explícitas as principais etapas internas de um compilador.

A geração de código tem como alvo assembly MIPS.
