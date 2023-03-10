/* A Bison parser, made by GNU Bison 3.3.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2019 Free Software Foundation,
   Inc.

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Undocumented macros, especially those whose name start with YY_,
   are private implementation details.  Do not rely on them.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.3.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1


/* Substitute the variable and function names.  */
#define yyparse         ptbuechi_parse
#define yylex           ptbuechi_lex
#define yyerror         ptbuechi_error
#define yydebug         ptbuechi_debug
#define yynerrs         ptbuechi_nerrs

#define yylval          ptbuechi_lval
#define yychar          ptbuechi_char
#define yylloc          ptbuechi_lloc

/* First part of user prologue.  */
#line 28 "Frontend/Parser/ParserBuechi.yy" /* yacc.c:337  */

#include <config.h>
#include <Frontend/SymbolTable/PlaceSymbol.h>
#include <Frontend/SymbolTable/TransitionSymbol.h>
#include <Frontend/SymbolTable/SymbolTable.h>
#include <Frontend/Parser/ParserPTNet.h>
#include <Frontend/Parser/error.h>
#include <Frontend/Parser/ast-system-k.h>       // for kc namespace
#include <Frontend/Parser/ast-system-yystype.h> // for YYSTYPE
#include <Formula/StatePredicate/DeadlockPredicate.h>
#include <Formula/StatePredicate/AtomicBooleanPredicate.h>
#include <Formula/StatePredicate/FireablePredicate.h>
#include <Formula/StatePredicate/TruePredicate.h>
#include <Formula/StatePredicate/FalsePredicate.h>
#include <Formula/StatePredicate/MagicNumber.h>
#include <Net/Petrinet.h>


#include <limits.h>
#include <libgen.h>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <set>

extern SymbolTable* buechiStateTable;
tShape ooppShape(tShape s);
#line 118 "Frontend/Parser/ParserBuechi.yy" /* yacc.c:337  */

// parser essentials
extern int ptbuechi_lex();
void ptbuechi_error(char const*);

int currentNextIndex = 0;
#line 126 "Frontend/Parser/ParserBuechi.yy" /* yacc.c:337  */

/* globals */
tBuechiAutomata TheBuechi;
uint32_t currentState;

#line 120 "Frontend/Parser/ParserBuechi.cc" /* yacc.c:337  */
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 1
#endif

/* In a future release of Bison, this section will be replaced
   by #include "y.tab.h".  */
#ifndef YY_PTBUECHI_FRONTEND_PARSER_PARSERBUECHI_HH_INCLUDED
# define YY_PTBUECHI_FRONTEND_PARSER_PARSERBUECHI_HH_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int ptbuechi_debug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    END = 0,
    IDENTIFIER = 258,
    NUMBER = 259,
    _accept_ = 260,
    _buechi_ = 261,
    _braceleft_ = 262,
    _braceright_ = 263,
    _comma_ = 264,
    _then_ = 265,
    _colon_ = 266,
    _INITIAL_ = 267,
    _DEADLOCK_ = 268,
    _AND_ = 269,
    _NOT_ = 270,
    _OR_ = 271,
    _XOR_ = 272,
    _iff_ = 273,
    _notequal_ = 274,
    _implies_ = 275,
    _equals_ = 276,
    _plus_ = 277,
    _minus_ = 278,
    _times_ = 279,
    _leftparenthesis_ = 280,
    _rightparenthesis_ = 281,
    _greaterthan_ = 282,
    _lessthan_ = 283,
    _greaterorequal_ = 284,
    _lessorequal_ = 285,
    _semicolon_ = 286,
    _TRUE_ = 287,
    _FALSE_ = 288,
    _FIREABLE_ = 289
  };
#endif
/* Tokens.  */
#define END 0
#define IDENTIFIER 258
#define NUMBER 259
#define _accept_ 260
#define _buechi_ 261
#define _braceleft_ 262
#define _braceright_ 263
#define _comma_ 264
#define _then_ 265
#define _colon_ 266
#define _INITIAL_ 267
#define _DEADLOCK_ 268
#define _AND_ 269
#define _NOT_ 270
#define _OR_ 271
#define _XOR_ 272
#define _iff_ 273
#define _notequal_ 274
#define _implies_ 275
#define _equals_ 276
#define _plus_ 277
#define _minus_ 278
#define _times_ 279
#define _leftparenthesis_ 280
#define _rightparenthesis_ 281
#define _greaterthan_ 282
#define _lessthan_ 283
#define _greaterorequal_ 284
#define _lessorequal_ 285
#define _semicolon_ 286
#define _TRUE_ 287
#define _FALSE_ 288
#define _FIREABLE_ 289

/* Value type.  */

/* Location type.  */
#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE YYLTYPE;
struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
};
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif


extern YYSTYPE ptbuechi_lval;
extern YYLTYPE ptbuechi_lloc;
int ptbuechi_parse (void);

#endif /* !YY_PTBUECHI_FRONTEND_PARSER_PARSERBUECHI_HH_INCLUDED  */



#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef YY_ATTRIBUTE
# if (defined __GNUC__                                               \
      && (2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__)))  \
     || defined __SUNPRO_C && 0x5110 <= __SUNPRO_C
#  define YY_ATTRIBUTE(Spec) __attribute__(Spec)
# else
#  define YY_ATTRIBUTE(Spec) /* empty */
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# define YY_ATTRIBUTE_PURE   YY_ATTRIBUTE ((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE ((__unused__))
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && ! defined __ICC && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif


#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL \
             && defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
  YYLTYPE yyls_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE) + sizeof (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYSIZE_T yynewbytes;                                            \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / sizeof (*yyptr);                          \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  4
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   91

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  35
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  11
/* YYNRULES -- Number of rules.  */
#define YYNRULES  38
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  76

#define YYUNDEFTOK  2
#define YYMAXUTOK   289

/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                                \
  ((unsigned) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   135,   135,   142,   143,   145,   151,   150,   169,   170,
     184,   185,   194,   206,   208,   224,   255,   305,   356,   541,
     597,   781,   794,   808,   822,   831,   844,   853,   860,   867,
     879,   910,   920,   922,   934,   941,   946,   952,   960
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 1
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "$undefined", "\"identifier\"",
  "\"number\"", "\"keyword accept\"", "\"keyword buechi\"",
  "\"opening brace\"", "\"closing brace\"", "\"comma\"",
  "\"transition =>\"", "\"colon\"", "\"keyword INITIAL\"",
  "\"keyword DEADLOCK\"", "\"Boolean conjuction\"", "\"Boolean negation\"",
  "\"Boolean disjunction\"", "\"Boolean exclusive disjunction\"",
  "\"Boolean iff\"", "\"not-equals sign\"", "\"Boolean implication\"",
  "\"equals sign\"", "\"plus sign\"", "\"minus sign\"",
  "\"multiplication sign\"", "\"opening parenthesis\"",
  "\"closing parenthesis\"", "\"greater-than sign\"", "\"less-than sign\"",
  "\"greater-than-or-equal sign\"", "\"less-than-or-equal sign\"",
  "\"semicolon\"", "\"Boolean TRUE\"", "\"Boolean FALSE\"",
  "\"keyword FIREABLE\"", "$accept", "buechiAutomata", "buechiRules",
  "buechiRule", "$@1", "transitionRules", "acceptingsets",
  "statepredicate", "atomic_proposition", "term", "identifier", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289
};
# endif

#define YYPACT_NINF -22

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-22)))

#define YYTABLE_NINF -1

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int8 yypact[] =
{
      -3,     7,    18,    27,   -22,   -22,    12,    23,    31,    36,
      27,     4,    42,   -22,   -22,    19,   -22,   -22,     4,     4,
     -22,   -22,    20,   -22,    71,   -22,    50,   -22,    55,     8,
     -22,    30,    38,    59,    60,     4,     4,     4,     4,     4,
       8,     8,     8,     8,     8,     8,     8,     8,    61,    66,
       8,   -22,   -22,   -22,    49,     4,   -14,    -5,    -5,   -22,
      58,    17,    17,   -22,   -22,    17,    17,    17,    17,    55,
      51,   -21,   -22,   -22,   -22,   -22
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     0,     0,     3,     1,     6,     0,     4,     0,     0,
       3,     8,     0,     5,    38,    34,    30,    31,     0,     0,
      27,    28,     0,     7,     0,    14,     0,    33,    10,     0,
      15,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    11,     0,
       0,    37,    13,    32,     0,     8,    16,    17,    18,    20,
      19,    22,    21,    35,    36,    23,    25,    24,    26,    10,
       0,     0,    29,     9,    12,     2
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -22,   -22,    73,   -22,   -22,    29,    21,    16,   -22,   -19,
      53
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
      -1,     2,     6,     7,     8,    23,    49,    24,    25,    26,
      27
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] =
{
      32,    42,    43,     1,    38,    53,    39,    14,    15,    35,
      51,    14,    15,    38,     3,    39,    16,    17,     4,    18,
       9,    61,    62,    63,    64,    65,    66,    67,    68,    19,
       5,    71,    10,    50,    30,    31,    20,    21,    22,    42,
      43,    12,    11,    29,    35,    33,    36,    37,    38,    28,
      39,    56,    57,    58,    59,    60,    52,    40,    48,    41,
      42,    43,    14,    55,    53,    44,    45,    46,    47,    40,
      69,    41,    42,    43,    70,    72,    38,    44,    45,    46,
      47,    34,    75,    13,    73,    35,    54,    36,    37,    38,
      74,    39
};

static const yytype_uint8 yycheck[] =
{
      19,    22,    23,     6,    18,    26,    20,     3,     4,    14,
      29,     3,     4,    18,     7,    20,    12,    13,     0,    15,
       8,    40,    41,    42,    43,    44,    45,    46,    47,    25,
       3,    50,     9,    25,    18,    19,    32,    33,    34,    22,
      23,     5,    11,    24,    14,    25,    16,    17,    18,     7,
      20,    35,    36,    37,    38,    39,    26,    19,     3,    21,
      22,    23,     3,     3,    26,    27,    28,    29,    30,    19,
       9,    21,    22,    23,     8,    26,    18,    27,    28,    29,
      30,    10,    31,    10,    55,    14,    33,    16,    17,    18,
      69,    20
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     6,    36,     7,     0,     3,    37,    38,    39,     8,
       9,    11,     5,    37,     3,     4,    12,    13,    15,    25,
      32,    33,    34,    40,    42,    43,    44,    45,     7,    24,
      42,    42,    44,    25,    10,    14,    16,    17,    18,    20,
      19,    21,    22,    23,    27,    28,    29,    30,     3,    41,
      25,    44,    26,    26,    45,     3,    42,    42,    42,    42,
      42,    44,    44,    44,    44,    44,    44,    44,    44,     9,
       8,    44,    26,    40,    41,    31
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    35,    36,    37,    37,    37,    39,    38,    40,    40,
      41,    41,    41,    42,    42,    42,    42,    42,    42,    42,
      42,    43,    43,    43,    43,    43,    43,    43,    43,    43,
      43,    43,    44,    44,    44,    44,    44,    44,    45
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     9,     0,     1,     3,     0,     4,     0,     4,
       0,     1,     3,     3,     1,     2,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     1,     1,     4,
       1,     1,     3,     1,     1,     3,     3,     3,     1
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)                                \
    do                                                                  \
      if (N)                                                            \
        {                                                               \
          (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;        \
          (Current).first_column = YYRHSLOC (Rhs, 1).first_column;      \
          (Current).last_line    = YYRHSLOC (Rhs, N).last_line;         \
          (Current).last_column  = YYRHSLOC (Rhs, N).last_column;       \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).first_line   = (Current).last_line   =              \
            YYRHSLOC (Rhs, 0).last_line;                                \
          (Current).first_column = (Current).last_column =              \
            YYRHSLOC (Rhs, 0).last_column;                              \
        }                                                               \
    while (0)
#endif

#define YYRHSLOC(Rhs, K) ((Rhs)[K])


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL

/* Print *YYLOCP on YYO.  Private, do not rely on its existence. */

YY_ATTRIBUTE_UNUSED
static int
yy_location_print_ (FILE *yyo, YYLTYPE const * const yylocp)
{
  int res = 0;
  int end_col = 0 != yylocp->last_column ? yylocp->last_column - 1 : 0;
  if (0 <= yylocp->first_line)
    {
      res += YYFPRINTF (yyo, "%d", yylocp->first_line);
      if (0 <= yylocp->first_column)
        res += YYFPRINTF (yyo, ".%d", yylocp->first_column);
    }
  if (0 <= yylocp->last_line)
    {
      if (yylocp->first_line < yylocp->last_line)
        {
          res += YYFPRINTF (yyo, "-%d", yylocp->last_line);
          if (0 <= end_col)
            res += YYFPRINTF (yyo, ".%d", end_col);
        }
      else if (0 <= end_col && yylocp->first_column < end_col)
        res += YYFPRINTF (yyo, "-%d", end_col);
    }
  return res;
 }

#  define YY_LOCATION_PRINT(File, Loc)          \
  yy_location_print_ (File, &(Loc))

# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value, Location); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
{
  FILE *yyoutput = yyo;
  YYUSE (yyoutput);
  YYUSE (yylocationp);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyo, yytoknum[yytype], *yyvaluep);
# endif
  YYUSE (yytype);
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
{
  YYFPRINTF (yyo, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  YY_LOCATION_PRINT (yyo, *yylocationp);
  YYFPRINTF (yyo, ": ");
  yy_symbol_value_print (yyo, yytype, yyvaluep, yylocationp);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, YYLTYPE *yylsp, int yyrule)
{
  unsigned long yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[yyssp[yyi + 1 - yynrhs]],
                       &yyvsp[(yyi + 1) - (yynrhs)]
                       , &(yylsp[(yyi + 1) - (yynrhs)])                       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, yylsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
yystrlen (const char *yystr)
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
        switch (*++yyp)
          {
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++yyp != '\\')
              goto do_not_strip_quotes;
            else
              goto append;

          append:
          default:
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return (YYSIZE_T) (yystpcpy (yyres, yystr) - yyres);
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
                  if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
                    yysize = yysize1;
                  else
                    return 2;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
    default: /* Avoid compiler warnings. */
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
      yysize = yysize1;
    else
      return 2;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, YYLTYPE *yylocationp)
{
  YYUSE (yyvaluep);
  YYUSE (yylocationp);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Location data for the lookahead symbol.  */
YYLTYPE yylloc
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
  = { 1, 1, 1, 1 }
# endif
;
/* Number of syntax errors so far.  */
int yynerrs;


/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.
       'yyls': related to locations.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    /* The location stack.  */
    YYLTYPE yylsa[YYINITDEPTH];
    YYLTYPE *yyls;
    YYLTYPE *yylsp;

    /* The locations where the error started and ended.  */
    YYLTYPE yyerror_range[3];

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yylsp = yyls = yylsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
  yylsp[0] = yylloc;
  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yynewstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  *yyssp = (yytype_int16) yystate;

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    goto yyexhaustedlab;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = (YYSIZE_T) (yyssp - yyss + 1);

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        YYSTYPE *yyvs1 = yyvs;
        yytype_int16 *yyss1 = yyss;
        YYLTYPE *yyls1 = yyls;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * sizeof (*yyssp),
                    &yyvs1, yysize * sizeof (*yyvsp),
                    &yyls1, yysize * sizeof (*yylsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
        yyls = yyls1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yytype_int16 *yyss1 = yyss;
        union yyalloc *yyptr =
          (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
        if (! yyptr)
          goto yyexhaustedlab;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
        YYSTACK_RELOCATE (yyls_alloc, yyls);
# undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
                  (unsigned long) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END
  *++yylsp = yylloc;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

  /* Default location. */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  yyerror_range[1] = yyloc;
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
#line 137 "Frontend/Parser/ParserBuechi.yy" /* yacc.c:1652  */
    { TheBuechi = BuechiAutomaton((yyvsp[-6].yt_tBuechiRules),(yyvsp[-2].yt_tAcceptingSet)); }
#line 1481 "Frontend/Parser/ParserBuechi.cc" /* yacc.c:1652  */
    break;

  case 3:
#line 142 "Frontend/Parser/ParserBuechi.yy" /* yacc.c:1652  */
    { (yyval.yt_tBuechiRules) = EmptyBuechiRules(); }
#line 1487 "Frontend/Parser/ParserBuechi.cc" /* yacc.c:1652  */
    break;

  case 4:
#line 144 "Frontend/Parser/ParserBuechi.yy" /* yacc.c:1652  */
    { (yyval.yt_tBuechiRules) = (yyvsp[0].yt_tBuechiRules); }
#line 1493 "Frontend/Parser/ParserBuechi.cc" /* yacc.c:1652  */
    break;

  case 5:
#line 146 "Frontend/Parser/ParserBuechi.yy" /* yacc.c:1652  */
    { (yyval.yt_tBuechiRules) = BuechiRules((yyvsp[-2].yt_tBuechiRules),(yyvsp[0].yt_tBuechiRules)); }
#line 1499 "Frontend/Parser/ParserBuechi.cc" /* yacc.c:1652  */
    break;

  case 6:
#line 151 "Frontend/Parser/ParserBuechi.yy" /* yacc.c:1652  */
    {
  	      Symbol *t = buechiStateTable->lookup((yyvsp[0].yt_casestring)->name);
  	      if (t == NULL)
          {
  		      t = new Symbol((yyvsp[0].yt_casestring)->name);
  		      buechiStateTable->insert(t);
  		      t->setIndex(currentNextIndex++);
  	      }
      }
#line 1513 "Frontend/Parser/ParserBuechi.cc" /* yacc.c:1652  */
    break;

  case 7:
#line 161 "Frontend/Parser/ParserBuechi.yy" /* yacc.c:1652  */
    {
          Symbol *t = buechiStateTable->lookup((yyvsp[-3].yt_casestring)->name);
          (yyval.yt_tBuechiRules) = BuechiRule((mkinteger(t->getIndex())),(yyvsp[0].yt_tTransitionRules)); (yyvsp[-3].yt_casestring)->free(true);
      }
#line 1522 "Frontend/Parser/ParserBuechi.cc" /* yacc.c:1652  */
    break;

  case 8:
#line 169 "Frontend/Parser/ParserBuechi.yy" /* yacc.c:1652  */
    { (yyval.yt_tTransitionRules) = EmptyTransitionRules(); }
#line 1528 "Frontend/Parser/ParserBuechi.cc" /* yacc.c:1652  */
    break;

  case 9:
#line 171 "Frontend/Parser/ParserBuechi.yy" /* yacc.c:1652  */
    {
        Symbol *t = buechiStateTable->lookup((yyvsp[-1].yt_casestring)->name);
	  	if (UNLIKELY(t == NULL)){
	  		buechiStateTable->insert(new Symbol((yyvsp[-1].yt_casestring)->name));
	  		t = buechiStateTable->lookup((yyvsp[-1].yt_casestring)->name);
	  		t->setIndex(currentNextIndex++);
	  	}
        (yyval.yt_tTransitionRules) = TransitionRules(TransitionRule(StatePredicateFormula((yyvsp[-3].yt_tStatePredicate)),mkinteger(t->getIndex())),(yyvsp[0].yt_tTransitionRules));
    }
#line 1542 "Frontend/Parser/ParserBuechi.cc" /* yacc.c:1652  */
    break;

  case 10:
#line 184 "Frontend/Parser/ParserBuechi.yy" /* yacc.c:1652  */
    { (yyval.yt_tAcceptingSet) = EmptyAcceptingSet(); }
#line 1548 "Frontend/Parser/ParserBuechi.cc" /* yacc.c:1652  */
    break;

  case 11:
#line 186 "Frontend/Parser/ParserBuechi.yy" /* yacc.c:1652  */
    {
        Symbol *t = buechiStateTable->lookup((yyvsp[0].yt_casestring)->name);
        if (UNLIKELY(t == NULL))
        {
            yyerrors((yyvsp[0].yt_casestring)->name, (yylsp[0]), "state '%s' unknown", (yyvsp[0].yt_casestring)->name);
        }
        (yyval.yt_tAcceptingSet) = AcceptingState(mkinteger(t->getIndex()));
    }
#line 1561 "Frontend/Parser/ParserBuechi.cc" /* yacc.c:1652  */
    break;

  case 12:
#line 195 "Frontend/Parser/ParserBuechi.yy" /* yacc.c:1652  */
    {
        Symbol *t = buechiStateTable->lookup((yyvsp[-2].yt_casestring)->name);
        if (UNLIKELY(t == NULL))
        {
            yyerrors((yyvsp[-2].yt_casestring)->name, (yylsp[-2]), "state '%s' unknown", (yyvsp[-2].yt_casestring)->name);
        }
        (yyval.yt_tAcceptingSet) = AcceptingSet(AcceptingState(mkinteger(t->getIndex())),(yyvsp[0].yt_tAcceptingSet));
    }
#line 1574 "Frontend/Parser/ParserBuechi.cc" /* yacc.c:1652  */
    break;

  case 13:
#line 207 "Frontend/Parser/ParserBuechi.yy" /* yacc.c:1652  */
    { (yyval.yt_tStatePredicate) = (yyvsp[-1].yt_tStatePredicate); }
#line 1580 "Frontend/Parser/ParserBuechi.cc" /* yacc.c:1652  */
    break;

  case 14:
#line 209 "Frontend/Parser/ParserBuechi.yy" /* yacc.c:1652  */
    { 
	if((yyvsp[0].yt_tAtomicProposition)->pred->magicnumber == MAGIC_NUMBER_TRUE)
	{
		delete (yyvsp[0].yt_tAtomicProposition)->pred;
		(yyvsp[0].yt_tAtomicProposition) -> pred = new TruePredicate(Petrinet::InitialNet);
	}
	else if((yyvsp[0].yt_tAtomicProposition)->pred->magicnumber == MAGIC_NUMBER_FALSE)
	{
		delete (yyvsp[0].yt_tAtomicProposition)->pred;
		(yyvsp[0].yt_tAtomicProposition) -> pred = new FalsePredicate(Petrinet::InitialNet);
	}
	(yyval.yt_tStatePredicate) = AtomicProposition((yyvsp[0].yt_tAtomicProposition)); 
	(yyval.yt_tStatePredicate) -> shape = (yyvsp[0].yt_tAtomicProposition) -> shape;
	(yyval.yt_tStatePredicate) -> formula = (yyvsp[0].yt_tAtomicProposition) -> pred;
    }
#line 1600 "Frontend/Parser/ParserBuechi.cc" /* yacc.c:1652  */
    break;

  case 15:
#line 225 "Frontend/Parser/ParserBuechi.yy" /* yacc.c:1652  */
    { 
	switch((yyvsp[0].yt_tStatePredicate) -> shape)
	{
	case AT_TEMP: 	(yyval.yt_tStatePredicate) = Negation((yyvsp[0].yt_tStatePredicate)); 
			break;
	case AT_DL:
	case AT_FIR:
	case AT_COMP: 	(yyvsp[0].yt_tStatePredicate) -> formula = (yyvsp[0].yt_tStatePredicate) -> formula -> negate(); 
			(yyval.yt_tStatePredicate) = (yyvsp[0].yt_tStatePredicate); 
			break;
	case AT_TRUE: 	delete((yyvsp[0].yt_tStatePredicate) -> formula);
			(yyvsp[0].yt_tStatePredicate) -> formula = new FalsePredicate(Petrinet::InitialNet);
			(yyvsp[0].yt_tStatePredicate) -> shape = AT_FALSE;
			(yyval.yt_tStatePredicate) = (yyvsp[0].yt_tStatePredicate);
			break;
	case AT_FALSE: 	delete((yyvsp[0].yt_tStatePredicate) -> formula);
			(yyvsp[0].yt_tStatePredicate) -> formula = new TruePredicate(Petrinet::InitialNet);
			(yyvsp[0].yt_tStatePredicate) -> shape = AT_TRUE;
			(yyval.yt_tStatePredicate) = (yyvsp[0].yt_tStatePredicate);
			break;
	case AT_AND: 	(yyvsp[0].yt_tStatePredicate) -> formula -> negate();
			(yyval.yt_tStatePredicate) = (yyvsp[0].yt_tStatePredicate);
			(yyval.yt_tStatePredicate) -> shape = AT_OR;
			break;
	case AT_OR: 	(yyvsp[0].yt_tStatePredicate) -> formula -> negate();
			(yyval.yt_tStatePredicate) = (yyvsp[0].yt_tStatePredicate);
			(yyval.yt_tStatePredicate) -> shape = AT_AND;
			break;
	}
    }
#line 1635 "Frontend/Parser/ParserBuechi.cc" /* yacc.c:1652  */
    break;

  case 16:
#line 256 "Frontend/Parser/ParserBuechi.yy" /* yacc.c:1652  */
    { 
	if((yyvsp[-2].yt_tStatePredicate) -> shape == AT_TEMP || (yyvsp[0].yt_tStatePredicate) -> shape == AT_TEMP)
	{
		(yyval.yt_tStatePredicate) = Conjunction((yyvsp[-2].yt_tStatePredicate), (yyvsp[0].yt_tStatePredicate)); 
		(yyval.yt_tStatePredicate) -> shape = AT_TEMP;
	}
	else if((yyvsp[-2].yt_tStatePredicate) -> shape == AT_FALSE || (yyvsp[0].yt_tStatePredicate) -> shape == AT_FALSE)
	{
		delete (yyvsp[-2].yt_tStatePredicate) -> formula;
		delete (yyvsp[0].yt_tStatePredicate) -> formula;
		(yyval.yt_tStatePredicate) = (yyvsp[-2].yt_tStatePredicate);
		(yyval.yt_tStatePredicate) -> formula = new FalsePredicate(Petrinet::InitialNet);
		(yyval.yt_tStatePredicate) -> shape = AT_FALSE;
	}
	else if((yyvsp[-2].yt_tStatePredicate) -> shape == AT_TRUE)
	{
		(yyval.yt_tStatePredicate) = (yyvsp[0].yt_tStatePredicate);
		(yyval.yt_tStatePredicate) -> shape = (yyvsp[0].yt_tStatePredicate) -> shape;
	}
	else if((yyvsp[0].yt_tStatePredicate) -> shape == AT_TRUE)
	{
		(yyval.yt_tStatePredicate) = (yyvsp[-2].yt_tStatePredicate);
		(yyval.yt_tStatePredicate) -> shape = (yyvsp[-2].yt_tStatePredicate) -> shape;
	} 
	else if(((yyvsp[-2].yt_tStatePredicate)->shape == AT_AND) && (((yyvsp[0].yt_tStatePredicate) -> shape == AT_OR) || ((yyvsp[0].yt_tStatePredicate) -> shape == AT_FIR) || ((yyvsp[0].yt_tStatePredicate) -> shape == AT_COMP) || ((yyvsp[0].yt_tStatePredicate)->shape == AT_DL)))
	{
		reinterpret_cast<AtomicBooleanPredicate *>((yyvsp[-2].yt_tStatePredicate) -> formula) -> addSub((yyvsp[0].yt_tStatePredicate)->formula);
		(yyval.yt_tStatePredicate) = (yyvsp[-2].yt_tStatePredicate);
	}
	else if(((yyvsp[0].yt_tStatePredicate)->shape == AT_AND) && (((yyvsp[-2].yt_tStatePredicate) -> shape == AT_OR) || ((yyvsp[-2].yt_tStatePredicate) -> shape == AT_COMP) || ((yyvsp[-2].yt_tStatePredicate) -> shape == AT_DL) || ((yyvsp[-2].yt_tStatePredicate) -> shape == AT_FIR)))
	{
		reinterpret_cast<AtomicBooleanPredicate *>((yyvsp[0].yt_tStatePredicate) -> formula) -> addSub((yyvsp[-2].yt_tStatePredicate)->formula);
		(yyval.yt_tStatePredicate) = (yyvsp[0].yt_tStatePredicate);
	}
	else if(((yyvsp[-2].yt_tStatePredicate)->shape == AT_AND) && ((yyvsp[0].yt_tStatePredicate) -> shape == AT_AND))
	{
		reinterpret_cast<AtomicBooleanPredicate *>((yyvsp[0].yt_tStatePredicate) -> formula)->merge(reinterpret_cast<AtomicBooleanPredicate *>((yyvsp[-2].yt_tStatePredicate)->formula));
		(yyval.yt_tStatePredicate) = (yyvsp[0].yt_tStatePredicate);
	}
	else // both $1 and $3 are AT_FIR, AT_COMP, AT_DL or AT_OR
	{
		AtomicBooleanPredicate * result = new AtomicBooleanPredicate(Petrinet::InitialNet,true);
		result -> addSub((yyvsp[-2].yt_tStatePredicate)->formula);
		result -> addSub((yyvsp[0].yt_tStatePredicate)->formula);
		(yyval.yt_tStatePredicate) = (yyvsp[-2].yt_tStatePredicate);
		(yyval.yt_tStatePredicate) -> formula = result;
		(yyval.yt_tStatePredicate) -> shape = AT_AND;
	}
    }
#line 1689 "Frontend/Parser/ParserBuechi.cc" /* yacc.c:1652  */
    break;

  case 17:
#line 306 "Frontend/Parser/ParserBuechi.yy" /* yacc.c:1652  */
    { 
	if((yyvsp[-2].yt_tStatePredicate) -> shape == AT_TEMP || (yyvsp[0].yt_tStatePredicate) -> shape == AT_TEMP)
	{
		(yyval.yt_tStatePredicate) = Disjunction((yyvsp[-2].yt_tStatePredicate), (yyvsp[0].yt_tStatePredicate)); 
		(yyval.yt_tStatePredicate) -> shape = AT_TEMP;
	}
	else if((yyvsp[-2].yt_tStatePredicate) -> shape == AT_TRUE || (yyvsp[0].yt_tStatePredicate) -> shape == AT_TRUE)
	{
		delete (yyvsp[-2].yt_tStatePredicate) -> formula;
		delete (yyvsp[0].yt_tStatePredicate) -> formula;
		(yyval.yt_tStatePredicate) = (yyvsp[-2].yt_tStatePredicate);
		(yyval.yt_tStatePredicate) -> formula = new TruePredicate(Petrinet::InitialNet);
		(yyval.yt_tStatePredicate) -> shape = AT_TRUE;
	}
	else if((yyvsp[-2].yt_tStatePredicate) -> shape == AT_FALSE)
	{
		(yyval.yt_tStatePredicate) = (yyvsp[0].yt_tStatePredicate);
		(yyval.yt_tStatePredicate) -> shape = (yyvsp[0].yt_tStatePredicate) -> shape;
	}
	else if((yyvsp[0].yt_tStatePredicate) -> shape == AT_FALSE)
	{
		(yyval.yt_tStatePredicate) = (yyvsp[-2].yt_tStatePredicate);
		(yyval.yt_tStatePredicate) -> shape = (yyvsp[-2].yt_tStatePredicate) -> shape;
	} 
	else if(((yyvsp[-2].yt_tStatePredicate)->shape == AT_OR) && (((yyvsp[0].yt_tStatePredicate) -> shape == AT_AND) || ((yyvsp[0].yt_tStatePredicate) -> shape == AT_COMP) || ((yyvsp[0].yt_tStatePredicate) -> shape == AT_FIR) || ((yyvsp[0].yt_tStatePredicate)->shape == AT_DL) ))
	{
		reinterpret_cast<AtomicBooleanPredicate *>((yyvsp[-2].yt_tStatePredicate) -> formula) -> addSub((yyvsp[0].yt_tStatePredicate)->formula);
		
		(yyval.yt_tStatePredicate) = (yyvsp[-2].yt_tStatePredicate);
	}
	else if(((yyvsp[0].yt_tStatePredicate)->shape == AT_OR) && (((yyvsp[-2].yt_tStatePredicate) -> shape == AT_AND) || ((yyvsp[-2].yt_tStatePredicate) -> shape == AT_COMP) || ((yyvsp[-2].yt_tStatePredicate)->shape == AT_FIR) || ((yyvsp[-2].yt_tStatePredicate)->shape == AT_DL)))
	{
		reinterpret_cast<AtomicBooleanPredicate *>((yyvsp[0].yt_tStatePredicate) -> formula) -> addSub((yyvsp[-2].yt_tStatePredicate)->formula);
		(yyval.yt_tStatePredicate) = (yyvsp[0].yt_tStatePredicate);
	}
	else if(((yyvsp[-2].yt_tStatePredicate)->shape == AT_OR) && ((yyvsp[0].yt_tStatePredicate) -> shape == AT_OR))
	{
		reinterpret_cast<AtomicBooleanPredicate *>((yyvsp[0].yt_tStatePredicate) -> formula)->merge(reinterpret_cast<AtomicBooleanPredicate *>((yyvsp[-2].yt_tStatePredicate)->formula));
		(yyval.yt_tStatePredicate) = (yyvsp[0].yt_tStatePredicate);
	}
	else // both $1 and $3 are AT_COMP,AT_FIR,AT_DL or AT_OR
	{
		AtomicBooleanPredicate * result = new AtomicBooleanPredicate(Petrinet::InitialNet,false);
		result -> addSub((yyvsp[-2].yt_tStatePredicate)->formula);
		result -> addSub((yyvsp[0].yt_tStatePredicate)->formula);
		(yyval.yt_tStatePredicate) = (yyvsp[-2].yt_tStatePredicate);
		(yyval.yt_tStatePredicate) -> formula = result;
		(yyval.yt_tStatePredicate) -> shape = AT_OR;
	}
    }
#line 1744 "Frontend/Parser/ParserBuechi.cc" /* yacc.c:1652  */
    break;

  case 18:
#line 357 "Frontend/Parser/ParserBuechi.yy" /* yacc.c:1652  */
    { 
	// translate into (p & -q) | (-p & q)

	if((yyvsp[-2].yt_tStatePredicate) -> shape == AT_TEMP || (yyvsp[0].yt_tStatePredicate) -> shape == AT_TEMP)
	{
		(yyval.yt_tStatePredicate) = ExclusiveDisjunction((yyvsp[-2].yt_tStatePredicate), (yyvsp[0].yt_tStatePredicate)); 
		(yyval.yt_tStatePredicate) -> shape = AT_TEMP;
	}
	else if((yyvsp[-2].yt_tStatePredicate) -> shape == AT_TRUE)
	{
		(yyvsp[0].yt_tStatePredicate) -> formula = (yyvsp[0].yt_tStatePredicate) -> formula -> negate();
		(yyval.yt_tStatePredicate) = (yyvsp[0].yt_tStatePredicate);
		(yyval.yt_tStatePredicate) -> shape = ooppShape((yyvsp[0].yt_tStatePredicate)->shape);
	}
	else if((yyvsp[0].yt_tStatePredicate) -> shape == AT_TRUE)
	{
		(yyvsp[-2].yt_tStatePredicate) -> formula = (yyvsp[-2].yt_tStatePredicate) -> formula -> negate();
		(yyval.yt_tStatePredicate) = (yyvsp[-2].yt_tStatePredicate);
		(yyval.yt_tStatePredicate) -> shape = ooppShape((yyvsp[-2].yt_tStatePredicate)->shape);
	}
	else if((yyvsp[0].yt_tStatePredicate) -> shape == AT_FALSE)
	{
		(yyval.yt_tStatePredicate) = (yyvsp[-2].yt_tStatePredicate);
	}
	else if((yyvsp[-2].yt_tStatePredicate) -> shape == AT_FALSE)
	{
		(yyval.yt_tStatePredicate) = (yyvsp[0].yt_tStatePredicate);
	}
	else if(((yyvsp[-2].yt_tStatePredicate) -> shape == AT_AND) && (((yyvsp[0].yt_tStatePredicate) -> shape == AT_COMP)|| ((yyvsp[0].yt_tStatePredicate)->shape == AT_FIR) || ((yyvsp[0].yt_tStatePredicate)->shape == AT_DL)))
	{
		StatePredicate * b = (yyvsp[0].yt_tStatePredicate) -> formula -> copy(NULL);
		b = b -> negate();  //  -q, AT_*
		StatePredicate * c = (yyvsp[-2].yt_tStatePredicate)->formula -> copy(NULL);
		c = c -> negate(); // -p, AT_OR
		AtomicBooleanPredicate * d = new AtomicBooleanPredicate(Petrinet::InitialNet,true);
		d -> addSub(c);
		d -> addSub((yyvsp[0].yt_tStatePredicate)->formula); // d = (-p & q)
		reinterpret_cast<AtomicBooleanPredicate *>((yyvsp[-2].yt_tStatePredicate) -> formula) ->addSub(b);     // $1 = (p & -q)
		AtomicBooleanPredicate * a = new AtomicBooleanPredicate(Petrinet::InitialNet,false);
		a -> addSub((yyvsp[-2].yt_tStatePredicate)->formula);
		a -> addSub(d); // a = result
		(yyval.yt_tStatePredicate) = (yyvsp[-2].yt_tStatePredicate);
		(yyval.yt_tStatePredicate) -> formula = a;
		(yyval.yt_tStatePredicate) -> shape = AT_OR;
	}
	else if((((yyvsp[-2].yt_tStatePredicate) -> shape == AT_COMP)||((yyvsp[-2].yt_tStatePredicate)->shape == AT_FIR) || ((yyvsp[-2].yt_tStatePredicate)->shape == AT_DL)) && ((yyvsp[0].yt_tStatePredicate) -> shape == AT_AND))
	{
		StatePredicate * b = (yyvsp[-2].yt_tStatePredicate) -> formula -> copy(NULL);
		b = b -> negate();  //  -p, AT_ELEM
		StatePredicate * c = (yyvsp[0].yt_tStatePredicate)->formula -> copy(NULL);
		c = c -> negate(); // -q, AT_OR
		AtomicBooleanPredicate * d = new AtomicBooleanPredicate(Petrinet::InitialNet,true);
		d -> addSub(c);
		d -> addSub((yyvsp[-2].yt_tStatePredicate)->formula); // d = (p & -q)
		reinterpret_cast<AtomicBooleanPredicate *>((yyvsp[0].yt_tStatePredicate) -> formula) ->addSub(b);     // $3 = (-p & q)
		AtomicBooleanPredicate * a = new AtomicBooleanPredicate(Petrinet::InitialNet,false);
		a -> addSub(d);
		a -> addSub((yyvsp[0].yt_tStatePredicate)->formula); // a = result
		(yyval.yt_tStatePredicate) = (yyvsp[-2].yt_tStatePredicate);
		(yyval.yt_tStatePredicate) -> formula = a;
		(yyval.yt_tStatePredicate) -> shape = AT_OR;
	}
	else if((((yyvsp[-2].yt_tStatePredicate) -> shape == AT_COMP)||((yyvsp[-2].yt_tStatePredicate)->shape == AT_FIR) || ((yyvsp[-2].yt_tStatePredicate)->shape == AT_DL)) && ((yyvsp[0].yt_tStatePredicate) -> shape == AT_OR))
	{
		StatePredicate * b = (yyvsp[-2].yt_tStatePredicate) -> formula -> copy(NULL);
		b = b -> negate();  //  -p, AT_ELEM
		StatePredicate * c = (yyvsp[0].yt_tStatePredicate)->formula -> copy(NULL);
		c = c -> negate(); // -q, AT_AND
		reinterpret_cast<AtomicBooleanPredicate *>(c) -> addSub((yyvsp[-2].yt_tStatePredicate) -> formula); // c = p & -q
		AtomicBooleanPredicate * d = new AtomicBooleanPredicate(Petrinet::InitialNet,true);
		d -> addSub(b);
		d -> addSub((yyvsp[0].yt_tStatePredicate)->formula); // d = (-p & q)
		AtomicBooleanPredicate * a = new AtomicBooleanPredicate(Petrinet::InitialNet,false);
		a -> addSub(d);
		a -> addSub(c); // a = result
		(yyval.yt_tStatePredicate) = (yyvsp[-2].yt_tStatePredicate);
		(yyval.yt_tStatePredicate) -> formula = a;
		(yyval.yt_tStatePredicate) -> shape = AT_OR;
	}
	else if(((yyvsp[-2].yt_tStatePredicate) -> shape == AT_OR) && (((yyvsp[0].yt_tStatePredicate) -> shape == AT_COMP)||((yyvsp[0].yt_tStatePredicate)->shape == AT_FIR) || ((yyvsp[0].yt_tStatePredicate)->shape == AT_DL)))
	{
		StatePredicate * b = (yyvsp[0].yt_tStatePredicate) -> formula -> copy(NULL);
		b = b -> negate();  //  -q, AT_ELEM
		StatePredicate * c = (yyvsp[-2].yt_tStatePredicate)->formula -> copy(NULL);
		c = c -> negate(); // -p, AT_AND
		reinterpret_cast<AtomicBooleanPredicate *>(c) -> addSub((yyvsp[0].yt_tStatePredicate) -> formula); // c = -p & q
		AtomicBooleanPredicate * d = new AtomicBooleanPredicate(Petrinet::InitialNet,true);
		d -> addSub(b);
		d -> addSub((yyvsp[-2].yt_tStatePredicate)->formula); // d = (p & -q)
		AtomicBooleanPredicate * a = new AtomicBooleanPredicate(Petrinet::InitialNet,false);
		a -> addSub(d);
		a -> addSub(c); // a = result
		(yyval.yt_tStatePredicate) = (yyvsp[-2].yt_tStatePredicate);
		(yyval.yt_tStatePredicate) -> formula = a;
		(yyval.yt_tStatePredicate) -> shape = AT_OR;
	}
	else if((((yyvsp[-2].yt_tStatePredicate) -> shape == AT_COMP)||((yyvsp[-2].yt_tStatePredicate)->shape == AT_FIR) || ((yyvsp[-2].yt_tStatePredicate)->shape == AT_DL)) && (((yyvsp[0].yt_tStatePredicate) -> shape == AT_COMP)|| ((yyvsp[0].yt_tStatePredicate)->shape == AT_FIR)||((yyvsp[0].yt_tStatePredicate)->shape==AT_DL)))
	{
		StatePredicate * b = (yyvsp[0].yt_tStatePredicate) -> formula -> copy(NULL);
		b = b -> negate();  //  -q, AT_ELEM
		StatePredicate * c = (yyvsp[-2].yt_tStatePredicate)->formula -> copy(NULL);
		c = c -> negate(); // -p, AT_AND
		AtomicBooleanPredicate * d = new AtomicBooleanPredicate(Petrinet::InitialNet,true);
		d -> addSub(b);
		d -> addSub((yyvsp[-2].yt_tStatePredicate)->formula); // d = (p & -q)
		AtomicBooleanPredicate * e = new AtomicBooleanPredicate(Petrinet::InitialNet,true);
		e -> addSub(c);
		e -> addSub((yyvsp[0].yt_tStatePredicate)->formula); // d = (p & -q)
		AtomicBooleanPredicate * a = new AtomicBooleanPredicate(Petrinet::InitialNet,false);
		a -> addSub(d);
		a -> addSub(e); // a = result
		(yyval.yt_tStatePredicate) = (yyvsp[-2].yt_tStatePredicate);
		(yyval.yt_tStatePredicate) -> formula = a;
		(yyval.yt_tStatePredicate) -> shape = AT_OR;
	}
	else if(((yyvsp[-2].yt_tStatePredicate) -> shape == AT_AND) && ((yyvsp[0].yt_tStatePredicate) -> shape == AT_AND))
	{
		StatePredicate * b = (yyvsp[0].yt_tStatePredicate) -> formula -> copy(NULL);
		b = b -> negate();  //  -q, AT_OR
		StatePredicate * c = (yyvsp[-2].yt_tStatePredicate)->formula -> copy(NULL);
		c = c -> negate(); // -p, AT_OR
		AtomicBooleanPredicate * d = new AtomicBooleanPredicate(Petrinet::InitialNet,true);
		d -> addSub(b);
		d -> addSub((yyvsp[-2].yt_tStatePredicate)->formula); // d = (p & -q)
		AtomicBooleanPredicate * e = new AtomicBooleanPredicate(Petrinet::InitialNet,true);
		e -> addSub(c);
		e -> addSub((yyvsp[0].yt_tStatePredicate)->formula); // d = (p & -q)
		AtomicBooleanPredicate * a = new AtomicBooleanPredicate(Petrinet::InitialNet,false);
		a -> addSub(d);
		a -> addSub(e); // a = result
		(yyval.yt_tStatePredicate) = (yyvsp[-2].yt_tStatePredicate);
		(yyval.yt_tStatePredicate) -> formula = a;
		(yyval.yt_tStatePredicate) -> shape = AT_OR;
	}
	else if(((yyvsp[-2].yt_tStatePredicate) -> shape == AT_AND) && ((yyvsp[0].yt_tStatePredicate) -> shape == AT_OR))
	{
		StatePredicate * b = (yyvsp[0].yt_tStatePredicate) -> formula -> copy(NULL);
		b = b -> negate();  //  -q, AT_AND
		StatePredicate * c = (yyvsp[-2].yt_tStatePredicate)->formula -> copy(NULL);
		c = c -> negate(); // -p, AT_OR
		reinterpret_cast<AtomicBooleanPredicate *>(b) -> addSub((yyvsp[-2].yt_tStatePredicate)->formula); // p & -q
		AtomicBooleanPredicate * e = new AtomicBooleanPredicate(Petrinet::InitialNet,true);
		e -> addSub(c);
		e -> addSub((yyvsp[0].yt_tStatePredicate)->formula); // e = (-p & q)
		AtomicBooleanPredicate * a = new AtomicBooleanPredicate(Petrinet::InitialNet,false);
		a -> addSub(b);
		a -> addSub(e); // a = result
		(yyval.yt_tStatePredicate) = (yyvsp[-2].yt_tStatePredicate);
		(yyval.yt_tStatePredicate) -> formula = a;
		(yyval.yt_tStatePredicate) -> shape = AT_OR;
	}
	else if(((yyvsp[-2].yt_tStatePredicate) -> shape == AT_OR) && ((yyvsp[0].yt_tStatePredicate) -> shape == AT_AND))
	{
		StatePredicate * b = (yyvsp[-2].yt_tStatePredicate) -> formula -> copy(NULL);
		b = b -> negate();  //  -p, AT_AND
		StatePredicate * c = (yyvsp[0].yt_tStatePredicate)->formula -> copy(NULL);
		c = c -> negate(); // -q, AT_OR
		reinterpret_cast<AtomicBooleanPredicate *>(b) -> addSub((yyvsp[0].yt_tStatePredicate)->formula); // -p & q
		AtomicBooleanPredicate * e = new AtomicBooleanPredicate(Petrinet::InitialNet,true);
		e -> addSub(c);
		e -> addSub((yyvsp[-2].yt_tStatePredicate)->formula); // e = (p & -q)
		AtomicBooleanPredicate * a = new AtomicBooleanPredicate(Petrinet::InitialNet,false);
		a -> addSub(b);
		a -> addSub(e); // a = result
		(yyval.yt_tStatePredicate) = (yyvsp[-2].yt_tStatePredicate);
		(yyval.yt_tStatePredicate) -> formula = a;
		(yyval.yt_tStatePredicate) -> shape = AT_OR;
	}
	else // (($1 -> shape == AT_OR) && ($3 -> shape == AT_OR))
	{
		StatePredicate * b = (yyvsp[-2].yt_tStatePredicate) -> formula -> copy(NULL);
		b = b -> negate();  //  -p, AT_AND
		StatePredicate * c = (yyvsp[0].yt_tStatePredicate)->formula -> copy(NULL);
		c = c -> negate(); // -q, AT_AND
		reinterpret_cast<AtomicBooleanPredicate *>(b) -> addSub((yyvsp[0].yt_tStatePredicate)->formula); // -p & q
		reinterpret_cast<AtomicBooleanPredicate *>(c) -> addSub((yyvsp[-2].yt_tStatePredicate)->formula); // p & -q
		AtomicBooleanPredicate * a = new AtomicBooleanPredicate(Petrinet::InitialNet,false);
		a -> addSub(b);
		a -> addSub(c); // a = result
		(yyval.yt_tStatePredicate) = (yyvsp[-2].yt_tStatePredicate);
		(yyval.yt_tStatePredicate) -> formula = a;
		(yyval.yt_tStatePredicate) -> shape = AT_OR;
	}
    }
#line 1933 "Frontend/Parser/ParserBuechi.cc" /* yacc.c:1652  */
    break;

  case 19:
#line 542 "Frontend/Parser/ParserBuechi.yy" /* yacc.c:1652  */
    { 
	if((yyvsp[-2].yt_tStatePredicate) -> shape == AT_TEMP || (yyvsp[0].yt_tStatePredicate) -> shape == AT_TEMP)
	{
		(yyval.yt_tStatePredicate) = Implication((yyvsp[-2].yt_tStatePredicate), (yyvsp[0].yt_tStatePredicate)); 
		(yyval.yt_tStatePredicate) -> shape = AT_TEMP;
	}
	else if((yyvsp[-2].yt_tStatePredicate) -> shape == AT_FALSE || (yyvsp[0].yt_tStatePredicate) -> shape == AT_TRUE)
	{
		delete (yyvsp[-2].yt_tStatePredicate) -> formula;
		delete (yyvsp[0].yt_tStatePredicate) -> formula;
		(yyval.yt_tStatePredicate) = (yyvsp[-2].yt_tStatePredicate);
		(yyval.yt_tStatePredicate) -> formula = new TruePredicate(Petrinet::InitialNet);
		(yyval.yt_tStatePredicate) -> shape = AT_TRUE;
	}
	else if((yyvsp[-2].yt_tStatePredicate) -> shape == AT_TRUE)
	{
		(yyval.yt_tStatePredicate) = (yyvsp[0].yt_tStatePredicate);
		(yyval.yt_tStatePredicate) -> shape = (yyvsp[0].yt_tStatePredicate) -> shape;
	}
	else if((yyvsp[0].yt_tStatePredicate) -> shape == AT_FALSE)
	{
		(yyvsp[-2].yt_tStatePredicate)->formula = (yyvsp[-2].yt_tStatePredicate) -> formula -> negate();
		(yyval.yt_tStatePredicate) = (yyvsp[-2].yt_tStatePredicate);
		(yyval.yt_tStatePredicate) -> shape = ooppShape((yyvsp[-2].yt_tStatePredicate) -> shape);
	} 
	else if(((yyvsp[-2].yt_tStatePredicate)->shape == AT_AND) && (((yyvsp[0].yt_tStatePredicate) -> shape == AT_AND) || ((yyvsp[0].yt_tStatePredicate) -> shape == AT_COMP)|| ((yyvsp[0].yt_tStatePredicate)->shape == AT_FIR) || ((yyvsp[0].yt_tStatePredicate)->shape == AT_DL)))
	{
		 (yyvsp[-2].yt_tStatePredicate) -> formula = (yyvsp[-2].yt_tStatePredicate) -> formula ->negate();
		reinterpret_cast<AtomicBooleanPredicate *>((yyvsp[-2].yt_tStatePredicate) -> formula) -> addSub((yyvsp[0].yt_tStatePredicate)->formula);
		(yyval.yt_tStatePredicate) = (yyvsp[-2].yt_tStatePredicate);
		(yyval.yt_tStatePredicate) -> shape = AT_OR;
	}
	else if(((yyvsp[0].yt_tStatePredicate)->shape == AT_OR) && (((yyvsp[-2].yt_tStatePredicate) -> shape == AT_AND) || ((yyvsp[-2].yt_tStatePredicate) -> shape == AT_COMP)||((yyvsp[-2].yt_tStatePredicate)->shape == AT_FIR) || ((yyvsp[-2].yt_tStatePredicate)->shape == AT_DL)))
	{
		(yyvsp[-2].yt_tStatePredicate) -> formula = (yyvsp[-2].yt_tStatePredicate)-> formula -> negate();
		reinterpret_cast<AtomicBooleanPredicate *>((yyvsp[0].yt_tStatePredicate) -> formula) -> addSub((yyvsp[-2].yt_tStatePredicate)->formula);
		(yyval.yt_tStatePredicate) = (yyvsp[0].yt_tStatePredicate);
	}
	else if(((yyvsp[-2].yt_tStatePredicate)->shape == AT_AND) && ((yyvsp[0].yt_tStatePredicate) -> shape == AT_OR))
	{
		(yyvsp[-2].yt_tStatePredicate) -> formula = (yyvsp[-2].yt_tStatePredicate) -> formula -> negate();
		reinterpret_cast<AtomicBooleanPredicate *>((yyvsp[0].yt_tStatePredicate) -> formula)->merge(reinterpret_cast<AtomicBooleanPredicate *>((yyvsp[-2].yt_tStatePredicate)->formula));
		(yyval.yt_tStatePredicate) = (yyvsp[0].yt_tStatePredicate);
	}
	else // both $1 and $3 are AT_* or AT_OR
	{
		(yyvsp[-2].yt_tStatePredicate) -> formula = (yyvsp[-2].yt_tStatePredicate) -> formula -> negate();
		AtomicBooleanPredicate * result = new AtomicBooleanPredicate(Petrinet::InitialNet,false);
		result -> addSub((yyvsp[-2].yt_tStatePredicate)->formula);
		result -> addSub((yyvsp[0].yt_tStatePredicate)->formula);
		(yyval.yt_tStatePredicate) = (yyvsp[-2].yt_tStatePredicate);
		(yyval.yt_tStatePredicate) -> formula = result;
		(yyval.yt_tStatePredicate) -> shape = AT_OR;
	}
    }
#line 1993 "Frontend/Parser/ParserBuechi.cc" /* yacc.c:1652  */
    break;

  case 20:
#line 598 "Frontend/Parser/ParserBuechi.yy" /* yacc.c:1652  */
    { 
	// translate into (p & q) | (-p & -q)

	if((yyvsp[-2].yt_tStatePredicate) -> shape == AT_TEMP || (yyvsp[0].yt_tStatePredicate) -> shape == AT_TEMP)
	{
		(yyval.yt_tStatePredicate) = Equivalence((yyvsp[-2].yt_tStatePredicate), (yyvsp[0].yt_tStatePredicate)); 
		(yyval.yt_tStatePredicate) -> shape = AT_TEMP;
	}
	else if((yyvsp[-2].yt_tStatePredicate) -> shape == AT_FALSE)
	{
		(yyvsp[0].yt_tStatePredicate) -> formula = (yyvsp[0].yt_tStatePredicate) -> formula -> negate();
		(yyval.yt_tStatePredicate) = (yyvsp[0].yt_tStatePredicate);
		(yyval.yt_tStatePredicate) -> shape = ooppShape((yyvsp[0].yt_tStatePredicate)->shape);
	}
	else if((yyvsp[0].yt_tStatePredicate) -> shape == AT_FALSE)
	{
		(yyvsp[-2].yt_tStatePredicate) -> formula = (yyvsp[-2].yt_tStatePredicate) -> formula -> negate();
		(yyval.yt_tStatePredicate) = (yyvsp[-2].yt_tStatePredicate);
		(yyval.yt_tStatePredicate) -> shape = ooppShape((yyvsp[-2].yt_tStatePredicate)->shape);
	}
	else if((yyvsp[0].yt_tStatePredicate) -> shape == AT_TRUE)
	{
		(yyval.yt_tStatePredicate) = (yyvsp[-2].yt_tStatePredicate);
	}
	else if((yyvsp[-2].yt_tStatePredicate) -> shape == AT_TRUE)
	{
		(yyval.yt_tStatePredicate) = (yyvsp[0].yt_tStatePredicate);
	}
	else if(((yyvsp[-2].yt_tStatePredicate) -> shape == AT_AND) && (((yyvsp[0].yt_tStatePredicate) -> shape == AT_COMP)||((yyvsp[0].yt_tStatePredicate)->shape == AT_FIR) || ((yyvsp[0].yt_tStatePredicate)->shape == AT_DL)))
	{
		StatePredicate * b = (yyvsp[0].yt_tStatePredicate) -> formula -> copy(NULL);
		b = b -> negate();  //  -q, AT_ELEM
		StatePredicate * c = (yyvsp[-2].yt_tStatePredicate)->formula -> copy(NULL);
		c = c -> negate(); // -p, AT_OR
		AtomicBooleanPredicate * d = new AtomicBooleanPredicate(Petrinet::InitialNet,true);
		d -> addSub(c);
		d -> addSub(b); // d = (-p & -q)
		reinterpret_cast<AtomicBooleanPredicate *>((yyvsp[-2].yt_tStatePredicate) -> formula) ->addSub((yyvsp[0].yt_tStatePredicate)->formula);     // $1 = (p & q)
		AtomicBooleanPredicate * a = new AtomicBooleanPredicate(Petrinet::InitialNet,false);
		a -> addSub((yyvsp[-2].yt_tStatePredicate)->formula);
		a -> addSub(d); // a = result
		(yyval.yt_tStatePredicate) = (yyvsp[-2].yt_tStatePredicate);
		(yyval.yt_tStatePredicate) -> formula = a;
		(yyval.yt_tStatePredicate) -> shape = AT_OR;
	}
	else if((((yyvsp[-2].yt_tStatePredicate) -> shape == AT_COMP)|| ((yyvsp[-2].yt_tStatePredicate)->shape == AT_FIR) || ((yyvsp[-2].yt_tStatePredicate)->shape == AT_DL)) && ((yyvsp[0].yt_tStatePredicate) -> shape == AT_AND))
	{
		StatePredicate * b = (yyvsp[-2].yt_tStatePredicate) -> formula -> copy(NULL);
		b = b -> negate();  //  -p, AT_ELEM
		StatePredicate * c = (yyvsp[0].yt_tStatePredicate)->formula -> copy(NULL);
		c = c -> negate(); // -q, AT_OR
		AtomicBooleanPredicate * d = new AtomicBooleanPredicate(Petrinet::InitialNet,true);
		d -> addSub(c);
		d -> addSub(b); // d = (-p & -q)
		reinterpret_cast<AtomicBooleanPredicate *>((yyvsp[0].yt_tStatePredicate) -> formula) ->addSub((yyvsp[-2].yt_tStatePredicate)->formula);     // $3 = (p & q)
		AtomicBooleanPredicate * a = new AtomicBooleanPredicate(Petrinet::InitialNet,false);
		a -> addSub(d);
		a -> addSub((yyvsp[0].yt_tStatePredicate)->formula); // a = result
		(yyval.yt_tStatePredicate) = (yyvsp[-2].yt_tStatePredicate);
		(yyval.yt_tStatePredicate) -> formula = a;
		(yyval.yt_tStatePredicate) -> shape = AT_OR;
	}
	else if((((yyvsp[-2].yt_tStatePredicate) -> shape == AT_COMP)||((yyvsp[-2].yt_tStatePredicate)->shape == AT_FIR) || ((yyvsp[-2].yt_tStatePredicate)->shape == AT_DL)) && ((yyvsp[0].yt_tStatePredicate) -> shape == AT_OR))
	{
		StatePredicate * b = (yyvsp[-2].yt_tStatePredicate) -> formula -> copy(NULL);
		b = b -> negate();  //  -p, AT_ELEM
		StatePredicate * c = (yyvsp[0].yt_tStatePredicate)->formula -> copy(NULL);
		c = c -> negate(); // -q, AT_AND
		reinterpret_cast<AtomicBooleanPredicate *>(c) -> addSub(b); // c = -p & -q
		AtomicBooleanPredicate * d = new AtomicBooleanPredicate(Petrinet::InitialNet,true);
		d -> addSub((yyvsp[-2].yt_tStatePredicate)->formula);
		d -> addSub((yyvsp[0].yt_tStatePredicate)->formula); // d = (p & q)
		AtomicBooleanPredicate * a = new AtomicBooleanPredicate(Petrinet::InitialNet,false);
		a -> addSub(d);
		a -> addSub(c); // a = result
		(yyval.yt_tStatePredicate) = (yyvsp[-2].yt_tStatePredicate);
		(yyval.yt_tStatePredicate) -> formula = a;
		(yyval.yt_tStatePredicate) -> shape = AT_OR;
	}
	else if(((yyvsp[-2].yt_tStatePredicate) -> shape == AT_OR) && (((yyvsp[0].yt_tStatePredicate) -> shape == AT_COMP)||((yyvsp[0].yt_tStatePredicate)->shape==AT_FIR) || ((yyvsp[0].yt_tStatePredicate)->shape == AT_DL)))
	{
		StatePredicate * b = (yyvsp[0].yt_tStatePredicate) -> formula -> copy(NULL);
		b = b -> negate();  //  -q, AT_ELEM
		StatePredicate * c = (yyvsp[-2].yt_tStatePredicate)->formula -> copy(NULL);
		c = c -> negate(); // -p, AT_AND
		reinterpret_cast<AtomicBooleanPredicate *>(c) -> addSub(b); // c = -p & -q
		AtomicBooleanPredicate * d = new AtomicBooleanPredicate(Petrinet::InitialNet,true);
		d -> addSub((yyvsp[0].yt_tStatePredicate)->formula);
		d -> addSub((yyvsp[-2].yt_tStatePredicate)->formula); // d = (p & q)
		AtomicBooleanPredicate * a = new AtomicBooleanPredicate(Petrinet::InitialNet,false);
		a -> addSub(d);
		a -> addSub(c); // a = result
		(yyval.yt_tStatePredicate) = (yyvsp[-2].yt_tStatePredicate);
		(yyval.yt_tStatePredicate) -> formula = a;
		(yyval.yt_tStatePredicate) -> shape = AT_OR;
	}
	else if((((yyvsp[-2].yt_tStatePredicate) -> shape == AT_COMP)|| ((yyvsp[-2].yt_tStatePredicate)->shape == AT_FIR) || ((yyvsp[-2].yt_tStatePredicate)->shape == AT_DL)) && (((yyvsp[0].yt_tStatePredicate) -> shape == AT_COMP)||((yyvsp[0].yt_tStatePredicate)->shape == AT_FIR)||((yyvsp[0].yt_tStatePredicate)->shape == AT_DL)))
	{
		StatePredicate * b = (yyvsp[0].yt_tStatePredicate) -> formula -> copy(NULL);
		b = b -> negate();  //  -q, AT_ELEM
		StatePredicate * c = (yyvsp[-2].yt_tStatePredicate)->formula -> copy(NULL);
		c = c -> negate(); // -p, AT_AND
		AtomicBooleanPredicate * d = new AtomicBooleanPredicate(Petrinet::InitialNet,true);
		d -> addSub(b);
		d -> addSub(c); // d = (-p & -q)
		AtomicBooleanPredicate * e = new AtomicBooleanPredicate(Petrinet::InitialNet,true);
		e -> addSub((yyvsp[-2].yt_tStatePredicate)->formula);
		e -> addSub((yyvsp[0].yt_tStatePredicate)->formula); // e = (p & q)
		AtomicBooleanPredicate * a = new AtomicBooleanPredicate(Petrinet::InitialNet,false);
		a -> addSub(d);
		a -> addSub(e); // a = result
		(yyval.yt_tStatePredicate) = (yyvsp[-2].yt_tStatePredicate);
		(yyval.yt_tStatePredicate) -> formula = a;
		(yyval.yt_tStatePredicate) -> shape = AT_OR;
	}
	else if(((yyvsp[-2].yt_tStatePredicate) -> shape == AT_AND) && ((yyvsp[0].yt_tStatePredicate) -> shape == AT_AND))
	{
		StatePredicate * b = (yyvsp[0].yt_tStatePredicate) -> formula -> copy(NULL);
		b = b -> negate();  //  -q, AT_OR
		StatePredicate * c = (yyvsp[-2].yt_tStatePredicate)->formula -> copy(NULL);
		c = c -> negate(); // -p, AT_OR
		AtomicBooleanPredicate * d = new AtomicBooleanPredicate(Petrinet::InitialNet,true);
		d -> addSub(b);
		d -> addSub(c); // d = (-p & -q)
		reinterpret_cast<AtomicBooleanPredicate *>((yyvsp[-2].yt_tStatePredicate)->formula)->merge(reinterpret_cast<AtomicBooleanPredicate *>((yyvsp[0].yt_tStatePredicate)->formula)); //$1 = p&q
		AtomicBooleanPredicate * a = new AtomicBooleanPredicate(Petrinet::InitialNet,false);
		a -> addSub(d);
		a -> addSub((yyvsp[-2].yt_tStatePredicate)->formula); // a = result
		(yyval.yt_tStatePredicate) = (yyvsp[-2].yt_tStatePredicate);
		(yyval.yt_tStatePredicate) -> formula = a;
		(yyval.yt_tStatePredicate) -> shape = AT_OR;
	}
	else if(((yyvsp[-2].yt_tStatePredicate) -> shape == AT_AND) && ((yyvsp[0].yt_tStatePredicate) -> shape == AT_OR))
	{
		StatePredicate * b = (yyvsp[0].yt_tStatePredicate) -> formula -> copy(NULL);
		b = b -> negate();  //  -q, AT_AND
		StatePredicate * c = (yyvsp[-2].yt_tStatePredicate)->formula -> copy(NULL);
		c = c -> negate(); // -p, AT_OR
		reinterpret_cast<AtomicBooleanPredicate *>(b) -> addSub(c); // -p & -q
		reinterpret_cast<AtomicBooleanPredicate *>((yyvsp[-2].yt_tStatePredicate)->formula)->addSub((yyvsp[0].yt_tStatePredicate)->formula); // $1 = p&q
		AtomicBooleanPredicate * a = new AtomicBooleanPredicate(Petrinet::InitialNet,false);
		a -> addSub(b);
		a -> addSub((yyvsp[-2].yt_tStatePredicate)->formula); // a = result
		(yyval.yt_tStatePredicate) = (yyvsp[-2].yt_tStatePredicate);
		(yyval.yt_tStatePredicate) -> formula = a;
		(yyval.yt_tStatePredicate) -> shape = AT_OR;
	}
	else if(((yyvsp[-2].yt_tStatePredicate) -> shape == AT_OR) && ((yyvsp[0].yt_tStatePredicate) -> shape == AT_AND))
	{
		StatePredicate * b = (yyvsp[-2].yt_tStatePredicate) -> formula -> copy(NULL);
		b = b -> negate();  //  -p, AT_AND
		StatePredicate * c = (yyvsp[0].yt_tStatePredicate)->formula -> copy(NULL);
		c = c -> negate(); // -q, AT_OR
		reinterpret_cast<AtomicBooleanPredicate *>(b) -> addSub(c); // -p & -q
		reinterpret_cast<AtomicBooleanPredicate *>((yyvsp[0].yt_tStatePredicate)->formula)->addSub((yyvsp[-2].yt_tStatePredicate)->formula); //$3 = p&q
		AtomicBooleanPredicate * a = new AtomicBooleanPredicate(Petrinet::InitialNet,false);
		a -> addSub(b);
		a -> addSub((yyvsp[0].yt_tStatePredicate)->formula); // a = result
		(yyval.yt_tStatePredicate) = (yyvsp[-2].yt_tStatePredicate);
		(yyval.yt_tStatePredicate) -> formula = a;
		(yyval.yt_tStatePredicate) -> shape = AT_OR;
	}
	else // (($1 -> shape == AT_OR) && ($3 -> shape == AT_OR))
	{
		StatePredicate * b = (yyvsp[-2].yt_tStatePredicate) -> formula -> copy(NULL);
		b = b -> negate();  //  -p, AT_AND
		StatePredicate * c = (yyvsp[0].yt_tStatePredicate)->formula -> copy(NULL);
		c = c -> negate(); // -q, AT_AND
		reinterpret_cast<AtomicBooleanPredicate *>(b) -> merge(reinterpret_cast<AtomicBooleanPredicate *>(c)); // -p & -q
		AtomicBooleanPredicate * d = new AtomicBooleanPredicate(Petrinet::InitialNet,true);
		d -> addSub((yyvsp[-2].yt_tStatePredicate)->formula);
		d -> addSub((yyvsp[0].yt_tStatePredicate)->formula); // d = p&q
		AtomicBooleanPredicate * a = new AtomicBooleanPredicate(Petrinet::InitialNet,false);
		a -> addSub(b);
		a -> addSub(d); // a = result
		(yyval.yt_tStatePredicate) = (yyvsp[-2].yt_tStatePredicate);
		(yyval.yt_tStatePredicate) -> formula = a;
		(yyval.yt_tStatePredicate) -> shape = AT_OR;
	}
    }
#line 2178 "Frontend/Parser/ParserBuechi.cc" /* yacc.c:1652  */
    break;

  case 21:
#line 782 "Frontend/Parser/ParserBuechi.yy" /* yacc.c:1652  */
    { 
	(yyvsp[0].yt_tTerm) -> ttt -> multiply(-1);
	(yyvsp[-2].yt_tTerm) -> ttt -> append((yyvsp[0].yt_tTerm) -> ttt);
	Term * T = (yyvsp[-2].yt_tTerm) -> ttt -> copy();
	T -> multiply(-1);
	AtomicBooleanPredicate * result = new AtomicBooleanPredicate(Petrinet::InitialNet,true);
	result -> addSub(new AtomicStatePredicate(Petrinet::InitialNet,(yyvsp[-2].yt_tTerm)->ttt));	
	result -> addSub(new AtomicStatePredicate(Petrinet::InitialNet,T));	
	(yyval.yt_tAtomicProposition) = Elementary();
	(yyval.yt_tAtomicProposition) -> shape = AT_AND;
	(yyval.yt_tAtomicProposition) -> pred = result;
    }
#line 2195 "Frontend/Parser/ParserBuechi.cc" /* yacc.c:1652  */
    break;

  case 22:
#line 795 "Frontend/Parser/ParserBuechi.yy" /* yacc.c:1652  */
    { 
	(yyvsp[0].yt_tTerm) -> ttt -> multiply(-1);
	(yyvsp[-2].yt_tTerm) -> ttt -> append((yyvsp[0].yt_tTerm) -> ttt);
	Term * T = (yyvsp[-2].yt_tTerm) -> ttt -> copy();
	T -> multiply(-1);
	AtomicBooleanPredicate * result = new AtomicBooleanPredicate(Petrinet::InitialNet,true);
	result -> addSub(new AtomicStatePredicate(Petrinet::InitialNet,(yyvsp[-2].yt_tTerm)->ttt));	
	result -> addSub(new AtomicStatePredicate(Petrinet::InitialNet,T));	
	result = (AtomicBooleanPredicate *) result -> negate();
	(yyval.yt_tAtomicProposition) = Elementary();
	(yyval.yt_tAtomicProposition) -> shape = AT_OR;
	(yyval.yt_tAtomicProposition) -> pred = result;
    }
#line 2213 "Frontend/Parser/ParserBuechi.cc" /* yacc.c:1652  */
    break;

  case 23:
#line 809 "Frontend/Parser/ParserBuechi.yy" /* yacc.c:1652  */
    { 
	(yyvsp[-2].yt_tTerm) -> ttt -> multiply(-1);
	(yyvsp[-2].yt_tTerm) -> ttt -> append((yyvsp[0].yt_tTerm) -> ttt);
	Term * T = new Term();
	T -> place = Petrinet::InitialNet->Card[PL];
	T -> mult = 1;
	(yyvsp[-2].yt_tTerm) -> ttt -> append(T);
	AtomicStatePredicate * result = new AtomicStatePredicate(Petrinet::InitialNet,(yyvsp[-2].yt_tTerm)->ttt);
	(yyval.yt_tAtomicProposition) = Elementary();
	(yyval.yt_tAtomicProposition) -> shape = AT_COMP;
	(yyval.yt_tAtomicProposition) -> pred = result;
	
    }
#line 2231 "Frontend/Parser/ParserBuechi.cc" /* yacc.c:1652  */
    break;

  case 24:
#line 823 "Frontend/Parser/ParserBuechi.yy" /* yacc.c:1652  */
    { 
	(yyvsp[-2].yt_tTerm) -> ttt -> multiply(-1);
	(yyvsp[-2].yt_tTerm) -> ttt -> append((yyvsp[0].yt_tTerm) -> ttt);
	AtomicStatePredicate * result = new AtomicStatePredicate(Petrinet::InitialNet,(yyvsp[-2].yt_tTerm)->ttt);
	(yyval.yt_tAtomicProposition) = Elementary();
	(yyval.yt_tAtomicProposition) -> shape = AT_COMP;
	(yyval.yt_tAtomicProposition) -> pred = result;
    }
#line 2244 "Frontend/Parser/ParserBuechi.cc" /* yacc.c:1652  */
    break;

  case 25:
#line 832 "Frontend/Parser/ParserBuechi.yy" /* yacc.c:1652  */
    { 
	(yyvsp[0].yt_tTerm) -> ttt -> multiply(-1);
	(yyvsp[-2].yt_tTerm) -> ttt -> append((yyvsp[0].yt_tTerm) -> ttt);
	Term * T = new Term();
	T -> place = Petrinet::InitialNet->Card[PL];
	T -> mult = 1;
	(yyvsp[-2].yt_tTerm) -> ttt -> append(T);
	AtomicStatePredicate * result = new AtomicStatePredicate(Petrinet::InitialNet,(yyvsp[-2].yt_tTerm)->ttt);
	(yyval.yt_tAtomicProposition) = Elementary();
	(yyval.yt_tAtomicProposition) -> shape = AT_COMP;
	(yyval.yt_tAtomicProposition) -> pred = result;
    }
#line 2261 "Frontend/Parser/ParserBuechi.cc" /* yacc.c:1652  */
    break;

  case 26:
#line 845 "Frontend/Parser/ParserBuechi.yy" /* yacc.c:1652  */
    { 
	(yyvsp[0].yt_tTerm) -> ttt -> multiply(-1);
	(yyvsp[-2].yt_tTerm) -> ttt -> append((yyvsp[0].yt_tTerm) -> ttt);
	AtomicStatePredicate * result = new AtomicStatePredicate(Petrinet::InitialNet,(yyvsp[-2].yt_tTerm)->ttt);
	(yyval.yt_tAtomicProposition) = Elementary();
	(yyval.yt_tAtomicProposition) -> shape = AT_COMP;
	(yyval.yt_tAtomicProposition) -> pred = result;
    }
#line 2274 "Frontend/Parser/ParserBuechi.cc" /* yacc.c:1652  */
    break;

  case 27:
#line 854 "Frontend/Parser/ParserBuechi.yy" /* yacc.c:1652  */
    { 
	TruePredicate * result = new TruePredicate(Petrinet::InitialNet);
	(yyval.yt_tAtomicProposition) = Elementary();
	(yyval.yt_tAtomicProposition) -> shape = AT_TRUE;
	(yyval.yt_tAtomicProposition) -> pred = result;
    }
#line 2285 "Frontend/Parser/ParserBuechi.cc" /* yacc.c:1652  */
    break;

  case 28:
#line 861 "Frontend/Parser/ParserBuechi.yy" /* yacc.c:1652  */
    { 
	FalsePredicate * result = new FalsePredicate(Petrinet::InitialNet);
	(yyval.yt_tAtomicProposition) = Elementary();
	(yyval.yt_tAtomicProposition) -> shape = AT_FALSE;
	(yyval.yt_tAtomicProposition) -> pred = result;
    }
#line 2296 "Frontend/Parser/ParserBuechi.cc" /* yacc.c:1652  */
    break;

  case 29:
#line 868 "Frontend/Parser/ParserBuechi.yy" /* yacc.c:1652  */
    {
        Symbol *t = ParserPTNet::currentsymbols->TransitionTable->lookup((yyvsp[-1].yt_casestring)->name);
        if (UNLIKELY(t == NULL))
        {
            yyerrors((yyvsp[-1].yt_casestring)->name, (yylsp[-1]), "transition '%s' unknown", (yyvsp[-1].yt_casestring)->name);
        }
	FireablePredicate * result = new FireablePredicate(Petrinet::InitialNet,t->getIndex(),true);
	(yyval.yt_tAtomicProposition) = Elementary();
	(yyval.yt_tAtomicProposition) -> shape = AT_FIR;
	(yyval.yt_tAtomicProposition) -> pred = result;
    }
#line 2312 "Frontend/Parser/ParserBuechi.cc" /* yacc.c:1652  */
    break;

  case 30:
#line 880 "Frontend/Parser/ParserBuechi.yy" /* yacc.c:1652  */
    { 
	AtomicBooleanPredicate * result = new AtomicBooleanPredicate(Petrinet::InitialNet,true);
	for(arrayindex_t i = 0; i < Petrinet::InitialNet->CardSignificant;i++)
	{
		capacity_t m0 = Petrinet::InitialNet->Initial[i];
	
		// insert p <= m0
		Term * T1 = new Term();
		T1 -> place  = i;
		T1 -> mult = 1;
		Term * T2 = new Term();
		T2 -> place = Petrinet::InitialNet->Card[PL];
		T2 -> mult = -m0;
		T1 -> append(T2);
		result -> addSub(new AtomicStatePredicate(Petrinet::InitialNet,T1));

		// insert p >= m0
		T1 = new Term();
		T1 -> place = i;
		T1 -> mult = -1;
		T2 = new Term();
		T2 -> place = Petrinet::InitialNet->Card[PL];
		T2 -> mult = m0;
		T1 -> append(T2);
		result -> addSub(new AtomicStatePredicate(Petrinet::InitialNet,T1));
	}
	(yyval.yt_tAtomicProposition) = Elementary(); 
	(yyval.yt_tAtomicProposition) -> shape = AT_AND;
	(yyval.yt_tAtomicProposition) -> pred = result;
    }
#line 2347 "Frontend/Parser/ParserBuechi.cc" /* yacc.c:1652  */
    break;

  case 31:
#line 911 "Frontend/Parser/ParserBuechi.yy" /* yacc.c:1652  */
    { 
	DeadlockPredicate * result = new DeadlockPredicate(Petrinet::InitialNet,true);
	(yyval.yt_tAtomicProposition) = Elementary(); 
	(yyval.yt_tAtomicProposition) -> shape = AT_DL;
	(yyval.yt_tAtomicProposition) -> pred = result;
    }
#line 2358 "Frontend/Parser/ParserBuechi.cc" /* yacc.c:1652  */
    break;

  case 32:
#line 921 "Frontend/Parser/ParserBuechi.yy" /* yacc.c:1652  */
    { (yyval.yt_tTerm) = (yyvsp[-1].yt_tTerm); }
#line 2364 "Frontend/Parser/ParserBuechi.cc" /* yacc.c:1652  */
    break;

  case 33:
#line 923 "Frontend/Parser/ParserBuechi.yy" /* yacc.c:1652  */
    {
        Symbol *p = ParserPTNet::currentsymbols->PlaceTable->lookup((yyvsp[0].yt_casestring)->name);
        if (UNLIKELY(p == NULL))
        {
            yyerrors((yyvsp[0].yt_casestring)->name, (yylsp[0]), "place '%s' unknown", (yyvsp[0].yt_casestring)->name);
        }
	(yyval.yt_tTerm) = Complex();
	(yyval.yt_tTerm) -> ttt = new Term();
	(yyval.yt_tTerm) -> ttt -> place = p -> getIndex();
	(yyval.yt_tTerm) -> ttt -> mult = 1;
    }
#line 2380 "Frontend/Parser/ParserBuechi.cc" /* yacc.c:1652  */
    break;

  case 34:
#line 935 "Frontend/Parser/ParserBuechi.yy" /* yacc.c:1652  */
    { 
	(yyval.yt_tTerm) = Complex(); 
	(yyval.yt_tTerm) -> ttt = new Term();
	(yyval.yt_tTerm) -> ttt -> place = Petrinet::InitialNet->Card[PL];
	(yyval.yt_tTerm) -> ttt -> mult = (yyvsp[0].yt_integer) -> value;
    }
#line 2391 "Frontend/Parser/ParserBuechi.cc" /* yacc.c:1652  */
    break;

  case 35:
#line 942 "Frontend/Parser/ParserBuechi.yy" /* yacc.c:1652  */
    { 
	(yyvsp[-2].yt_tTerm) -> ttt -> append((yyvsp[0].yt_tTerm) -> ttt);
	(yyval.yt_tTerm) = (yyvsp[-2].yt_tTerm); 
    }
#line 2400 "Frontend/Parser/ParserBuechi.cc" /* yacc.c:1652  */
    break;

  case 36:
#line 947 "Frontend/Parser/ParserBuechi.yy" /* yacc.c:1652  */
    { 
	(yyvsp[0].yt_tTerm) -> ttt -> multiply(-1);
	(yyvsp[-2].yt_tTerm) -> ttt -> append((yyvsp[0].yt_tTerm) -> ttt);
	(yyval.yt_tTerm) = (yyvsp[-2].yt_tTerm); 
    }
#line 2410 "Frontend/Parser/ParserBuechi.cc" /* yacc.c:1652  */
    break;

  case 37:
#line 953 "Frontend/Parser/ParserBuechi.yy" /* yacc.c:1652  */
    { 
	(yyvsp[0].yt_tTerm) -> ttt->multiply((yyvsp[-2].yt_integer)->value);
	(yyval.yt_tTerm) = (yyvsp[0].yt_tTerm); 
    }
#line 2419 "Frontend/Parser/ParserBuechi.cc" /* yacc.c:1652  */
    break;

  case 38:
#line 960 "Frontend/Parser/ParserBuechi.yy" /* yacc.c:1652  */
    { (yyval.yt_casestring) = (yyvsp[0].yt_casestring); }
#line 2425 "Frontend/Parser/ParserBuechi.cc" /* yacc.c:1652  */
    break;


#line 2429 "Frontend/Parser/ParserBuechi.cc" /* yacc.c:1652  */
      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;
  *++yylsp = yyloc;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }

  yyerror_range[1] = yylloc;

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval, &yylloc);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYTERROR;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;

      yyerror_range[1] = *yylsp;
      yydestruct ("Error: popping",
                  yystos[yystate], yyvsp, yylsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  yyerror_range[2] = yylloc;
  /* Using YYLLOC is tempting, but would change the location of
     the lookahead.  YYLOC is available though.  */
  YYLLOC_DEFAULT (yyloc, yyerror_range, 2);
  *++yylsp = yyloc;

  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;


#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif


/*-----------------------------------------------------.
| yyreturn -- parsing is finished, return the result.  |
`-----------------------------------------------------*/
yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, &yylloc);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  yystos[*yyssp], yyvsp, yylsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  return yyresult;
}
#line 963 "Frontend/Parser/ParserBuechi.yy" /* yacc.c:1918  */


tShape ooppShape(tShape s)
{
	switch(s)
	{
	case AT_COMP: return AT_COMP;
	case AT_FIR: return AT_FIR;
	case AT_DL: return AT_DL;
	case AT_TEMP: return AT_TEMP;
	case AT_AND: return AT_OR;
	case AT_OR: return AT_AND;
	case AT_TRUE: return AT_FALSE;
	case AT_FALSE: return AT_TRUE;
	}
}

/// display a parser error and exit
void ptbuechi_error(char const* mess) __attribute__((noreturn));
void ptbuechi_error(char const* mess)
{
    extern char* ptbuechi_text; ///< the current token text from Flex
    yyerrors(ptbuechi_text, ptbuechi_lloc, mess);
}
