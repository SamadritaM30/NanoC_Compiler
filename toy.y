    %{
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

    %}

    /* ================================================================
    Bison declarations
    ================================================================ */

    %code requires {
        typedef struct ASTNode ASTNode;
    }

    %union {
        char *str;
        ASTNode *node;
    }

    %token <str> ID NUM RELOP
    %token INT IF ELSE WHILE

    %type <str> cond if_header while_header
    %type <node> program stmts stmt decl idlist assign expr block
    %type <node> non_if_stmt matched_stmt unmatched_stmt else_prep

    /* Operator precedence (low → high) */
    %right '='
    %left  '+' '-'
    %left  '*' '/'
    %nonassoc RELOP
    /* Dangling-else fix */
    %nonassoc LOWER_THAN_ELSE
    %nonassoc ELSE

    %%

    /* ================================================================
    Grammar rules
    ================================================================ */

    program
        : stmts
        {
            root = $1;
        }
        
    stmts
        : stmts stmt
        {
            addChild($1, $2);
            $$ = $1;
        }
        | stmt
        {
            ASTNode *n = makeNode("stmts");
            addChild(n, $1);
            $$ = n;
        }
        ;

    stmt
        : matched_stmt
        {
            $$ = $1;
        }
        | unmatched_stmt
        {
            $$ = $1;
        }
    ;

    non_if_stmt
        : decl ';'
        {
            $$ = $1;
        }

        | assign ';'
        {
            $$ = $1;
        }

        | block
        {
            $$ = $1;
        }

        | ';'
        {
            $$ = makeNode("empty");
        }

        | error ';'
        {
            yyerrok;
            $$ = makeNode("error");
        }
        ;

    matched_stmt
        : non_if_stmt
        {
            $$ = $1;
        }
        | while_header matched_stmt
        {
            ASTNode *n = makeNode("while");
            addChild(n, makeNode("condition"));
            addChild(n, $2);

            emit("goto",  "", "", while_start());
            emit("label", "", "", while_end());
            pop_while();

            $$ = n;
        }
        | if_header matched_stmt else_prep ELSE matched_stmt
        {
            emit("label", "", "", if_end());
            pop_if();

            ASTNode *n = makeNode("if");
            addChild(n, makeNode("condition"));
            addChild(n, $2);
            addChild(n, $5);
            $$ = n;
        }
        ;

    unmatched_stmt
        : if_header stmt %prec LOWER_THAN_ELSE
        {
            emit("label", "", "", if_false());
            pop_if();
            $$ = $2;
        }
        | while_header unmatched_stmt
        {
            ASTNode *n = makeNode("while");
            addChild(n, makeNode("condition"));
            addChild(n, $2);

            emit("goto",  "", "", while_start());
            emit("label", "", "", while_end());
            pop_while();

            $$ = n;
        }
        | if_header matched_stmt else_prep ELSE unmatched_stmt
        {
            emit("label", "", "", if_end());
            pop_if();

            ASTNode *n = makeNode("if");
            addChild(n, makeNode("condition"));
            addChild(n, $2);
            addChild(n, $5);
            $$ = n;
        }
        ;

    else_prep
        :
        {
            /* After THEN, jump over ELSE and mark ELSE entry label. */
            emit("goto", "", "", if_end());
            emit("label", "", "", if_false());
            $$ = NULL;
        }
        ;

    block
        : '{' stmts '}'
        {
        ASTNode *n = makeNode("block");
        addChild(n, $2);
        $$ = n;
        }
        ;

    /* ── Declarations ──────────────────────────────────────────────── */

    decl
        : INT idlist
        {
            ASTNode *n = makeNode("decl(int)");
            addChild(n, $2);
            $$ = n;
        }

    idlist
        : idlist ',' ID
        {
            declare_var($3, "int");
            addChild($1, makeNode($3));
            $$ = $1;
            free($3);
        }
        | ID
        {
            declare_var($1, "int");
            ASTNode *n = makeNode("idlist");
            addChild(n, makeNode($1));
            $$ = n;
            free($1);
        }
        ;
    /* ── Assignment ────────────────────────────────────────────────── */

    assign
        : ID '=' expr
        {
            int errs_before = semantic_error_count;
            use_var($1);
            if (semantic_error_count == errs_before)
                emit("=", $3->place, "", $1);

            ASTNode *n = makeNode("=");
            addChild(n, makeNode($1));
            addChild(n, $3);
            $$ = n;

            free($1);
        }
        ;

    /* ── Expressions ───────────────────────────────────────────────── */

    expr
        : expr '+' expr
        {
            char *t = new_temp();
            emit("+", $1->place, $3->place, t);

            ASTNode *n = makeNode("+");
            addChild(n, $1);
            addChild(n, $3);
            strcpy(n->place, t); 
            $$ = n;
        }
        | expr '-' expr
        {
            char *t = new_temp();
            emit("-", $1->place, $3->place, t);

            ASTNode *n = makeNode("-");
            addChild(n, $1);
            addChild(n, $3);
            strcpy(n->place, t); 
            $$ = n;
        }
        | expr '*' expr
        {
            char *t = new_temp();
            emit("*", $1->place, $3->place, t);

            ASTNode *n = makeNode("*");
            addChild(n, $1);
            addChild(n, $3);
            strcpy(n->place, t); 
            $$ = n;
        }
        | expr '/' expr
        {
            char *t = new_temp();
            emit("/", $1->place, $3->place, t);

            ASTNode *n = makeNode("/");
            addChild(n, $1);
            addChild(n, $3);
            strcpy(n->place, t); 
            $$ = n;
        }
        | '(' expr ')'
        {
            $$ = $2;
        }
        | ID
        {
            use_var($1);
            ASTNode *n = makeNode($1);
            strcpy(n->place, $1);
            $$ = n;
        }
        | NUM
        {
            ASTNode *n = makeNode($1);
            strcpy(n->place, $1);
            $$ = n;
        }
        ;

    /* ── Conditions ────────────────────────────────────────────────── */

    cond
        : expr RELOP expr
        {
            char *s = malloc(strlen($1->label) + strlen($2) + strlen($3->label) + 4);
            sprintf(s, "%s %s %s", $1->label, $2, $3->label);

            free($2);  // RELOP string
            $$ = s;
        }
        ;

    /* ── Named markers to avoid mid-rule-action reduce/reduce conflicts ── */

    if_header
        : IF '(' cond ')'
        {
            push_if();
            emit("ifgoto", $3, "", if_true());
            emit("goto",   "",  "", if_false());
            emit("label",  "",  "", if_true());
            free($3);
            $$ = NULL;
        }
        ;

    while_header
        : WHILE
        {
            push_while();
            emit("label", "", "", while_start());
        }
        '(' cond ')'
        {
            emit("ifgoto", $4, "", while_body());
            emit("goto",   "",  "", while_end());
            emit("label",  "",  "", while_body());
            free($4);
            $$ = NULL;
        }
        ;

    %%

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