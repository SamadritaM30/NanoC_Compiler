#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

/*
    Optional TAC utility file.
    Your current parser already generates TAC internally,
    so this file is only for clean project organization or future refactor.
*/

typedef struct TACNode {
    char *place;
    char *code;
} TACNode;

static int temp_count = 0;
static int label_count = 0;

static char *xstrdup2(const char *s) {
    if (!s) return NULL;
    char *p = (char *)malloc(strlen(s) + 1);
    if (!p) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    strcpy(p, s);
    return p;
}

static char *xasprintf2(const char *fmt, ...) {
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

TACNode *tac_new(const char *place, const char *code) {
    TACNode *n = (TACNode *)malloc(sizeof(TACNode));
    if (!n) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    n->place = place ? xstrdup2(place) : NULL;
    n->code = code ? xstrdup2(code) : xstrdup2("");
    return n;
}

TACNode *tac_empty(void) {
    return tac_new(NULL, "");
}

char *tac_new_temp(void) {
    char buf[32];
    snprintf(buf, sizeof(buf), "t%d", temp_count++);
    return xstrdup2(buf);
}

char *tac_new_label(void) {
    char buf[32];
    snprintf(buf, sizeof(buf), "L%d", label_count++);
    return xstrdup2(buf);
}

TACNode *tac_append(TACNode *a, TACNode *b) {
    if (!a && !b) return tac_empty();
    if (!a) return tac_new(NULL, b->code);
    if (!b) return tac_new(NULL, a->code);

    char *merged = xasprintf2("%s%s", a->code, b->code);
    return tac_new(NULL, merged);
}

TACNode *tac_emit_assign(const char *lhs, const char *rhs) {
    char *code = xasprintf2("%s = %s\n", lhs, rhs);
    return tac_new(lhs, code);
}

TACNode *tac_emit_binary(const char *lhs, const char *op1, const char *op, const char *op2) {
    char *code = xasprintf2("%s = %s %s %s\n", lhs, op1, op, op2);
    return tac_new(lhs, code);
}

TACNode *tac_emit_if_goto(const char *cond, const char *label_true) {
    char *code = xasprintf2("if %s goto %s\n", cond, label_true);
    return tac_new(NULL, code);
}

TACNode *tac_emit_goto(const char *label) {
    char *code = xasprintf2("goto %s\n", label);
    return tac_new(NULL, code);
}

TACNode *tac_emit_label(const char *label) {
    char *code = xasprintf2("%s:\n", label);
    return tac_new(NULL, code);
}