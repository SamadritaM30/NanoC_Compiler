CC=gcc
CFLAGS=-Wall -Wextra -O2

all: compiler codegen

compiler: lex.yy.c parser.tab.c
	$(CC) $(CFLAGS) lex.yy.c parser.tab.c -o compiler

lex.yy.c: lexer.l parser.tab.h
	flex lexer.l

parser.tab.c parser.tab.h: parser.y
	bison -d parser.y

codegen: codegen.c
	$(CC) $(CFLAGS) codegen.c -o codegen

run: compiler codegen
	./compiler test1.c | ./codegen > assembly.asm
	@echo "Assembly written to assembly.asm"

clean:
	rm -f compiler codegen lex.yy.c parser.tab.c parser.tab.h assembly.asm *.o