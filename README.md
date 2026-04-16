# NanoC Compiler – Mini Compiler for Toy C Language

## Overview

This project implements a **mini compiler for a Toy C language (NanoC)** that translates source code into **assembly for a hypothetical RISC-style ISA**.

The compiler is built using:

* **Lex (Flex)** → Lexical Analysis
* **Yacc (Bison)** → Syntax Analysis
* **C Language** → Semantic Analysis, TAC, Optimization, Code Generation

---

## Features Supported

### Data Types

* `int`

### Expressions

* Arithmetic: `+`, `-`, `*`, `/`
* Relational: `<`, `>`, `==`, `!=`

### Statements

* Assignment
* `if-else`
* `while` loop

---

## Compiler Phases Implemented

### 1. Lexical Analysis

* Tokenizes input using Flex
* Recognizes keywords, identifiers, numbers, operators

---

### 2. Syntax Analysis

* Grammar defined using Bison
* Builds structured representation of program
* Detects syntax errors

---

### 3. Semantic Analysis

* Symbol table management
* Detects:

  * Undeclared variables
  * Duplicate declarations
* Performs type checking

---

### 4. Intermediate Code Generation (TAC)

* Generates **Three Address Code**
* Uses temporaries (`t0, t1, ...`)
* Example:

```
t0 = a + b
c = t0
```

---

### 5. Optimization

Implemented optimizations:

* Constant Folding
* Constant Propagation
* Common Subexpression Elimination (CSE)
* Algebraic Simplifications

---

### 6. Target Code Generation

* Converts TAC → Assembly
* Uses a **custom RISC-style ISA**

---

## Target ISA (Hypothetical)

Inspired by MIPS-like architecture.

### Registers

* `R1`, `R2` (general purpose)

### Instructions

* `LOAD R, var`
* `STORE R, var`
* `MOV R, constant`
* `ADD R1, R2`
* `SUB R1, R2`
* `MUL R1, R2`
* `DIV R1, R2`
* `CMP R1, R2`
* `JLT`, `JGT`, `JEQ`, `JNE`
* `JMP label`

---

## Compilation Pipeline

```
Source Code (.c)
        ↓
Lexical Analysis (Flex)
        ↓
Syntax Analysis (Bison)
        ↓
Semantic Analysis
        ↓
Three Address Code (TAC)
        ↓
Optimization
        ↓
Assembly Code (Target ISA)
```

---

## How to Run

### Step 1: Generate lexer and parser

```
flex lexer.l
bison -d parser.y
```

### Step 2: Compile

```
gcc lex.yy.c parser.tab.c tac.c -o compiler
gcc codegen.c -o codegen
```

### Step 3: Run

```
./compiler test1.c | ./codegen > output.txt
```

---

## Project Structure

```
NanoC_Compiler/
│
├── lexer.l           # Lexical Analyzer
├── parser.y          # Syntax + Semantic Analysis + TAC
├── tac.c             # TAC utilities
├── tac.h             # Node structure
├── codegen.c         # TAC → Assembly
├── symbol_table.h    # Symbol table implementation
│
├── test1.c ... test5.c   # Sample programs
├── output.txt            # Generated output
└── README.md
```

---

## Sample Input

```c
int a, b, c;
a = 5;
b = 10;
c = a + b;
```

---

## 🔹 Three Address Code (TAC)

```
a = 5
b = 10
t0 = a + b
c = t0
```

---

## 🔹 Generated Assembly

```
MOV R1, 5
STORE R1, a

MOV R1, 10
STORE R1, b

LOAD R1, a
LOAD R2, b
ADD R1, R2
STORE R1, t0

LOAD R1, t0
STORE R1, c
```

---

## Key Highlights

* Complete compiler pipeline implemented
* Modular design (lexer, parser, codegen separated)
* Includes optimization techniques
* Clean RISC-style assembly output

---

## Learning Outcomes

* Understanding of compiler phases
* Hands-on experience with Flex & Bison
* Intermediate representation (TAC)
* Code optimization techniques
* Assembly code generation

---
