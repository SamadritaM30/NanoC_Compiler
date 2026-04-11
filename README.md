# NanoC_Compiler

Design and Implementation of a Mini Compiler for a Toy C Language Targeting a Hypothetical ISA

You may consider a subset of instructions from MIPS or any RISC ISA with limited set of registers.
While submitting your code, you need to mention the details of the considered ISA.
You need to implement the major phases of a compiler
1. Lexical analysis [10 mark]
2. Syntax analysis [10 mark]
3. Semantic analysis (type checking, undeclared variable detection, etc.) [20 mark]
4. Intermediate code generation (Three address code) [20 mark]
5. Basic optimization (common sub-expression evaluation, constant propagation, etc.) [20 mark]
6. Target code generation (Assembly) [30 mark]
Toy-C program should have the following features:
- int data type
- Expressions: +, -, *, /, <, >, ==, !=
- Statements: assignment, if-else, loop
Provide 5 test programs from your side that will be successfully compiled.
Sample Input:
int a, b, c;
a = b + c;
Three address code:
t1 = b + c
a = t1

Sample assembly code:
LOAD R1, b
LOAD R2, c
ADD R1, R2
STORE R1, a

You need to show all intermediate steps and output after every phase. You can implement in C or
use Lex-Yacc tool.
