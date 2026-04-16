%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "toy.tab.h"

extern int dump_lex;
%}

%option yylineno noyywrap

DIGIT    [0-9]
LETTER   [a-zA-Z_]
ID       {LETTER}({LETTER}|{DIGIT})*
NUM      {DIGIT}+
WS       [ \t\r\n]+

%%

{WS}            { /* skip whitespace */ }

"//"[^\n]*      { /* skip single-line comments */ }

"int"           {
                    if (dump_lex) printf("KEYWORD    : int\n");
                    return INT;
                }
"if"            {
                    if (dump_lex) printf("KEYWORD    : if\n");
                    return IF;
                }
"else"          {
                    if (dump_lex) printf("KEYWORD    : else\n");
                    return ELSE;
                }
"while"         {
                    if (dump_lex) printf("KEYWORD    : while\n");
                    return WHILE;
                }

"<="            {
                    if (dump_lex) printf("RELOP      : <=\n");
                    yylval.str = strdup(yytext);
                    return RELOP;
                }
">="            {
                    if (dump_lex) printf("RELOP      : >=\n");
                    yylval.str = strdup(yytext);
                    return RELOP;
                }
"=="            {
                    if (dump_lex) printf("RELOP      : ==\n");
                    yylval.str = strdup(yytext);
                    return RELOP;
                }
"!="            {
                    if (dump_lex) printf("RELOP      : !=\n");
                    yylval.str = strdup(yytext);
                    return RELOP;
                }
"<"             {
                    if (dump_lex) printf("RELOP      : <\n");
                    yylval.str = strdup(yytext);
                    return RELOP;
                }
">"             {
                    if (dump_lex) printf("RELOP      : >\n");
                    yylval.str = strdup(yytext);
                    return RELOP;
                }

"="             {
                    if (dump_lex) printf("ASSIGN     : =\n");
                    return '=';
                }
"+"             {
                    if (dump_lex) printf("OP         : +\n");
                    return '+';
                }
"-"             {
                    if (dump_lex) printf("OP         : -\n");
                    return '-';
                }
"*"             {
                    if (dump_lex) printf("OP         : *\n");
                    return '*';
                }
"/"             {
                    if (dump_lex) printf("OP         : /\n");
                    return '/';
                }
"("             {
                    if (dump_lex) printf("LPAREN     : (\n");
                    return '(';
                }
")"             {
                    if (dump_lex) printf("RPAREN     : )\n");
                    return ')';
                }
"{"             {
                    if (dump_lex) printf("LBRACE     : {\n");
                    return '{';
                }
"}"             {
                    if (dump_lex) printf("RBRACE     : }\n");
                    return '}';
                }
";"             {
                    if (dump_lex) printf("SEMI       : ;\n");
                    return ';';
                }
","             {
                    if (dump_lex) printf("COMMA      : ,\n");
                    return ',';
                }

{ID}            {
                    if (dump_lex) printf("ID         : %s\n", yytext);
                    yylval.str = strdup(yytext);
                    return ID;
                }
{NUM}           {
                    if (dump_lex) printf("NUM        : %s\n", yytext);
                    yylval.str = strdup(yytext);
                    return NUM;
                }

.               {
                    fprintf(stderr, "Lexical Error at line %d: unknown character '%s'\n",
                            yylineno, yytext);
                    exit(1);
                }

%%
