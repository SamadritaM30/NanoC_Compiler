%option noyywrap yylineno

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.tab.h"   /* generated later by bison -d parser.y */

static void lex_error(const char *msg) {
    fprintf(stderr, "Lexical error at line %d: %s\n", yylineno, msg);
}
%}

%x COMMENT

DIGIT      [0-9]
ID_START   [A-Za-z_]
ID_CONT    [A-Za-z0-9_]
ID         {ID_START}{ID_CONT}*
NUM        {DIGIT}+

%%

/* ---------- whitespace ---------- */
[ \t\r]+                 ;
\n                       ;

/* ---------- comments ---------- */
"//".*                   ;
"/*"                     { BEGIN(COMMENT); }

<COMMENT>"*/"            { BEGIN(INITIAL); }
<COMMENT>\n              ;
<COMMENT>.               ;
<COMMENT><<EOF>>         {
                            lex_error("unterminated block comment");
                            return 0;
                         }

/* ---------- keywords ---------- */
"int"                    { return INT; }
"if"                     { return IF; }
"else"                   { return ELSE; }
"while"                  { return WHILE; }

/* ---------- operators ---------- */
"=="                     { return EQ; }
"!="                     { return NE; }
"<"                      { return LT; }
">"                      { return GT; }
"+"                      { return PLUS; }
"-"                      { return MINUS; }
"*"                      { return MUL; }
"/"                      { return DIV; }
"="                      { return ASSIGN; }

/* ---------- punctuation ---------- */
";"                      { return SEMICOLON; }
","                      { return COMMA; }
"("                      { return LPAREN; }
")"                      { return RPAREN; }
"{"                      { return LBRACE; }
"}"                      { return RBRACE; }

/* ---------- identifiers ---------- */
{ID}                     {
                            yylval.id = strdup(yytext);
                            return ID;
                         }

/* ---------- integers ---------- */
{NUM}                    {
                            yylval.num = atoi(yytext);
                            return NUMBER;
                         }

/* ---------- invalid identifiers like 12abc ---------- */
{DIGIT}+{ID_CONT}+       {
                            char msg[128];
                            snprintf(msg, sizeof(msg), "invalid token '%s'", yytext);
                            lex_error(msg);
                         }

/* ---------- anything else ---------- */
.                        {
                            char msg[64];
                            snprintf(msg, sizeof(msg), "unexpected character '%s'", yytext);
                            lex_error(msg);
                         }

%%

int yywrap(void) {
    return 1;
}