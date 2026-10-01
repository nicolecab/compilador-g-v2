CC = gcc
BISON = bison
FLEX = flex
CFLAGS = -Wall -Wextra -g

TARGET = g-v2

.PHONY: all clean

all: $(TARGET)

$(TARGET): g-v2.tab.o lex.yy.o ast.o tabela_simbolos.o semantico.o gerador_codigo.o
	$(CC) $(CFLAGS) -o $@ g-v2.tab.o lex.yy.o ast.o tabela_simbolos.o semantico.o gerador_codigo.o

g-v2.tab.c g-v2.tab.h: g-v2.y
	$(BISON) -d g-v2.y

lex.yy.c: g-v2.l g-v2.tab.h
	$(FLEX) g-v2.l

g-v2.tab.o: g-v2.tab.c
	$(CC) $(CFLAGS) -c g-v2.tab.c

lex.yy.o: lex.yy.c
	$(CC) $(CFLAGS) -c lex.yy.c

ast.o: ast.c ast.h
	$(CC) $(CFLAGS) -c ast.c

tabela_simbolos.o: tabela_simbolos.c tabela_simbolos.h ast.h
	$(CC) $(CFLAGS) -c tabela_simbolos.c

semantico.o: semantico.c semantico.h tabela_simbolos.h ast.h
	$(CC) $(CFLAGS) -c semantico.c

gerador_codigo.o: gerador_codigo.c gerador_codigo.h ast.h
	$(CC) $(CFLAGS) -c gerador_codigo.c

clean:
	rm -f $(TARGET) g-v2.tab.c g-v2.tab.h g-v2.tab.o lex.yy.c lex.yy.o ast.o tabela_simbolos.o semantico.o gerador_codigo.o
