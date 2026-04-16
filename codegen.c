#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/*
    Simple TAC -> Assembly translator

    Expected TAC forms produced by your parser:
      t0 = a + b
      t1 = a - b
      t2 = a * b
      t3 = a / b
      t4 = a < b
      t5 = a > b
      t6 = a == b
      t7 = a != b
      x = y
      x = 5
      if t0 goto L1
      goto L2
      L1:
*/

#define MAX_TOKENS 16
#define MAX_LINE    1024

static int is_integer(const char *s) {
    if (!s || !*s) return 0;
    if (*s == '-' && s[1] != '\0') s++;
    while (*s) {
        if (!isdigit((unsigned char)*s)) return 0;
        s++;
    }
    return 1;
}

static char *trim(char *s) {
    while (*s && isspace((unsigned char)*s)) s++;
    if (*s == '\0') return s;

    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return s;
}

static int split_tokens(char *line, char *tokens[], int max_tokens) {
    int n = 0;
    char *tok = strtok(line, " \t\r\n");
    while (tok && n < max_tokens) {
        tokens[n++] = tok;
        tok = strtok(NULL, " \t\r\n");
    }
    return n;
}

static void emit_load_operand(const char *reg, const char *operand) {
    if (is_integer(operand)) {
        printf("MOV %s, %s\n", reg, operand);
    } else {
        printf("LOAD %s, %s\n", reg, operand);
    }
}

static void emit_store_result(const char *result, const char *reg) {
    printf("STORE %s, %s\n", reg, result);
}

static void emit_arith(const char *lhs, const char *op1, const char *op, const char *op2) {
    emit_load_operand("R1", op1);
    emit_load_operand("R2", op2);

    if (strcmp(op, "+") == 0) {
        printf("ADD R1, R2\n");
        emit_store_result(lhs, "R1");
    } else if (strcmp(op, "-") == 0) {
        printf("SUB R1, R2\n");
        emit_store_result(lhs, "R1");
    } else if (strcmp(op, "*") == 0) {
        printf("MUL R1, R2\n");
        emit_store_result(lhs, "R1");
    } else if (strcmp(op, "/") == 0) {
        printf("DIV R1, R2\n");
        emit_store_result(lhs, "R1");
    } else {
        /* relational result: store 0 or 1 */
        printf("CMP R1, R2\n");
        printf("MOV R1, 0\n");

        const char *true_lbl = "LCMP_TRUE";
        const char *end_lbl  = "LCMP_END";

        if (strcmp(op, "<") == 0) {
            printf("JLT %s\n", true_lbl);
        } else if (strcmp(op, ">") == 0) {
            printf("JGT %s\n", true_lbl);
        } else if (strcmp(op, "==") == 0) {
            printf("JEQ %s\n", true_lbl);
        } else if (strcmp(op, "!=") == 0) {
            printf("JNE %s\n", true_lbl);
        } else {
            return;
        }

        printf("JMP %s\n", end_lbl);
        printf("%s:\n", true_lbl);
        printf("MOV R1, 1\n");
        printf("%s:\n", end_lbl);
        emit_store_result(lhs, "R1");
    }
}

static int looks_like_header(const char *line) {
    return (strncmp(line, "====", 4) == 0) ||
           (strncmp(line, "NAME", 4) == 0) ||
           (strncmp(line, "Compilation", 11) == 0) ||
           (strncmp(line, "Semantic error", 14) == 0) ||
           (strncmp(line, "Syntax error", 12) == 0);
}

int main(void) {
    char line[MAX_LINE];

    printf("; ===== ASSEMBLY GENERATED FROM TAC =====\n");

    while (fgets(line, sizeof(line), stdin)) {
        char *s = trim(line);
        if (*s == '\0') continue;
        if (looks_like_header(s)) continue;

        /* Make a writable copy for tokenization */
        char buf[MAX_LINE];
        strncpy(buf, s, sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';

        char *tokens[MAX_TOKENS];
        int n = split_tokens(buf, tokens, MAX_TOKENS);
        if (n == 0) continue;

        /* Label: L1: */
        {
            size_t len = strlen(tokens[0]);
            if (n == 1 && len > 0 && tokens[0][len - 1] == ':') {
                printf("%s\n", tokens[0]);
                continue;
            }
        }

        /* Unconditional jump: goto L1 */
        if (n == 2 && strcmp(tokens[0], "goto") == 0) {
            printf("JMP %s\n", tokens[1]);
            continue;
        }

        /* Conditional jump: if t0 goto L1 */
        if (n == 4 && strcmp(tokens[0], "if") == 0 && strcmp(tokens[2], "goto") == 0) {
            emit_load_operand("R1", tokens[1]);
            printf("MOV R2, 0\n");
            printf("CMP R1, R2\n");
            printf("JNE %s\n", tokens[3]);
            continue;
        }

        /* Assignment / binary op:
           x = y
           x = a + b
           x = a < b
        */
        if (n == 3 && strcmp(tokens[1], "=") == 0) {
            /* simple assignment */
            emit_load_operand("R1", tokens[2]);
            emit_store_result(tokens[0], "R1");
            continue;
        }

        if (n == 5 && strcmp(tokens[1], "=") == 0) {
            /* binary assignment */
            emit_arith(tokens[0], tokens[2], tokens[3], tokens[4]);
            continue;
        }

        /* Anything else is ignored safely */
    }

    return 0;
}