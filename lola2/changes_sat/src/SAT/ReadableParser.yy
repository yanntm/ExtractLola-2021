%defines
%name-prefix "satreadable_"

%{
#include <SAT/SATSymbolTable.h>

extern int satreadable_lex();
extern void satreadable_error(char const *);
%}

%union {
  char * name;
  int id;
}

%left KEY_EQUALS
%left KEY_IMPLIES KEY_IMPLIES_BACK
%left KEY_OR KEY_AND KEY_XOR
%right KEY_NOT


%token LBRACK RBRACK KEY_VARIABLE KEY_TRUE KEY_FALSE KEY_EOF

%type<id> clause

%%

clause:
    LBRACK clause RBRACK            { $$ = $2; }
  | clause KEY_AND clause           { $$ = SATSymbolTable::parse_clause(SAT_AND, $1, $3); }
  | clause KEY_OR clause            { $$ = SATSymbolTable::parse_clause(SAT_OR, $1, $3); }
  | clause KEY_XOR clause           { $$ = SATSymbolTable::parse_clause(SAT_XOR, $1, $3); }
  | clause KEY_EQUALS clause        { $$ = SATSymbolTable::parse_clause(SAT_EQUALS, $1, $3); }
  | clause KEY_IMPLIES clause       { $$ = SATSymbolTable::parse_clause(SAT_OR, SATSymbolTable::parse_clause(SAT_NOT, $1, -1), $3); }
  | clause KEY_IMPLIES_BACK clause  { $$ = SATSymbolTable::parse_clause(SAT_OR, SATSymbolTable::parse_clause(SAT_NOT, $3, -1), $1); }
  | KEY_NOT clause                  { $$ = SATSymbolTable::parse_clause(SAT_NOT, $2, -1); }
  | KEY_VARIABLE                    { $$ = SATSymbolTable::parse_variable(yyval.name); }
  | KEY_TRUE                        { $$ = std::numeric_limits<int>::max(); }
  | KEY_FALSE                       { $$ = std::numeric_limits<int>::min(); }
;

%%