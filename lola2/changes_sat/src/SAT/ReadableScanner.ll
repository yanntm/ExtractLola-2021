%option outfile="lex.yy.c"
%option prefix="satreadable_"
%option noyywrap
%option yylineno

%{
#include <SAT/ReadableParser.hh>
extern void satreadable_error (char const *);
extern int satreadable_lex();
%}

%%

\(          {      return LBRACK; }
\)          {      return RBRACK; }
AND         {      return KEY_AND; }
&&          {      return KEY_AND; }
OR          {      return KEY_OR; }
\|\|        {      return KEY_OR; }
XOR         {      return KEY_XOR; }
\^          {      return KEY_XOR; }
NOT         {      return KEY_NOT; }
!           {      return KEY_NOT; }
EQUALS      {      return KEY_EQUALS; }
\<\-\>      {      return KEY_EQUALS; }
IMPLIES     {      return KEY_IMPLIES; }
\-\>        {      return KEY_IMPLIES; }
\<\-        {      return KEY_IMPLIES_BACK; }
ATOM_TRUE        {      return KEY_TRUE; }
ATOM_FALSE       {      return KEY_FALSE; }

\/\/.*\n      { }

[a-zA-Z]([0-9a-zA-Z\._])*     {  satreadable_lval.name = strdup(yytext); return KEY_VARIABLE; }

[ \r\t\n]                         {  }

.           { fprintf(stderr, "error: %s", yytext); satreadable_error("LEXICAL ERROR"); }

%%

void satreadable_error (char const *s)
{
    fprintf(stderr, "%d: %s at %s\n", yylineno, s, yytext);
}