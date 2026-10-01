# Compilador G-V2

Implementação de um compilador para a linguagem G-V2, a fim de estudar a construção de compiladores.

O compilador recebe programas escritos em G-V2 e gera código em assembly MIPS, passando por análise léxica, análise sintática, construção de AST, tabela de símbolos, análise semântica e geração de código.

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

## Etapas

- Análise léxica com Flex (`g-v2.l`);
- análise sintática e construção da AST com Bison (`g-v2.y`);
- representação da AST (`ast.h` e `ast.c`);
- tabela de símbolos com pilha de escopos (`tabela_simbolos.h` e `tabela_simbolos.c`);
- análise semântica (`semantico.h` e `semantico.c`);
- geração de código MIPS (`gerador_codigo.h` e `gerador_codigo.c`).

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
├── exemplos/
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
