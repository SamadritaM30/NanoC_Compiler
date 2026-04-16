/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 1 "toy.y"

    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <ctype.h>

    extern int yylineno;
    extern FILE *yyin;
    void yyrestart(FILE *input_file);

    int yylex(void);
    void yyerror(const char *s);

    int dump_lex = 0;

    #define MAX_SYMBOLS 200
    #define MAX_QUADS   500
    #define MAX_STACK   100

    /* ─────────────────────────────────────────
    Data structures
    ───────────────────────────────────────── */

    typedef struct ASTNode {
        char label[64];     // for tree (like "+")
        char place[64];     // for TAC (like "t1")
        struct ASTNode *children[10];
        int count;
    } ASTNode;

    ASTNode *root = NULL;

    ASTNode* makeNode(const char *label) {
        ASTNode *n = malloc(sizeof(ASTNode));
        strcpy(n->label, label);
        strcpy(n->place, label);   // default
        n->count = 0;
        return n;
    }

    void addChild(ASTNode *p, ASTNode *c) {
        if (p && c) p->children[p->count++] = c;
    }

    void printTree(ASTNode *node, int depth, int isLast, int prefix[]) {
        if (!node) return;

        // print branches
        for (int i = 0; i < depth; i++) {
            if (prefix[i])
                printf("│   ");
            else
                printf("    ");
        }

        if (depth > 0) {
            if (isLast)
                printf("└── ");
            else
                printf("├── ");
        }

        printf("%s\n", node->label);

        // update prefix for children
        if (depth >= 0)
            prefix[depth] = !isLast;

        for (int i = 0; i < node->count; i++) {
            printTree(node->children[i], depth + 1,
                    (i == node->count - 1), prefix);
        }
    }

    typedef struct {
        char op[16];
        char arg1[64];
        char arg2[64];
        char res[64];
    } Quad;

    typedef struct {
        char *t;   /* true  label */
        char *f;   /* false label */
        char *e;   /* end   label (else branch) */
    } IfCtx;

    typedef struct {
        char *start;
        char *body;
        char *end;
    } WhileCtx;

    typedef struct {
        char name[32];
        int  valid;
        int  value;
    } ConstEntry;

    /* ─────────────────────────────────────────
    Symbol / type table
    ───────────────────────────────────────── */
    typedef struct {
        char name[32];
        char type[16];   /* "int"*/
    } Symbol;

    Symbol symTable[MAX_SYMBOLS];
    int sc = 0;

    /* ─────────────────────────────────────────
    Quad tables
    ───────────────────────────────────────── */
    Quad quads[MAX_QUADS];
    Quad optQuads[MAX_QUADS];
    int qCount  = 0;
    int optCount = 0;

    /* ─────────────────────────────────────────
    Constant propagation table
    ───────────────────────────────────────── */
    ConstEntry consts[MAX_SYMBOLS];
    int constCount = 0;

    /* ─────────────────────────────────────────
    If / while stacks
    ───────────────────────────────────────── */
    IfCtx    ifStack[MAX_STACK];
    WhileCtx whileStack[MAX_STACK];
    int ifTop    = -1;
    int whileTop = -1;

    /* ─────────────────────────────────────────
    Counters
    ───────────────────────────────────────── */
    int tempCount  = 1;
    int labelCount = 1;

    /* ================================================================
    Utility helpers
    ================================================================ */

    static char *dupstr(const char *s) {
        char *p = malloc(strlen(s) + 1);
        if (!p) { fprintf(stderr, "Memory allocation failed\n"); exit(1); }
        strcpy(p, s);
        return p;
    }

    static int is_number(const char *s) {
        if (!s || !*s) return 0;
        int i = 0;
        if (s[0] == '-' && s[1]) i = 1;
        for (; s[i]; i++)
            if (!isdigit((unsigned char)s[i])) return 0;
        return 1;
    }

    static int is_temp_name(const char *s) {
        return s && s[0] == 't' && is_number(s + 1);
    }

    /* ================================================================
    Symbol table
    ================================================================ */

    static int lookup_symbol(const char *name) {
        for (int i = 0; i < sc; i++)
            if (strcmp(symTable[i].name, name) == 0) return i;
        return -1;
    }

    static void declare_var(const char *name, const char *type) {
        if (lookup_symbol(name) >= 0) {
            printf("Semantic Warning: variable '%s' already declared\n", name);
            return;
        }
        if (sc >= MAX_SYMBOLS) { printf("Symbol table full\n"); exit(1); }
        strcpy(symTable[sc].name, name);
        strcpy(symTable[sc].type, type);
        sc++;
    }

    /* Semantic error counter */
    static int semantic_error_count = 0;

    /* Returns the type string for the variable.
    On undeclared variable: records the error, returns "int" as safe dummy
    so parsing and code-gen can continue for all remaining statements. */
    static const char *use_var(const char *name) {
        int idx = lookup_symbol(name);
        if (idx < 0) {
            printf("  Line %d: Semantic error - undeclared variable '%s'\n",
                yylineno, name);
            semantic_error_count++;
            return "int";   /* safe dummy — let compilation continue */
        }
        return symTable[idx].type;
    }

    static char *new_temp(void) {
        char buf[32];
        sprintf(buf, "t%d", tempCount++);
        return dupstr(buf);
    }

    static char *new_label(void) {
        char buf[32];
        sprintf(buf, "L%d", labelCount++);
        return dupstr(buf);
    }

    /* ================================================================
    Quad emission
    ================================================================ */

    static void emit(const char *op, const char *a1, const char *a2,
                    const char *res) {
        if (qCount >= MAX_QUADS) { printf("Quad array full\n"); exit(1); }
        strncpy(quads[qCount].op,   op  ? op  : "", 15);
        strncpy(quads[qCount].arg1, a1  ? a1  : "", 63);
        strncpy(quads[qCount].arg2, a2  ? a2  : "", 63);
        strncpy(quads[qCount].res,  res ? res : "", 63);
        qCount++;
    }

    /* ================================================================
    Printing
    ================================================================ */

    static void print_symbol_table(void) {
        printf("\n[Phase 3] Symbol Table\n");
        printf("-----------------------\n");
        for (int i = 0; i < sc; i++)
            printf("  %d. %s  [type: %s]\n", i + 1, symTable[i].name,
                symTable[i].type);
    }

    static void print_quads(Quad *arr, int n, const char *title) {
        (void)title;
        for (int i = 0; i < n; i++) {
            Quad *q = &arr[i];
            if (strcmp(q->op, "+") == 0 || strcmp(q->op, "-") == 0 ||
                strcmp(q->op, "*") == 0 || strcmp(q->op, "/") == 0) {
                printf("  %s = %s %s %s\n", q->res, q->arg1, q->op, q->arg2);
            } else if (strcmp(q->op, "=") == 0) {
                printf("  %s = %s\n", q->res, q->arg1);
            } else if (strcmp(q->op, "ifgoto") == 0) {
                printf("  if %s goto %s\n", q->arg1, q->res);
            } else if (strcmp(q->op, "goto") == 0) {
                printf("  goto %s\n", q->res);
            } else if (strcmp(q->op, "label") == 0) {
                printf("%s:\n", q->res);
            } else {
                printf("  %s %s %s %s\n", q->op, q->arg1, q->arg2, q->res);
            }
        }
    }

    /* ================================================================
    Constant propagation / folding helpers
    ================================================================ */

    static int compute_arith(int x, int y, const char *op) {
        if (strcmp(op, "+") == 0) return x + y;
        if (strcmp(op, "-") == 0) return x - y;
        if (strcmp(op, "*") == 0) return x * y;
        if (strcmp(op, "/") == 0) return (y == 0) ? 0 : x / y;
        return 0;
    }

    static void clear_consts(void) {
        for (int i = 0; i < constCount; i++) consts[i].valid = 0;
    }

    static int get_const_value(const char *name, int *value) {
        if (is_number(name)) { *value = atoi(name); return 1; }
        for (int i = 0; i < constCount; i++)
            if (consts[i].valid && strcmp(consts[i].name, name) == 0) {
                *value = consts[i].value; return 1;
            }
        return 0;
    }

    static void set_const_value(const char *name, int value) {
        for (int i = 0; i < constCount; i++)
            if (strcmp(consts[i].name, name) == 0) {
                consts[i].valid = 1; consts[i].value = value; return;
            }
        strncpy(consts[constCount].name, name, 31);
        consts[constCount].valid = 1;
        consts[constCount].value = value;
        constCount++;
    }

    static void kill_const(const char *name) {
        for (int i = 0; i < constCount; i++)
            if (strcmp(consts[i].name, name) == 0) { consts[i].valid = 0; return; }
    }

    /* ================================================================
    Optimiser  (constant folding + constant propagation)
    ================================================================ */

    static void optimize_quads(void) {
        optCount = 0;
        clear_consts();

        for (int i = 0; i < qCount; i++) {
            Quad q = quads[i];

            /* ── arithmetic ───────────────────────────────────────── */
            if (strcmp(q.op, "+") == 0 || strcmp(q.op, "-") == 0 ||
                strcmp(q.op, "*") == 0 || strcmp(q.op, "/") == 0) {

                /* Propagate known constants into operands */
                int a, b;
                char left[64], right[64];

                if (get_const_value(q.arg1, &a)) sprintf(left,  "%d", a);
                else                              strcpy(left,  q.arg1);

                if (get_const_value(q.arg2, &b)) sprintf(right, "%d", b);
                else                              strcpy(right, q.arg2);

                /* Check for pattern:  t = op … ; x = t  (temp copy-prop) */
                if (i + 1 < qCount &&
                    strcmp(quads[i+1].op,   "=")    == 0 &&
                    strcmp(quads[i+1].arg1, q.res)  == 0 &&
                    is_temp_name(q.res)) {

                    char target[64];
                    strcpy(target, quads[i+1].res);

                    if (is_number(left) && is_number(right)) {
                        int res = compute_arith(atoi(left), atoi(right), q.op);
                        char buf[32]; sprintf(buf, "%d", res);
                        strcpy(optQuads[optCount].op,   "=");
                        strcpy(optQuads[optCount].arg1, buf);
                        strcpy(optQuads[optCount].arg2, "");
                        strcpy(optQuads[optCount].res,  target);
                        optCount++;
                        set_const_value(target, res);
                    } else {
                        strcpy(optQuads[optCount].op,   q.op);
                        strcpy(optQuads[optCount].arg1, left);
                        strcpy(optQuads[optCount].arg2, right);
                        strcpy(optQuads[optCount].res,  target);
                        optCount++;
                        kill_const(target);
                    }
                    i++;   /* skip the "= t" quad */
                    continue;
                }

                /* Standalone arithmetic quad */
                if (is_number(left) && is_number(right)) {
                    int res = compute_arith(atoi(left), atoi(right), q.op);
                    char buf[32]; sprintf(buf, "%d", res);
                    strcpy(optQuads[optCount].op,   "=");
                    strcpy(optQuads[optCount].arg1, buf);
                    strcpy(optQuads[optCount].arg2, "");
                    strcpy(optQuads[optCount].res,  q.res);
                    optCount++;
                    set_const_value(q.res, res);
                } else {
                    strcpy(optQuads[optCount].op,   q.op);
                    strcpy(optQuads[optCount].arg1, left);
                    strcpy(optQuads[optCount].arg2, right);
                    strcpy(optQuads[optCount].res,  q.res);
                    optCount++;
                    kill_const(q.res);
                }

            /* ── assignment ───────────────────────────────────────── */
            } else if (strcmp(q.op, "=") == 0) {
                int val;
                char src[64];
                if (get_const_value(q.arg1, &val)) {
                    sprintf(src, "%d", val);
                    strcpy(optQuads[optCount].op,   "=");
                    strcpy(optQuads[optCount].arg1, src);
                    strcpy(optQuads[optCount].arg2, "");
                    strcpy(optQuads[optCount].res,  q.res);
                    optCount++;
                    set_const_value(q.res, val);
                } else {
                    strcpy(optQuads[optCount].op,   "=");
                    strcpy(optQuads[optCount].arg1, q.arg1);
                    strcpy(optQuads[optCount].arg2, "");
                    strcpy(optQuads[optCount].res,  q.res);
                    optCount++;
                    kill_const(q.res);
                }

            /* ── control flow ─────────────────────────────────────── */
            } else if (strcmp(q.op, "label")   == 0 ||
                    strcmp(q.op, "goto")    == 0 ||
                    strcmp(q.op, "ifgoto")  == 0) {
                clear_consts();          /* conservative: kill all on branch */
                strcpy(optQuads[optCount].op,   q.op);
                strcpy(optQuads[optCount].arg1, q.arg1);
                strcpy(optQuads[optCount].arg2, q.arg2);
                strcpy(optQuads[optCount].res,  q.res);
                optCount++;

            /* ── anything else ────────────────────────────────────── */
            } else {
                strcpy(optQuads[optCount].op,   q.op);
                strcpy(optQuads[optCount].arg1, q.arg1);
                strcpy(optQuads[optCount].arg2, q.arg2);
                strcpy(optQuads[optCount].res,  q.res);
                optCount++;
            }
        }
    }

    /* ================================================================
    If / while context management
    ================================================================ */

    static void push_if(void) {
        ifTop++;
        ifStack[ifTop].t = new_label();
        ifStack[ifTop].f = new_label();
        ifStack[ifTop].e = NULL;
    }
    static void pop_if(void)  { ifTop--; }

    static char *if_true(void)  { return ifStack[ifTop].t; }
    static char *if_false(void) { return ifStack[ifTop].f; }
    static char *if_end(void) {
        if (ifStack[ifTop].e == NULL)
            ifStack[ifTop].e = new_label();
        return ifStack[ifTop].e;
    }

    static void push_while(void) {
        whileTop++;
        whileStack[whileTop].start = new_label();
        whileStack[whileTop].body  = new_label();
        whileStack[whileTop].end   = new_label();
    }
    static void pop_while(void) { whileTop--; }

    static char *while_start(void) { return whileStack[whileTop].start; }
    static char *while_body(void)  { return whileStack[whileTop].body;  }
    static char *while_end(void)   { return whileStack[whileTop].end;   }

    /* ================================================================
    Assembly generation
    (Hypothetical RISC ISA: LOADI, LOAD, STORE, ADD, SUB, MUL, DIV,
        CMP, JLT, JGT, JLE, JGE, JE, JNE, JMP)
    ================================================================ */

    static const char *branch_for_relop(const char *op) {
        if (strcmp(op, "<")  == 0) return "JLT";
        if (strcmp(op, ">")  == 0) return "JGT";
        if (strcmp(op, "<=") == 0) return "JLE";
        if (strcmp(op, ">=") == 0) return "JGE";
        if (strcmp(op, "==") == 0) return "JE";
        if (strcmp(op, "!=") == 0) return "JNE";
        return "JMP";
    }

    static void load_operand(const char *opnd, const char *reg) {
        if (is_number(opnd)) printf("  LOADI %s, #%s\n", reg, opnd);
        else                 printf("  LOAD  %s, %s\n",  reg, opnd);
    }

    static void generate_assembly(Quad *arr, int n) {
        printf("\nAssembly Code\n");
        printf("-------------\n");

        for (int i = 0; i < n; i++) {
            Quad *q = &arr[i];

            if (strcmp(q->op, "label") == 0) {
                printf("%s:\n", q->res);

            } else if (strcmp(q->op, "goto") == 0) {
                printf("  JMP %s\n", q->res);

            } else if (strcmp(q->op, "ifgoto") == 0) {
                char left[64], rel[8], right[64];
                sscanf(q->arg1, "%63s %7s %63s", left, rel, right);
                load_operand(left,  "R1");
                load_operand(right, "R2");
                printf("  CMP R1, R2\n");
                printf("  %s %s\n", branch_for_relop(rel), q->res);

            } else if (strcmp(q->op, "+") == 0 || strcmp(q->op, "-") == 0 ||
                    strcmp(q->op, "*") == 0 || strcmp(q->op, "/") == 0) {
                load_operand(q->arg1, "R1");
                load_operand(q->arg2, "R2");
                if (strcmp(q->op, "+") == 0) printf("  ADD R1, R2\n");
                if (strcmp(q->op, "-") == 0) printf("  SUB R1, R2\n");
                if (strcmp(q->op, "*") == 0) printf("  MUL R1, R2\n");
                if (strcmp(q->op, "/") == 0) printf("  DIV R1, R2\n");
                printf("  STORE R1, %s\n", q->res);

            } else if (strcmp(q->op, "=") == 0) {
                load_operand(q->arg1, "R1");
                printf("  STORE R1, %s\n", q->res);
            }
        }
    }

    

#line 582 "toy.tab.c"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

#include "toy.tab.h"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_ID = 3,                         /* ID  */
  YYSYMBOL_NUM = 4,                        /* NUM  */
  YYSYMBOL_RELOP = 5,                      /* RELOP  */
  YYSYMBOL_INT = 6,                        /* INT  */
  YYSYMBOL_IF = 7,                         /* IF  */
  YYSYMBOL_ELSE = 8,                       /* ELSE  */
  YYSYMBOL_WHILE = 9,                      /* WHILE  */
  YYSYMBOL_10_ = 10,                       /* '='  */
  YYSYMBOL_11_ = 11,                       /* '+'  */
  YYSYMBOL_12_ = 12,                       /* '-'  */
  YYSYMBOL_13_ = 13,                       /* '*'  */
  YYSYMBOL_14_ = 14,                       /* '/'  */
  YYSYMBOL_LOWER_THAN_ELSE = 15,           /* LOWER_THAN_ELSE  */
  YYSYMBOL_16_ = 16,                       /* ';'  */
  YYSYMBOL_17_ = 17,                       /* '{'  */
  YYSYMBOL_18_ = 18,                       /* '}'  */
  YYSYMBOL_19_ = 19,                       /* ','  */
  YYSYMBOL_20_ = 20,                       /* '('  */
  YYSYMBOL_21_ = 21,                       /* ')'  */
  YYSYMBOL_YYACCEPT = 22,                  /* $accept  */
  YYSYMBOL_program = 23,                   /* program  */
  YYSYMBOL_stmts = 24,                     /* stmts  */
  YYSYMBOL_stmt = 25,                      /* stmt  */
  YYSYMBOL_non_if_stmt = 26,               /* non_if_stmt  */
  YYSYMBOL_matched_stmt = 27,              /* matched_stmt  */
  YYSYMBOL_unmatched_stmt = 28,            /* unmatched_stmt  */
  YYSYMBOL_else_prep = 29,                 /* else_prep  */
  YYSYMBOL_block = 30,                     /* block  */
  YYSYMBOL_decl = 31,                      /* decl  */
  YYSYMBOL_idlist = 32,                    /* idlist  */
  YYSYMBOL_assign = 33,                    /* assign  */
  YYSYMBOL_expr = 34,                      /* expr  */
  YYSYMBOL_cond = 35,                      /* cond  */
  YYSYMBOL_if_header = 36,                 /* if_header  */
  YYSYMBOL_while_header = 37,              /* while_header  */
  YYSYMBOL_38_1 = 38                       /* $@1  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_int8 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if !defined yyoverflow

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* !defined yyoverflow */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  26
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   74

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  22
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  17
/* YYNRULES -- Number of rules.  */
#define YYNRULES  34
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  63

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   265


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      20,    21,    13,    11,    19,    12,     2,    14,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,    16,
       2,    10,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    17,     2,    18,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    15
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   547,   547,   553,   558,   567,   571,   578,   583,   588,
     593,   598,   606,   610,   622,   636,   642,   654,   669,   678,
     689,   697,   704,   716,   735,   746,   757,   768,   779,   783,
     790,   801,   814,   827,   826
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "ID", "NUM", "RELOP",
  "INT", "IF", "ELSE", "WHILE", "'='", "'+'", "'-'", "'*'", "'/'",
  "LOWER_THAN_ELSE", "';'", "'{'", "'}'", "','", "'('", "')'", "$accept",
  "program", "stmts", "stmt", "non_if_stmt", "matched_stmt",
  "unmatched_stmt", "else_prep", "block", "decl", "idlist", "assign",
  "expr", "cond", "if_header", "while_header", "$@1", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-18)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-19)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int8 yypact[] =
{
      55,    -5,     7,    19,     5,   -18,   -18,    55,    29,     3,
     -18,   -18,   -18,   -18,   -18,    23,    26,    55,    55,   -18,
       4,   -18,    32,     4,    25,    37,   -18,   -18,   -18,   -18,
     -18,    44,   -18,   -18,   -18,   -18,     4,    54,    56,    36,
      39,     4,   -18,    49,     2,     4,     4,     4,     4,   -18,
       4,   -18,    42,    55,   -18,    14,    14,   -18,   -18,    54,
     -18,   -18,   -18
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       0,     0,     0,     0,     0,    33,    10,     0,     0,     0,
       4,    12,     5,     6,     9,     0,     0,     0,     0,    11,
       0,    22,    20,     0,     0,     0,     1,     3,     7,     8,
      15,     5,    13,    16,    29,    30,     0,    23,     0,     0,
       0,     0,    19,     0,     0,     0,     0,     0,     0,    21,
       0,    32,     0,     0,    28,    24,    25,    26,    27,    31,
      34,    14,    17
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -18,   -18,    62,     9,   -18,   -17,   -16,   -18,   -18,   -18,
     -18,   -18,   -15,    33,   -18,   -18,   -18
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
       0,     8,     9,    10,    11,    12,    13,    43,    14,    15,
      22,    16,    39,    40,    17,    18,    24
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int8 yytable[] =
{
      31,    32,    33,    -2,     1,    37,     2,    34,    35,     3,
       4,    19,     5,    45,    46,    47,    48,    20,    27,     6,
       7,    44,    21,    54,    36,    23,    30,    47,    48,    26,
      55,    56,    57,    58,    27,    59,    61,    62,     1,    28,
       2,    50,    29,     3,     4,    41,     5,    45,    46,    47,
      48,    38,   -18,     6,     7,    42,     1,    53,     2,    49,
      51,     3,     4,    60,     5,    45,    46,    47,    48,    25,
       0,     6,     7,     0,    52
};

static const yytype_int8 yycheck[] =
{
      17,    18,    18,     0,     1,    20,     3,     3,     4,     6,
       7,    16,     9,    11,    12,    13,    14,    10,     9,    16,
      17,    36,     3,    21,    20,    20,    17,    13,    14,     0,
      45,    46,    47,    48,    25,    50,    53,    53,     1,    16,
       3,     5,    16,     6,     7,    20,     9,    11,    12,    13,
      14,    19,     8,    16,    17,    18,     1,     8,     3,     3,
      21,     6,     7,    21,     9,    11,    12,    13,    14,     7,
      -1,    16,    17,    -1,    41
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     1,     3,     6,     7,     9,    16,    17,    23,    24,
      25,    26,    27,    28,    30,    31,    33,    36,    37,    16,
      10,     3,    32,    20,    38,    24,     0,    25,    16,    16,
      25,    27,    27,    28,     3,     4,    20,    34,    19,    34,
      35,    20,    18,    29,    34,    11,    12,    13,    14,     3,
       5,    21,    35,     8,    21,    34,    34,    34,    34,    34,
      21,    27,    28
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    22,    23,    24,    24,    25,    25,    26,    26,    26,
      26,    26,    27,    27,    27,    28,    28,    28,    29,    30,
      31,    32,    32,    33,    34,    34,    34,    34,    34,    34,
      34,    35,    36,    38,    37
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     2,     1,     1,     1,     2,     2,     1,
       1,     2,     1,     2,     5,     2,     2,     5,     0,     3,
       2,     3,     1,     3,     3,     3,     3,     3,     3,     1,
       1,     3,     4,     0,     5
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)




# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)]);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep)
{
  YY_USE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;




/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 2: /* program: stmts  */
#line 548 "toy.y"
        {
            root = (yyvsp[0].node);
        }
#line 1643 "toy.tab.c"
    break;

  case 3: /* stmts: stmts stmt  */
#line 554 "toy.y"
        {
            addChild((yyvsp[-1].node), (yyvsp[0].node));
            (yyval.node) = (yyvsp[-1].node);
        }
#line 1652 "toy.tab.c"
    break;

  case 4: /* stmts: stmt  */
#line 559 "toy.y"
        {
            ASTNode *n = makeNode("stmts");
            addChild(n, (yyvsp[0].node));
            (yyval.node) = n;
        }
#line 1662 "toy.tab.c"
    break;

  case 5: /* stmt: matched_stmt  */
#line 568 "toy.y"
        {
            (yyval.node) = (yyvsp[0].node);
        }
#line 1670 "toy.tab.c"
    break;

  case 6: /* stmt: unmatched_stmt  */
#line 572 "toy.y"
        {
            (yyval.node) = (yyvsp[0].node);
        }
#line 1678 "toy.tab.c"
    break;

  case 7: /* non_if_stmt: decl ';'  */
#line 579 "toy.y"
        {
            (yyval.node) = (yyvsp[-1].node);
        }
#line 1686 "toy.tab.c"
    break;

  case 8: /* non_if_stmt: assign ';'  */
#line 584 "toy.y"
        {
            (yyval.node) = (yyvsp[-1].node);
        }
#line 1694 "toy.tab.c"
    break;

  case 9: /* non_if_stmt: block  */
#line 589 "toy.y"
        {
            (yyval.node) = (yyvsp[0].node);
        }
#line 1702 "toy.tab.c"
    break;

  case 10: /* non_if_stmt: ';'  */
#line 594 "toy.y"
        {
            (yyval.node) = makeNode("empty");
        }
#line 1710 "toy.tab.c"
    break;

  case 11: /* non_if_stmt: error ';'  */
#line 599 "toy.y"
        {
            yyerrok;
            (yyval.node) = makeNode("error");
        }
#line 1719 "toy.tab.c"
    break;

  case 12: /* matched_stmt: non_if_stmt  */
#line 607 "toy.y"
        {
            (yyval.node) = (yyvsp[0].node);
        }
#line 1727 "toy.tab.c"
    break;

  case 13: /* matched_stmt: while_header matched_stmt  */
#line 611 "toy.y"
        {
            ASTNode *n = makeNode("while");
            addChild(n, makeNode("condition"));
            addChild(n, (yyvsp[0].node));

            emit("goto",  "", "", while_start());
            emit("label", "", "", while_end());
            pop_while();

            (yyval.node) = n;
        }
#line 1743 "toy.tab.c"
    break;

  case 14: /* matched_stmt: if_header matched_stmt else_prep ELSE matched_stmt  */
#line 623 "toy.y"
        {
            emit("label", "", "", if_end());
            pop_if();

            ASTNode *n = makeNode("if");
            addChild(n, makeNode("condition"));
            addChild(n, (yyvsp[-3].node));
            addChild(n, (yyvsp[0].node));
            (yyval.node) = n;
        }
#line 1758 "toy.tab.c"
    break;

  case 15: /* unmatched_stmt: if_header stmt  */
#line 637 "toy.y"
        {
            emit("label", "", "", if_false());
            pop_if();
            (yyval.node) = (yyvsp[0].node);
        }
#line 1768 "toy.tab.c"
    break;

  case 16: /* unmatched_stmt: while_header unmatched_stmt  */
#line 643 "toy.y"
        {
            ASTNode *n = makeNode("while");
            addChild(n, makeNode("condition"));
            addChild(n, (yyvsp[0].node));

            emit("goto",  "", "", while_start());
            emit("label", "", "", while_end());
            pop_while();

            (yyval.node) = n;
        }
#line 1784 "toy.tab.c"
    break;

  case 17: /* unmatched_stmt: if_header matched_stmt else_prep ELSE unmatched_stmt  */
#line 655 "toy.y"
        {
            emit("label", "", "", if_end());
            pop_if();

            ASTNode *n = makeNode("if");
            addChild(n, makeNode("condition"));
            addChild(n, (yyvsp[-3].node));
            addChild(n, (yyvsp[0].node));
            (yyval.node) = n;
        }
#line 1799 "toy.tab.c"
    break;

  case 18: /* else_prep: %empty  */
#line 669 "toy.y"
        {
            /* After THEN, jump over ELSE and mark ELSE entry label. */
            emit("goto", "", "", if_end());
            emit("label", "", "", if_false());
            (yyval.node) = NULL;
        }
#line 1810 "toy.tab.c"
    break;

  case 19: /* block: '{' stmts '}'  */
#line 679 "toy.y"
        {
        ASTNode *n = makeNode("block");
        addChild(n, (yyvsp[-1].node));
        (yyval.node) = n;
        }
#line 1820 "toy.tab.c"
    break;

  case 20: /* decl: INT idlist  */
#line 690 "toy.y"
        {
            ASTNode *n = makeNode("decl(int)");
            addChild(n, (yyvsp[0].node));
            (yyval.node) = n;
        }
#line 1830 "toy.tab.c"
    break;

  case 21: /* idlist: idlist ',' ID  */
#line 698 "toy.y"
        {
            declare_var((yyvsp[0].str), "int");
            addChild((yyvsp[-2].node), makeNode((yyvsp[0].str)));
            (yyval.node) = (yyvsp[-2].node);
            free((yyvsp[0].str));
        }
#line 1841 "toy.tab.c"
    break;

  case 22: /* idlist: ID  */
#line 705 "toy.y"
        {
            declare_var((yyvsp[0].str), "int");
            ASTNode *n = makeNode("idlist");
            addChild(n, makeNode((yyvsp[0].str)));
            (yyval.node) = n;
            free((yyvsp[0].str));
        }
#line 1853 "toy.tab.c"
    break;

  case 23: /* assign: ID '=' expr  */
#line 717 "toy.y"
        {
            int errs_before = semantic_error_count;
            use_var((yyvsp[-2].str));
            if (semantic_error_count == errs_before)
                emit("=", (yyvsp[0].node)->place, "", (yyvsp[-2].str));

            ASTNode *n = makeNode("=");
            addChild(n, makeNode((yyvsp[-2].str)));
            addChild(n, (yyvsp[0].node));
            (yyval.node) = n;

            free((yyvsp[-2].str));
        }
#line 1871 "toy.tab.c"
    break;

  case 24: /* expr: expr '+' expr  */
#line 736 "toy.y"
        {
            char *t = new_temp();
            emit("+", (yyvsp[-2].node)->place, (yyvsp[0].node)->place, t);

            ASTNode *n = makeNode("+");
            addChild(n, (yyvsp[-2].node));
            addChild(n, (yyvsp[0].node));
            strcpy(n->place, t); 
            (yyval.node) = n;
        }
#line 1886 "toy.tab.c"
    break;

  case 25: /* expr: expr '-' expr  */
#line 747 "toy.y"
        {
            char *t = new_temp();
            emit("-", (yyvsp[-2].node)->place, (yyvsp[0].node)->place, t);

            ASTNode *n = makeNode("-");
            addChild(n, (yyvsp[-2].node));
            addChild(n, (yyvsp[0].node));
            strcpy(n->place, t); 
            (yyval.node) = n;
        }
#line 1901 "toy.tab.c"
    break;

  case 26: /* expr: expr '*' expr  */
#line 758 "toy.y"
        {
            char *t = new_temp();
            emit("*", (yyvsp[-2].node)->place, (yyvsp[0].node)->place, t);

            ASTNode *n = makeNode("*");
            addChild(n, (yyvsp[-2].node));
            addChild(n, (yyvsp[0].node));
            strcpy(n->place, t); 
            (yyval.node) = n;
        }
#line 1916 "toy.tab.c"
    break;

  case 27: /* expr: expr '/' expr  */
#line 769 "toy.y"
        {
            char *t = new_temp();
            emit("/", (yyvsp[-2].node)->place, (yyvsp[0].node)->place, t);

            ASTNode *n = makeNode("/");
            addChild(n, (yyvsp[-2].node));
            addChild(n, (yyvsp[0].node));
            strcpy(n->place, t); 
            (yyval.node) = n;
        }
#line 1931 "toy.tab.c"
    break;

  case 28: /* expr: '(' expr ')'  */
#line 780 "toy.y"
        {
            (yyval.node) = (yyvsp[-1].node);
        }
#line 1939 "toy.tab.c"
    break;

  case 29: /* expr: ID  */
#line 784 "toy.y"
        {
            use_var((yyvsp[0].str));
            ASTNode *n = makeNode((yyvsp[0].str));
            strcpy(n->place, (yyvsp[0].str));
            (yyval.node) = n;
        }
#line 1950 "toy.tab.c"
    break;

  case 30: /* expr: NUM  */
#line 791 "toy.y"
        {
            ASTNode *n = makeNode((yyvsp[0].str));
            strcpy(n->place, (yyvsp[0].str));
            (yyval.node) = n;
        }
#line 1960 "toy.tab.c"
    break;

  case 31: /* cond: expr RELOP expr  */
#line 802 "toy.y"
        {
            char *s = malloc(strlen((yyvsp[-2].node)->label) + strlen((yyvsp[-1].str)) + strlen((yyvsp[0].node)->label) + 4);
            sprintf(s, "%s %s %s", (yyvsp[-2].node)->label, (yyvsp[-1].str), (yyvsp[0].node)->label);

            free((yyvsp[-1].str));  // RELOP string
            (yyval.str) = s;
        }
#line 1972 "toy.tab.c"
    break;

  case 32: /* if_header: IF '(' cond ')'  */
#line 815 "toy.y"
        {
            push_if();
            emit("ifgoto", (yyvsp[-1].str), "", if_true());
            emit("goto",   "",  "", if_false());
            emit("label",  "",  "", if_true());
            free((yyvsp[-1].str));
            (yyval.str) = NULL;
        }
#line 1985 "toy.tab.c"
    break;

  case 33: /* $@1: %empty  */
#line 827 "toy.y"
        {
            push_while();
            emit("label", "", "", while_start());
        }
#line 1994 "toy.tab.c"
    break;

  case 34: /* while_header: WHILE $@1 '(' cond ')'  */
#line 832 "toy.y"
        {
            emit("ifgoto", (yyvsp[-1].str), "", while_body());
            emit("goto",   "",  "", while_end());
            emit("label",  "",  "", while_body());
            free((yyvsp[-1].str));
            (yyval.str) = NULL;
        }
#line 2006 "toy.tab.c"
    break;


#line 2010 "toy.tab.c"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (YY_("syntax error"));
    }

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 841 "toy.y"


    /* ================================================================
    Error callback
    ================================================================ */

    static int syntax_error_count = 0;

    void yyerror(const char *s) {
        if (!dump_lex) {   /* only report errors during the parse pass */
            printf("  Line %d: Syntax error - %s\n", yylineno, s);
            syntax_error_count++;
        }
    }

    /* ================================================================
    Global reset (between lex pass and parse pass)
    ================================================================ */

    static void reset_state(void) {
        sc                  = 0;
        constCount          = 0;
        qCount              = 0;
        optCount            = 0;
        tempCount           = 1;
        labelCount          = 1;
        ifTop               = -1;
        whileTop            = -1;
        syntax_error_count  = 0;
        semantic_error_count = 0;
    }

    /* ================================================================
    main
    ================================================================ */

    int main(int argc, char *argv[]) {
        if (argc < 2) {
            printf("Usage: %s <input_file>\n", argv[0]);
            return 1;
        }

        FILE *fp = fopen(argv[1], "r");
        if (!fp) { perror("Cannot open input file"); return 1; }

        yyin = fp;

        /* ── Phase 1: Lexical Analysis ─────────────────────────────── */
        printf("[Phase 1] Lexical Analysis\n");
        printf("--------------------------\n");
        dump_lex = 1;
        while (yylex() != 0) { }

        /* Reset for the parse pass */
        rewind(fp);
        yyrestart(fp);
        yylineno = 1;
        reset_state();
        dump_lex = 0;

        /* ── Phase 2: Syntax Analysis ──────────────────────────────── */
        printf("\n[Phase 2] Syntax Analysis\n");
        printf("--------------------------\n");
        yyparse();   /* always run to completion — error recovery handles bad stmts */
        if (syntax_error_count > 0) {
            printf("Found %d syntax error(s). Erroneous statements skipped.\n", syntax_error_count);
        } else {
            printf("No syntax errors.\n");
            printf("\nSyntax Tree\n");
            printf("------------\n");
            int prefix[100] = {0};
            printTree(root, 0, 1, prefix);
        }

        /* ── Phase 3: Semantic Analysis ────────────────────────────── */
        printf("\n[Phase 3] Semantic Analysis\n");
        printf("----------------------------\n");
        if (semantic_error_count > 0)
            printf("Found %d semantic error(s). Affected statements skipped.\n", semantic_error_count);
        else
            printf("No semantic errors found\n");

        /* ── Phase 4: Intermediate Code ────────────────────────────── */
        print_symbol_table();
        printf("\n[Phase 4] Three Address Code\n");
        printf("-----------------------------\n");
        print_quads(quads, qCount, "TAC");

        /* ── Phase 5: Optimisation ─────────────────────────────────── */
        optimize_quads();
        printf("\n[Phase 5] Optimized Three Address Code\n");
        printf("---------------------------------------\n");
        print_quads(optQuads, optCount, "Optimized TAC");

        /* ── Phase 6: Target Code ───────────────────────────────────── */
        printf("\n[Phase 6] Target Code Generation\n");
        printf("---------------------------------\n");
        generate_assembly(optQuads, optCount);

        if (syntax_error_count == 0 && semantic_error_count == 0)
            printf("\nCompilation successful.\n");
        else
            printf("\nCompilation done. Syntax errors: %d, Semantic errors: %d\n", syntax_error_count, semantic_error_count);

        fclose(fp);
        return 0;
    }
