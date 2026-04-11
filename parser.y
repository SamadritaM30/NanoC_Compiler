%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

/* lexer exports these */
extern int yylineno;
extern FILE *yyin;
int yylex(void);
void yyerror(const char *s);

/* ------------------------- AST / IR NODE ------------------------- */
typedef struct Node {
    char *place;   /* final result / temporary / constant / identifier */
    char *code;    /* generated three-address code */
} Node;

/* ------------------------- GLOBALS ------------------------- */
static int temp_count = 0;
static int label_count = 0;
static int syntax_errors = 0;
static int semantic_errors = 0;

/* ------------------------- SYMBOL TABLE ------------------------- */
#define MAX_SYMBOLS 256

typedef struct {
    char *name;
    int is_const;
    char *const_val;
} Symbol;

static Symbol symtab[MAX_SYMBOLS];
static int symcount = 0;

/* ------------------------- CSE CACHE ------------------------- */
#define MAX_CSE 256
typedef struct {
    char *key;
    char *temp;
} CSEntry;

static CSEntry cse_cache[MAX_CSE];
static int cse_count = 0;

/* ------------------------- SMALL STRING HELPERS ------------------------- */
static char *xstrdup(const char *s) {
    if (!s) return NULL;
    size_t n = strlen(s);
    char *p = (char *)malloc(n + 1);
    if (!p) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    memcpy(p, s, n + 1);
    return p;
}

static char *xasprintf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    va_list ap2;
    va_copy(ap2, ap);
    int n = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);

    char *buf = (char *)malloc((size_t)n + 1);
    if (!buf) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    vsnprintf(buf, (size_t)n + 1, fmt, ap2);
    va_end(ap2);
    return buf;
}

static char *concat2(const char *a, const char *b) {
    size_t la = a ? strlen(a) : 0;
    size_t lb = b ? strlen(b) : 0;
    char *s = (char *)malloc(la + lb + 1);
    if (!s) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    if (a) memcpy(s, a, la);
    if (b) memcpy(s + la, b, lb);
    s[la + lb] = '\0';
    return s;
}

static Node *new_node(const char *place, const char *code) {
    Node *n = (Node *)malloc(sizeof(Node));
    if (!n) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    n->place = place ? xstrdup(place) : NULL;
    n->code  = code  ? xstrdup(code)  : xstrdup("");
    return n;
}

static Node *empty_node(void) {
    return new_node(NULL, "");
}

static Node *append_nodes(Node *a, Node *b) {
    if (!a && !b) return empty_node();
    if (!a) return new_node(NULL, b ? b->code : "");
    if (!b) return new_node(NULL, a ? a->code : "");
    char *code = concat2(a->code, b->code);
    return new_node(NULL, code);
}

static int is_int_literal(const char *s) {
    if (!s || !*s) return 0;
    const unsigned char *p = (const unsigned char *)s;
    if (*p == '-') p++;
    if (!*p) return 0;
    for (; *p; ++p) {
        if (!isdigit(*p)) return 0;
    }
    return 1;
}

static long long lit_to_ll(const char *s) {
    return strtoll(s, NULL, 10);
}

static char *new_temp(void) {
    return xasprintf("t%d", temp_count++);
}

static char *new_label(void) {
    return xasprintf("L%d", label_count++);
}

/* ------------------------- SYMBOL TABLE HELPERS ------------------------- */
static int sym_index(const char *name) {
    for (int i = 0; i < symcount; i++) {
        if (strcmp(symtab[i].name, name) == 0) return i;
    }
    return -1;
}

static void semantic_err(const char *msg) {
    semantic_errors++;
    fprintf(stderr, "Semantic error at line %d: %s\n", yylineno, msg);
}

static void add_symbol(const char *name) {
    if (symcount >= MAX_SYMBOLS) {
        semantic_err("symbol table overflow");
        return;
    }
    if (sym_index(name) != -1) {
        char *m = xasprintf("duplicate declaration of '%s'", name);
        semantic_err(m);
        free(m);
        return;
    }
    symtab[symcount].name = xstrdup(name);
    symtab[symcount].is_const = 0;
    symtab[symcount].const_val = NULL;
    symcount++;
}

static int declared(const char *name) {
    return sym_index(name) != -1;
}

static void set_const_value(const char *name, const char *val) {
    int idx = sym_index(name);
    if (idx < 0) return;
    if (symtab[idx].const_val) {
        free(symtab[idx].const_val);
        symtab[idx].const_val = NULL;
    }
    symtab[idx].is_const = 1;
    symtab[idx].const_val = xstrdup(val);
}

static void clear_const_value(const char *name) {
    int idx = sym_index(name);
    if (idx < 0) return;
    symtab[idx].is_const = 0;
    if (symtab[idx].const_val) {
        free(symtab[idx].const_val);
        symtab[idx].const_val = NULL;
    }
}

static const char *get_const_value(const char *name) {
    int idx = sym_index(name);
    if (idx < 0) return NULL;
    if (symtab[idx].is_const) return symtab[idx].const_val;
    return NULL;
}

/* ------------------------- CSE HELPERS ------------------------- */
static int is_commutative(const char *op) {
    return (
        strcmp(op, "+") == 0 ||
        strcmp(op, "*") == 0 ||
        strcmp(op, "==") == 0 ||
        strcmp(op, "!=") == 0
    );
}

static char *make_cse_key(const char *op, const char *a, const char *b) {
    if (is_commutative(op) && strcmp(a, b) > 0) {
        const char *tmp = a;
        a = b;
        b = tmp;
    }
    return xasprintf("%s|%s|%s", op, a, b);
}

static const char *cse_lookup(const char *key) {
    for (int i = 0; i < cse_count; i++) {
        if (strcmp(cse_cache[i].key, key) == 0) return cse_cache[i].temp;
    }
    return NULL;
}

static void cse_store(const char *key, const char *temp) {
    if (cse_count >= MAX_CSE) return;
    cse_cache[cse_count].key = xstrdup(key);
    cse_cache[cse_count].temp = xstrdup(temp);
    cse_count++;
}

/* ------------------------- CONSTRUCTION HELPERS ------------------------- */
static Node *make_id_node(const char *name) {
    if (!declared(name)) {
        char *m = xasprintf("use of undeclared variable '%s'", name);
        semantic_err(m);
        free(m);
    }

    const char *cv = get_const_value(name);
    if (cv) {
        return new_node(cv, "");   /* constant propagation */
    }
    return new_node(name, "");
}

static Node *make_num_node(int value) {
    char *v = xasprintf("%d", value);
    Node *n = new_node(v, "");
    free(v);
    return n;
}

static Node *make_unary_minus(Node *x) {
    char *code = xstrdup(x->code ? x->code : "");
    if (is_int_literal(x->place)) {
        long long v = -lit_to_ll(x->place);
        char *p = xasprintf("%lld", v);
        Node *n = new_node(p, code);
        free(p);
        free(code);
        return n;
    }

    char *t = new_temp();
    char *instr = xasprintf("%s = 0 - %s\n", t, x->place);
    char *all = concat2(code, instr);
    Node *n = new_node(t, all);
    free(t);
    free(instr);
    free(code);
    return n;
}

static Node *make_binary(const char *op, Node *l, Node *r) {
    char *code = concat2(l->code ? l->code : "", r->code ? r->code : "");

    /* small algebraic simplifications */
    if (strcmp(op, "+") == 0) {
        if (is_int_literal(l->place) && lit_to_ll(l->place) == 0) {
            return new_node(r->place, code);
        }
        if (is_int_literal(r->place) && lit_to_ll(r->place) == 0) {
            return new_node(l->place, code);
        }
    }
    if (strcmp(op, "-") == 0) {
        if (is_int_literal(r->place) && lit_to_ll(r->place) == 0) {
            return new_node(l->place, code);
        }
    }
    if (strcmp(op, "*") == 0) {
        if ((is_int_literal(l->place) && lit_to_ll(l->place) == 0) ||
            (is_int_literal(r->place) && lit_to_ll(r->place) == 0)) {
            Node *n = new_node("0", code);
            free(code);
            return n;
        }
        if (is_int_literal(l->place) && lit_to_ll(l->place) == 1) {
            return new_node(r->place, code);
        }
        if (is_int_literal(r->place) && lit_to_ll(r->place) == 1) {
            return new_node(l->place, code);
        }
    }
    if (strcmp(op, "/") == 0) {
        if (is_int_literal(r->place) && lit_to_ll(r->place) == 1) {
            return new_node(l->place, code);
        }
        if (is_int_literal(r->place) && lit_to_ll(r->place) == 0) {
            semantic_err("division by zero in constant expression");
        }
    }

    /* constant folding */
    if (is_int_literal(l->place) && is_int_literal(r->place)) {
        long long a = lit_to_ll(l->place);
        long long b = lit_to_ll(r->place);
        long long ans = 0;
        int folded = 1;

        if (strcmp(op, "+") == 0) ans = a + b;
        else if (strcmp(op, "-") == 0) ans = a - b;
        else if (strcmp(op, "*") == 0) ans = a * b;
        else if (strcmp(op, "/") == 0) {
            if (b == 0) {
                folded = 0;
            } else {
                ans = a / b;
            }
        } else if (strcmp(op, "<") == 0) ans = (a < b);
        else if (strcmp(op, ">") == 0) ans = (a > b);
        else if (strcmp(op, "==") == 0) ans = (a == b);
        else if (strcmp(op, "!=") == 0) ans = (a != b);
        else folded = 0;

        if (folded) {
            char *p = xasprintf("%lld", ans);
            Node *n = new_node(p, code);
            free(p);
            free(code);
            return n;
        }
    }

    /* common subexpression elimination */
    char *key = make_cse_key(op, l->place, r->place);
    const char *cached = cse_lookup(key);
    if (cached) {
        Node *n = new_node(cached, code);
        free(key);
        free(code);
        return n;
    }

    char *t = new_temp();
    char *instr = xasprintf("%s = %s %s %s\n", t, l->place, op, r->place);
    char *all = concat2(code, instr);

    cse_store(key, t);

    Node *n = new_node(t, all);
    free(t);
    free(instr);
    free(key);
    free(code);
    return n;
}

static Node *make_assignment(const char *lhs, Node *rhs) {
    if (!declared(lhs)) {
        char *m = xasprintf("assignment to undeclared variable '%s'", lhs);
        semantic_err(m);
        free(m);
    }

    char *assign = xasprintf("%s = %s\n", lhs, rhs->place);
    char *all = concat2(rhs->code ? rhs->code : "", assign);

    /* constant propagation bookkeeping */
    if (is_int_literal(rhs->place)) {
        set_const_value(lhs, rhs->place);
    } else {
        clear_const_value(lhs);
    }

    Node *n = new_node(lhs, all);
    free(assign);
    free(all);
    return n;
}

static Node *make_if(Node *cond, Node *then_part) {
    char *Ltrue = new_label();
    char *Lfalse = new_label();

    char *code1 = xasprintf("if %s goto %s\n", cond->place, Ltrue);
    char *code2 = xasprintf("goto %s\n%s:\n", Lfalse, Ltrue);

    char *all = concat2(cond->code ? cond->code : "", code1);
    char *tmp = concat2(all, code2);
    char *tmp2 = concat2(tmp, then_part->code ? then_part->code : "");
    char *code3 = xasprintf("%s:\n", Lfalse);
    char *final = concat2(tmp2, code3);

    Node *n = new_node(NULL, final);

    free(Ltrue);
    free(Lfalse);
    free(code1);
    free(code2);
    free(code3);
    free(all);
    free(tmp);
    free(tmp2);
    free(final);
    return n;
}

static Node *make_if_else(Node *cond, Node *then_part, Node *else_part) {
    char *Ltrue = new_label();
    char *Lfalse = new_label();
    char *Lend = new_label();

    char *c1 = xasprintf("if %s goto %s\n", cond->place, Ltrue);
    char *c2 = xasprintf("goto %s\n%s:\n", Lfalse, Ltrue);
    char *c3 = xasprintf("goto %s\n%s:\n", Lend, Lfalse);
    char *c4 = xasprintf("%s:\n", Lend);

    char *a = concat2(cond->code ? cond->code : "", c1);
    char *b = concat2(a, c2);
    char *d = concat2(b, then_part->code ? then_part->code : "");
    char *e = concat2(d, c3);
    char *f = concat2(e, else_part->code ? else_part->code : "");
    char *final = concat2(f, c4);

    Node *n = new_node(NULL, final);

    free(Ltrue);
    free(Lfalse);
    free(Lend);
    free(c1);
    free(c2);
    free(c3);
    free(c4);
    free(a);
    free(b);
    free(d);
    free(e);
    free(f);
    free(final);
    return n;
}

static Node *make_while(Node *cond, Node *body) {
    char *Lstart = new_label();
    char *Lbody = new_label();
    char *Lend = new_label();

    char *h1 = xasprintf("%s:\n", Lstart);
    char *h2 = xasprintf("if %s goto %s\n", cond->place, Lbody);
    char *h3 = xasprintf("goto %s\n%s:\n", Lend, Lbody);
    char *h4 = xasprintf("goto %s\n%s:\n", Lstart, Lend);

    char *a = concat2(h1, cond->code ? cond->code : "");
    char *b = concat2(a, h2);
    char *c = concat2(b, h3);
    char *d = concat2(c, body->code ? body->code : "");
    char *final = concat2(d, h4);

    Node *n = new_node(NULL, final);

    free(Lstart);
    free(Lbody);
    free(Lend);
    free(h1);
    free(h2);
    free(h3);
    free(h4);
    free(a);
    free(b);
    free(c);
    free(d);
    free(final);
    return n;
}

static void print_symbol_table(void) {
    printf("\n===== SYMBOL TABLE =====\n");
    printf("%-16s %-8s %-12s\n", "NAME", "TYPE", "CONST_VALUE");
    printf("------------------------------------------\n");
    for (int i = 0; i < symcount; i++) {
        printf("%-16s %-8s %-12s\n",
               symtab[i].name,
               "int",
               symtab[i].is_const ? symtab[i].const_val : "-");
    }
}
%}

%define parse.error verbose
%start program

%union {
    int num;
    char *id;
    Node *node;
}

%token INT IF ELSE WHILE
%token <num> NUMBER
%token <id> ID
%token PLUS MINUS MUL DIV
%token LT GT EQ NE
%token ASSIGN
%token SEMICOLON COMMA
%token LPAREN RPAREN LBRACE RBRACE

%type <node> program external_list external declaration stmt_list statement block assignment expr

/* precedence: later means higher */
%left LT GT EQ NE
%left PLUS MINUS
%left MUL DIV
%right UMINUS
%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%%

program
    : external_list
      {
          printf("\n===== TOY C COMPILER OUTPUT =====\n");
          printf("\n===== THREE ADDRESS CODE =====\n");
          printf("%s", $1->code ? $1->code : "");

          print_symbol_table();

          if (syntax_errors == 0 && semantic_errors == 0) {
              printf("\nCompilation finished successfully.\n");
          } else {
              printf("\nCompilation finished with %d syntax error(s) and %d semantic error(s).\n",
                     syntax_errors, semantic_errors);
          }
      }
    ;

external_list
    : external_list external
      {
          $$ = append_nodes($1, $2);
      }
    | /* empty */
      {
          $$ = empty_node();
      }
    ;

external
    : declaration
      {
          $$ = $1;
      }
    | statement
      {
          $$ = $1;
      }
    ;

declaration
    : INT declarator_list SEMICOLON
      {
          $$ = empty_node();   /* declarations do not emit TAC */
      }
    ;

declarator_list
    : ID
      {
          add_symbol($1);
      }
    | declarator_list COMMA ID
      {
          add_symbol($3);
      }
    ;

stmt_list
    : stmt_list statement
      {
          $$ = append_nodes($1, $2);
      }
    | /* empty */
      {
          $$ = empty_node();
      }
    ;

block
    : LBRACE stmt_list RBRACE
      {
          $$ = $2;
      }
    ;

statement
    : assignment SEMICOLON
      {
          $$ = $1;
      }
    | block
      {
          $$ = $1;
      }
    | IF LPAREN expr RPAREN statement %prec LOWER_THAN_ELSE
      {
          $$ = make_if($3, $5);
      }
    | IF LPAREN expr RPAREN statement ELSE statement
      {
          $$ = make_if_else($3, $5, $7);
      }
    | WHILE LPAREN expr RPAREN statement
      {
          $$ = make_while($3, $5);
      }
    | SEMICOLON
      {
          $$ = empty_node();
      }
    ;

assignment
    : ID ASSIGN expr
      {
          $$ = make_assignment($1, $3);
      }
    ;

expr
    : expr PLUS expr
      {
          $$ = make_binary("+", $1, $3);
      }
    | expr MINUS expr
      {
          $$ = make_binary("-", $1, $3);
      }
    | expr MUL expr
      {
          $$ = make_binary("*", $1, $3);
      }
    | expr DIV expr
      {
          $$ = make_binary("/", $1, $3);
      }
    | expr LT expr
      {
          $$ = make_binary("<", $1, $3);
      }
    | expr GT expr
      {
          $$ = make_binary(">", $1, $3);
      }
    | expr EQ expr
      {
          $$ = make_binary("==", $1, $3);
      }
    | expr NE expr
      {
          $$ = make_binary("!=", $1, $3);
      }
    | MINUS expr %prec UMINUS
      {
          $$ = make_unary_minus($2);
      }
    | LPAREN expr RPAREN
      {
          $$ = $2;
      }
    | ID
      {
          $$ = make_id_node($1);
      }
    | NUMBER
      {
          $$ = make_num_node($1);
      }
    ;

%%

void yyerror(const char *s) {
    syntax_errors++;
    fprintf(stderr, "Syntax error at line %d: %s\n", yylineno, s);
}

int main(int argc, char **argv) {
    if (argc > 1) {
        yyin = fopen(argv[1], "r");
        if (!yyin) {
            perror("fopen");
            return 1;
        }
    }

    if (yyparse() == 0) {
        /* parser already printed outputs in program rule */
    } else {
        fprintf(stderr, "Parsing failed.\n");
    }

    if (argc > 1 && yyin) {
        fclose(yyin);
    }
    return 0;
}