#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SYMBOLS 256

typedef struct {
    char *name;
    int is_const;
    char *const_val;
} Symbol;

typedef struct {
    Symbol table[MAX_SYMBOLS];
    int count;
} SymbolTable;

static inline void st_init(SymbolTable *st) {
    st->count = 0;
}

static inline int st_find(SymbolTable *st, const char *name) {
    for (int i = 0; i < st->count; i++) {
        if (strcmp(st->table[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

static inline int st_exists(SymbolTable *st, const char *name) {
    return st_find(st, name) != -1;
}

static inline void st_add(SymbolTable *st, const char *name) {
    if (st->count >= MAX_SYMBOLS) {
        fprintf(stderr, "Symbol table overflow\n");
        exit(1);
    }

    if (st_exists(st, name)) {
        fprintf(stderr, "Semantic error: duplicate declaration of '%s'\n", name);
        return;
    }

    st->table[st->count].name = strdup(name);
    st->table[st->count].is_const = 0;
    st->table[st->count].const_val = NULL;
    st->count++;
}

static inline void st_set_const(SymbolTable *st, const char *name, const char *value) {
    int idx = st_find(st, name);
    if (idx < 0) return;

    st->table[idx].is_const = 1;
    if (st->table[idx].const_val) {
        free(st->table[idx].const_val);
    }
    st->table[idx].const_val = strdup(value);
}

static inline void st_clear_const(SymbolTable *st, const char *name) {
    int idx = st_find(st, name);
    if (idx < 0) return;

    st->table[idx].is_const = 0;
    if (st->table[idx].const_val) {
        free(st->table[idx].const_val);
        st->table[idx].const_val = NULL;
    }
}

static inline const char *st_get_const(SymbolTable *st, const char *name) {
    int idx = st_find(st, name);
    if (idx < 0) return NULL;
    if (!st->table[idx].is_const) return NULL;
    return st->table[idx].const_val;
}

static inline void st_print(SymbolTable *st) {
    printf("\n===== SYMBOL TABLE =====\n");
    printf("%-16s %-8s %-12s\n", "NAME", "TYPE", "CONST_VALUE");
    printf("------------------------------------------\n");
    for (int i = 0; i < st->count; i++) {
        printf("%-16s %-8s %-12s\n",
               st->table[i].name,
               "int",
               st->table[i].is_const ? st->table[i].const_val : "-");
    }
}

#endif