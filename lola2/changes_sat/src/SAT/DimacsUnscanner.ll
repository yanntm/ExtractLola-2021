%option outfile="lex.yy.c"
%option prefix="dimacsun_"
%option noyywrap

%{
#include <SAT/SATSymbolTable.h>
void dimacsun_error (char const *s)
{
    fprintf (stderr, "%s\n", s);
}
extern int dimacsun_lex();
%}

%%

SAT\n           { SATSymbolTable::unparse_sat(1); }
UNSAT\n         { SATSymbolTable::unparse_sat(0); }
[ ]             {  }
-?[0-9][0-9]*   { SATSymbolTable::unparse_var(atoi(yytext)); }

.               { dimacsun_error("LEXICAL ERROR WHILE UNPARSING"); }

%%