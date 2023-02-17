/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

#ifndef YY_SATREADABLE_SAT_READABLEPARSER_HH_INCLUDED
# define YY_SATREADABLE_SAT_READABLEPARSER_HH_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int satreadable_debug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    KEY_EQUALS = 258,
    KEY_IMPLIES = 259,
    KEY_IMPLIES_BACK = 260,
    KEY_OR = 261,
    KEY_AND = 262,
    KEY_XOR = 263,
    KEY_NOT = 264,
    LBRACK = 265,
    RBRACK = 266,
    KEY_VARIABLE = 267,
    KEY_TRUE = 268,
    KEY_FALSE = 269,
    KEY_EOF = 270
  };
#endif
/* Tokens.  */
#define KEY_EQUALS 258
#define KEY_IMPLIES 259
#define KEY_IMPLIES_BACK 260
#define KEY_OR 261
#define KEY_AND 262
#define KEY_XOR 263
#define KEY_NOT 264
#define LBRACK 265
#define RBRACK 266
#define KEY_VARIABLE 267
#define KEY_TRUE 268
#define KEY_FALSE 269
#define KEY_EOF 270

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 11 "SAT/ReadableParser.yy" /* yacc.c:1909  */

  char * name;
  int id;

#line 89 "SAT/ReadableParser.hh" /* yacc.c:1909  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE satreadable_lval;

int satreadable_parse (void);

#endif /* !YY_SATREADABLE_SAT_READABLEPARSER_HH_INCLUDED  */
