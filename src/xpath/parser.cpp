/* A Bison parser, made by GNU Bison 2.4.2.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2006, 2009-2010 Free Software
   Foundation, Inc.
   
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

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.4.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 0

/* Substitute the variable and function names.  */
#define yyparse         khtmlxpathyyparse
#define yylex           khtmlxpathyylex
#define yyerror         khtmlxpathyyerror
#define yylval          khtmlxpathyylval
#define yychar          khtmlxpathyychar
#define yydebug         khtmlxpathyydebug
#define yynerrs         khtmlxpathyynerrs


/* Copy the first part of user declarations.  */

/* Line 189 of yacc.c  */
#line 1 "parser.y"

#include "functions.h"
#include "path.h"
#include "predicate.h"
#include "util.h"
#include "tokenizer.h"

#include "expression.h"
#include "util.h"
#include "variablereference.h"

#include "dom/dom_string.h"
#include "dom/dom_exception.h"
#include "dom/dom3_xpath.h"
#include "xml/dom_stringimpl.h"
#include "xml/dom3_xpathimpl.h"

using namespace DOM;
using namespace DOM::XPath;
using namespace khtml;
using namespace khtml::XPath;



#include <QList>
#include <QPair>
#include <QtDebug>

#define YYDEBUG 1

Expression * khtmlParseXPathStatement( const DOM::DOMString &statement, int& ec );

static Expression *_topExpr;
static int xpathParseException;



/* Line 189 of yacc.c  */
#line 118 "parser.tab.c"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     EQOP = 258,
     RELOP = 259,
     MULOP = 260,
     MINUS = 261,
     PLUS = 262,
     AND = 263,
     OR = 264,
     AXISNAME = 265,
     NODETYPE = 266,
     PI = 267,
     FUNCTIONNAME = 268,
     LITERAL = 269,
     VARIABLEREFERENCE = 270,
     NUMBER = 271,
     DOTDOT = 272,
     SLASHSLASH = 273,
     NAMETEST = 274,
     ERROR = 275
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 214 of yacc.c  */
#line 39 "parser.y"

	khtml::XPath::Step::AxisType axisType;
	int        num;
	DOM::DOMString *str; // we use this and not DOMStringImpl*, so the
	                     // memory management for this is entirely manual,
	                     // and not an RC/manual hybrid
	khtml::XPath::Expression *expr;
	QList<khtml::XPath::Predicate *> *predList;
	QList<khtml::XPath::Expression *> *argList;
	khtml::XPath::Step *step;
	khtml::XPath::LocationPath *locationPath;



/* Line 214 of yacc.c  */
#line 189 "parser.tab.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */

/* Line 264 of yacc.c  */
#line 52 "parser.y"



/* Line 264 of yacc.c  */
#line 205 "parser.tab.c"

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
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
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
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
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
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  43
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   115

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  30
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  26
/* YYNRULES -- Number of rules.  */
#define YYNRULES  60
/* YYNRULES -- Number of states.  */
#define YYNSTATES  90

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   275

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      23,    24,     2,     2,    28,     2,    27,    21,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    22,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    25,     2,    26,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    29,     2,     2,     2,     2,     2,
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
      15,    16,    17,    18,    19,    20
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint8 yyprhs[] =
{
       0,     0,     3,     5,     7,     9,    11,    14,    17,    19,
      23,    27,    29,    32,    35,    39,    41,    43,    45,    47,
      51,    55,    60,    62,    65,    69,    71,    73,    75,    77,
      81,    83,    85,    87,    91,    96,    98,   102,   104,   106,
     110,   112,   114,   118,   122,   124,   127,   129,   133,   135,
     139,   141,   145,   147,   151,   153,   157,   161,   163,   167,
     169
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      31,     0,    -1,    49,    -1,    34,    -1,    33,    -1,    21,
      -1,    21,    34,    -1,    40,    34,    -1,    35,    -1,    34,
      21,    35,    -1,    34,    40,    35,    -1,    37,    -1,    37,
      38,    -1,    36,    37,    -1,    36,    37,    38,    -1,    41,
      -1,    10,    -1,    22,    -1,    19,    -1,    11,    23,    24,
      -1,    12,    23,    24,    -1,    12,    23,    14,    24,    -1,
      39,    -1,    38,    39,    -1,    25,    31,    26,    -1,    18,
      -1,    27,    -1,    17,    -1,    15,    -1,    23,    31,    24,
      -1,    14,    -1,    16,    -1,    43,    -1,    13,    23,    24,
      -1,    13,    23,    44,    24,    -1,    45,    -1,    44,    28,
      45,    -1,    31,    -1,    47,    -1,    46,    29,    47,    -1,
      32,    -1,    48,    -1,    48,    21,    34,    -1,    48,    40,
      34,    -1,    42,    -1,    42,    38,    -1,    50,    -1,    49,
       9,    50,    -1,    51,    -1,    50,     8,    51,    -1,    52,
      -1,    51,     3,    52,    -1,    53,    -1,    52,     4,    53,
      -1,    54,    -1,    53,     7,    54,    -1,    53,     6,    54,
      -1,    55,    -1,    54,     5,    55,    -1,    46,    -1,     6,
      55,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,    93,    93,   100,   105,   112,   117,   122,   130,   136,
     141,   149,   155,   162,   168,   175,   179,   181,   188,   202,
     207,   209,   220,   226,   233,   240,   247,   252,   259,   265,
     270,   276,   282,   286,   298,   312,   318,   325,   330,   332,
     341,   346,   351,   356,   364,   369,   376,   378,   385,   387,
     394,   396,   403,   405,   412,   414,   419,   426,   428,   435,
     437
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "EQOP", "RELOP", "MULOP", "MINUS",
  "PLUS", "AND", "OR", "AXISNAME", "NODETYPE", "PI", "FUNCTIONNAME",
  "LITERAL", "VARIABLEREFERENCE", "NUMBER", "DOTDOT", "SLASHSLASH",
  "NAMETEST", "ERROR", "'/'", "'@'", "'('", "')'", "'['", "']'", "'.'",
  "','", "'|'", "$accept", "Expr", "LocationPath", "AbsoluteLocationPath",
  "RelativeLocationPath", "Step", "AxisSpecifier", "NodeTest",
  "PredicateList", "Predicate", "DescendantOrSelf", "AbbreviatedStep",
  "PrimaryExpr", "FunctionCall", "ArgumentList", "Argument", "UnionExpr",
  "PathExpr", "FilterExpr", "OrExpr", "AndExpr", "EqualityExpr",
  "RelationalExpr", "AdditiveExpr", "MultiplicativeExpr", "UnaryExpr", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,    47,    64,    40,    41,    91,    93,    46,    44,   124
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    30,    31,    32,    32,    33,    33,    33,    34,    34,
      34,    35,    35,    35,    35,    35,    36,    36,    37,    37,
      37,    37,    38,    38,    39,    40,    41,    41,    42,    42,
      42,    42,    42,    43,    43,    44,    44,    45,    46,    46,
      47,    47,    47,    47,    48,    48,    49,    49,    50,    50,
      51,    51,    52,    52,    53,    53,    53,    54,    54,    55,
      55
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     1,     1,     2,     2,     1,     3,
       3,     1,     2,     2,     3,     1,     1,     1,     1,     3,
       3,     4,     1,     2,     3,     1,     1,     1,     1,     3,
       1,     1,     1,     3,     4,     1,     3,     1,     1,     3,
       1,     1,     3,     3,     1,     2,     1,     3,     1,     3,
       1,     3,     1,     3,     1,     3,     3,     1,     3,     1,
       2
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     0,    16,     0,     0,     0,    30,    28,    31,    27,
      25,    18,     5,    17,     0,    26,     0,    40,     4,     3,
       8,     0,    11,     0,    15,    44,    32,    59,    38,    41,
       2,    46,    48,    50,    52,    54,    57,    60,     0,     0,
       0,     6,     0,     1,     0,     0,    13,     0,    12,    22,
       7,    45,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    19,     0,    20,    33,    37,     0,    35,    29,
       9,    10,    14,     0,    23,    39,    42,    43,    47,    49,
      51,    53,    56,    55,    58,    21,    34,     0,    24,    36
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
      -1,    66,    17,    18,    19,    20,    21,    22,    48,    49,
      23,    24,    25,    26,    67,    68,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -45
static const yytype_int8 yypact[] =
{
      70,    70,   -45,   -17,     4,    14,   -45,   -45,   -45,   -45,
     -45,   -45,    -2,   -45,    70,   -45,    39,   -45,   -45,     3,
     -45,    19,    17,    -2,   -45,    17,   -45,    18,   -45,    15,
      34,    38,    45,    46,     6,    47,   -45,   -45,    27,     5,
      51,     3,    29,   -45,    -2,    -2,    17,    70,    17,   -45,
       3,    17,    88,    -2,    -2,    70,    70,    70,    70,    70,
      70,    70,   -45,    30,   -45,   -45,   -45,    -6,   -45,   -45,
     -45,   -45,    17,    53,   -45,   -45,     3,     3,    38,    45,
      46,     6,    47,    47,   -45,   -45,   -45,    70,   -45,   -45
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -45,     2,   -45,   -45,    -9,   -10,   -45,    35,   -20,   -44,
     -18,   -45,   -45,   -45,   -45,   -32,   -45,    25,   -45,   -45,
      16,    40,    33,    36,   -19,    -1
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const yytype_uint8 yytable[] =
{
      37,    45,    16,    41,    74,    51,    38,    74,     2,     3,
       4,    54,    59,    60,    50,     9,    42,    11,    86,    63,
      13,    10,    87,    45,    44,    15,    72,    39,    74,    64,
       3,     4,    45,    10,    70,    71,    53,    40,    11,    43,
      82,    83,    47,    55,    76,    77,    56,    52,    57,    73,
      58,    62,    61,    69,    85,    89,    46,     1,    45,    45,
      84,     2,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    78,    12,    13,    14,    65,     1,    75,    15,    88,
       2,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      80,    12,    13,    14,    81,     0,    79,    15,     2,     3,
       4,     5,     6,     7,     8,     9,    10,    11,     0,    12,
      13,    14,     0,     0,     0,    15
};

static const yytype_int8 yycheck[] =
{
       1,    19,     0,    12,    48,    25,    23,    51,    10,    11,
      12,    29,     6,     7,    23,    17,    14,    19,    24,    14,
      22,    18,    28,    41,    21,    27,    46,    23,    72,    24,
      11,    12,    50,    18,    44,    45,    21,    23,    19,     0,
      59,    60,    25,     9,    53,    54,     8,    29,     3,    47,
       4,    24,     5,    24,    24,    87,    21,     6,    76,    77,
      61,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    55,    21,    22,    23,    24,     6,    52,    27,    26,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      57,    21,    22,    23,    58,    -1,    56,    27,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    -1,    21,
      22,    23,    -1,    -1,    -1,    27
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     6,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    21,    22,    23,    27,    31,    32,    33,    34,
      35,    36,    37,    40,    41,    42,    43,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    55,    23,    23,
      23,    34,    31,     0,    21,    40,    37,    25,    38,    39,
      34,    38,    29,    21,    40,     9,     8,     3,     4,     6,
       7,     5,    24,    14,    24,    24,    31,    44,    45,    24,
      35,    35,    38,    31,    39,    47,    34,    34,    50,    51,
      52,    53,    54,    54,    55,    24,    24,    28,    26,    45
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  However,
   YYFAIL appears to be in use.  Nevertheless, it is formally deprecated
   in Bison 2.4.2's NEWS entry, where a plan to phase it out is
   discussed.  */

#define YYFAIL		goto yyerrlab
#if defined YYFAIL
  /* This is here to suppress warnings from the GCC cpp's
     -Wunused-macros.  Normally we don't worry about that warning, but
     some users do, and we want to make it easy for users to remove
     YYFAIL uses, which will produce warnings from Bison 2.5.  */
#endif

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

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
#ifndef	YYINITDEPTH
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
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
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
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
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
	    /* Fall through.  */
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

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}

/* Prevent warnings from -Wmissing-prototypes.  */
#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */


/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*-------------------------.
| yyparse or yypush_parse.  |
`-------------------------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{


    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */
  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
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
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

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
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
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
      if (yyn == 0 || yyn == YYTABLE_NINF)
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
  *++yyvsp = yylval;

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
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:

/* Line 1464 of yacc.c  */
#line 94 "parser.y"
    {
		_topExpr = (yyvsp[(1) - (1)].expr);
	;}
    break;

  case 3:

/* Line 1464 of yacc.c  */
#line 101 "parser.y"
    {
		(yyval.locationPath)->m_absolute = false;
	;}
    break;

  case 4:

/* Line 1464 of yacc.c  */
#line 106 "parser.y"
    {
		(yyval.locationPath)->m_absolute = true;
	;}
    break;

  case 5:

/* Line 1464 of yacc.c  */
#line 113 "parser.y"
    {
		(yyval.locationPath) = new LocationPath;
	;}
    break;

  case 6:

/* Line 1464 of yacc.c  */
#line 118 "parser.y"
    {
		(yyval.locationPath) = (yyvsp[(2) - (2)].locationPath);
	;}
    break;

  case 7:

/* Line 1464 of yacc.c  */
#line 123 "parser.y"
    {
		(yyval.locationPath) = (yyvsp[(2) - (2)].locationPath);
		(yyval.locationPath)->m_steps.prepend( (yyvsp[(1) - (2)].step) );
	;}
    break;

  case 8:

/* Line 1464 of yacc.c  */
#line 131 "parser.y"
    {
		(yyval.locationPath) = new LocationPath;
		(yyval.locationPath)->m_steps.append( (yyvsp[(1) - (1)].step) );
	;}
    break;

  case 9:

/* Line 1464 of yacc.c  */
#line 137 "parser.y"
    {
		(yyval.locationPath)->m_steps.append( (yyvsp[(3) - (3)].step) );
	;}
    break;

  case 10:

/* Line 1464 of yacc.c  */
#line 142 "parser.y"
    {
		(yyval.locationPath)->m_steps.append( (yyvsp[(2) - (3)].step) );
		(yyval.locationPath)->m_steps.append( (yyvsp[(3) - (3)].step) );
	;}
    break;

  case 11:

/* Line 1464 of yacc.c  */
#line 150 "parser.y"
    {
		(yyval.step) = new Step( Step::ChildAxis, *(yyvsp[(1) - (1)].str) );
		delete (yyvsp[(1) - (1)].str);
	;}
    break;

  case 12:

/* Line 1464 of yacc.c  */
#line 156 "parser.y"
    {
		(yyval.step) = new Step( Step::ChildAxis, *(yyvsp[(1) - (2)].str), *(yyvsp[(2) - (2)].predList) );
		delete (yyvsp[(1) - (2)].str);
		delete (yyvsp[(2) - (2)].predList);
	;}
    break;

  case 13:

/* Line 1464 of yacc.c  */
#line 163 "parser.y"
    {
		(yyval.step) = new Step( (yyvsp[(1) - (2)].axisType), *(yyvsp[(2) - (2)].str) );
		delete (yyvsp[(2) - (2)].str);
	;}
    break;

  case 14:

/* Line 1464 of yacc.c  */
#line 169 "parser.y"
    {
		(yyval.step) = new Step( (yyvsp[(1) - (3)].axisType), *(yyvsp[(2) - (3)].str),  *(yyvsp[(3) - (3)].predList) );
		delete (yyvsp[(2) - (3)].str);
		delete (yyvsp[(3) - (3)].predList);
	;}
    break;

  case 17:

/* Line 1464 of yacc.c  */
#line 182 "parser.y"
    {
		(yyval.axisType) = Step::AttributeAxis;
	;}
    break;

  case 18:

/* Line 1464 of yacc.c  */
#line 189 "parser.y"
    {
		const int colon = (yyvsp[(1) - (1)].str)->find( ':' );
		if ( colon > -1 ) {
			DOMString prefix( (yyvsp[(1) - (1)].str)->substring( 0, colon ) );
			XPathNSResolverImpl *resolver = Expression::evaluationContext().resolver;
			if ( !resolver || resolver->lookupNamespaceURI( prefix ).isNull() ) {
				qWarning() << "Found unknown namespace prefix " << prefix.string();
				xpathParseException = DOMException::NAMESPACE_ERR;
				YYABORT;
			}
		}
	;}
    break;

  case 19:

/* Line 1464 of yacc.c  */
#line 203 "parser.y"
    {
		(yyval.str) = new DOMString( *(yyvsp[(1) - (3)].str) + DOMString("()") );
	;}
    break;

  case 21:

/* Line 1464 of yacc.c  */
#line 210 "parser.y"
    {
		QString s = (yyvsp[(1) - (4)].str)->string() + QLatin1Char(' ') + (yyvsp[(3) - (4)].str)->string();
		s = s.trimmed();
		(yyval.str) = new DOMString( s );
		delete (yyvsp[(1) - (4)].str);
		delete (yyvsp[(3) - (4)].str);
	;}
    break;

  case 22:

/* Line 1464 of yacc.c  */
#line 221 "parser.y"
    {
		(yyval.predList) = new QList<Predicate *>;
		(yyval.predList)->append( new Predicate( (yyvsp[(1) - (1)].expr) ) );
	;}
    break;

  case 23:

/* Line 1464 of yacc.c  */
#line 227 "parser.y"
    {
		(yyval.predList)->append( new Predicate( (yyvsp[(2) - (2)].expr) ) );
	;}
    break;

  case 24:

/* Line 1464 of yacc.c  */
#line 234 "parser.y"
    {
		(yyval.expr) = (yyvsp[(2) - (3)].expr);
	;}
    break;

  case 25:

/* Line 1464 of yacc.c  */
#line 241 "parser.y"
    {
		(yyval.step) = new Step( Step::DescendantOrSelfAxis, "node()" );
	;}
    break;

  case 26:

/* Line 1464 of yacc.c  */
#line 248 "parser.y"
    {
		(yyval.step) = new Step( Step::SelfAxis, "node()" );
	;}
    break;

  case 27:

/* Line 1464 of yacc.c  */
#line 253 "parser.y"
    {
		(yyval.step) = new Step( Step::ParentAxis, "node()" );
	;}
    break;

  case 28:

/* Line 1464 of yacc.c  */
#line 260 "parser.y"
    {
		(yyval.expr) = new VariableReference( *(yyvsp[(1) - (1)].str) );
		delete (yyvsp[(1) - (1)].str);
	;}
    break;

  case 29:

/* Line 1464 of yacc.c  */
#line 266 "parser.y"
    {
		(yyval.expr) = (yyvsp[(2) - (3)].expr);
	;}
    break;

  case 30:

/* Line 1464 of yacc.c  */
#line 271 "parser.y"
    {
		(yyval.expr) = new String( *(yyvsp[(1) - (1)].str) );
		delete (yyvsp[(1) - (1)].str);
	;}
    break;

  case 31:

/* Line 1464 of yacc.c  */
#line 277 "parser.y"
    {
		(yyval.expr) = new Number( (yyvsp[(1) - (1)].str)->toFloat() );
		delete (yyvsp[(1) - (1)].str);
	;}
    break;

  case 33:

/* Line 1464 of yacc.c  */
#line 287 "parser.y"
    {
		Function* f = FunctionLibrary::self().getFunction( *(yyvsp[(1) - (3)].str) );
		delete (yyvsp[(1) - (3)].str);
		if (!f) {
			xpathParseException = XPathException::toCode(INVALID_EXPRESSION_ERR);
			YYABORT;
		}
		
		(yyval.expr) = f;
	;}
    break;

  case 34:

/* Line 1464 of yacc.c  */
#line 299 "parser.y"
    {
		Function* f = FunctionLibrary::self().getFunction( *(yyvsp[(1) - (4)].str), *(yyvsp[(3) - (4)].argList) );
		delete (yyvsp[(1) - (4)].str);
		delete (yyvsp[(3) - (4)].argList);
		if (!f) {
			xpathParseException = XPathException::toCode(INVALID_EXPRESSION_ERR);
			YYABORT;
		}
		(yyval.expr) = f;
	;}
    break;

  case 35:

/* Line 1464 of yacc.c  */
#line 313 "parser.y"
    {
		(yyval.argList) = new QList<Expression *>;
		(yyval.argList)->append( (yyvsp[(1) - (1)].expr) );
	;}
    break;

  case 36:

/* Line 1464 of yacc.c  */
#line 319 "parser.y"
    {
		(yyval.argList)->append( (yyvsp[(3) - (3)].expr) );
	;}
    break;

  case 39:

/* Line 1464 of yacc.c  */
#line 333 "parser.y"
    {
		(yyval.expr) = new Union;
		(yyval.expr)->addSubExpression( (yyvsp[(1) - (3)].expr) );
		(yyval.expr)->addSubExpression( (yyvsp[(3) - (3)].expr) );
	;}
    break;

  case 40:

/* Line 1464 of yacc.c  */
#line 342 "parser.y"
    {
		(yyval.expr) = (yyvsp[(1) - (1)].locationPath);
	;}
    break;

  case 41:

/* Line 1464 of yacc.c  */
#line 347 "parser.y"
    {
		(yyval.expr) = (yyvsp[(1) - (1)].expr);
	;}
    break;

  case 42:

/* Line 1464 of yacc.c  */
#line 352 "parser.y"
    {
		(yyval.expr) = new Path( static_cast<Filter *>( (yyvsp[(1) - (3)].expr) ), (yyvsp[(3) - (3)].locationPath) );
	;}
    break;

  case 43:

/* Line 1464 of yacc.c  */
#line 357 "parser.y"
    {
		(yyvsp[(3) - (3)].locationPath)->m_steps.prepend( (yyvsp[(2) - (3)].step) );
		(yyval.expr) = new Path( static_cast<Filter *>( (yyvsp[(1) - (3)].expr) ), (yyvsp[(3) - (3)].locationPath) );
	;}
    break;

  case 44:

/* Line 1464 of yacc.c  */
#line 365 "parser.y"
    {
		(yyval.expr) = (yyvsp[(1) - (1)].expr);
	;}
    break;

  case 45:

/* Line 1464 of yacc.c  */
#line 370 "parser.y"
    {
		(yyval.expr) = new Filter( (yyvsp[(1) - (2)].expr), *(yyvsp[(2) - (2)].predList) );
	;}
    break;

  case 47:

/* Line 1464 of yacc.c  */
#line 379 "parser.y"
    {
		(yyval.expr) = new LogicalOp( LogicalOp::OP_Or, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr) );
	;}
    break;

  case 49:

/* Line 1464 of yacc.c  */
#line 388 "parser.y"
    {
		(yyval.expr) = new LogicalOp( LogicalOp::OP_And, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr) );
	;}
    break;

  case 51:

/* Line 1464 of yacc.c  */
#line 397 "parser.y"
    {
		(yyval.expr) = new RelationOp( (yyvsp[(2) - (3)].num), (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr) );
	;}
    break;

  case 53:

/* Line 1464 of yacc.c  */
#line 406 "parser.y"
    {
		(yyval.expr) = new RelationOp( (yyvsp[(2) - (3)].num), (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr) );
	;}
    break;

  case 55:

/* Line 1464 of yacc.c  */
#line 415 "parser.y"
    {
		(yyval.expr) = new NumericOp( NumericOp::OP_Add, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr) );
	;}
    break;

  case 56:

/* Line 1464 of yacc.c  */
#line 420 "parser.y"
    {
		(yyval.expr) = new NumericOp( NumericOp::OP_Sub, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr) );
	;}
    break;

  case 58:

/* Line 1464 of yacc.c  */
#line 429 "parser.y"
    {
		(yyval.expr) = new NumericOp( (yyvsp[(2) - (3)].num), (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr) );
	;}
    break;

  case 60:

/* Line 1464 of yacc.c  */
#line 438 "parser.y"
    {
		(yyval.expr) = new Negative;
		(yyval.expr)->addSubExpression( (yyvsp[(2) - (2)].expr) );
	;}
    break;



/* Line 1464 of yacc.c  */
#line 1953 "parser.tab.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }



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
		      yytoken, &yylval);
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

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
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
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
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


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;


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

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
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
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}



/* Line 1684 of yacc.c  */
#line 444 "parser.y"


namespace khtml {
namespace XPath {

Expression *khtmlParseXPathStatement( const DOM::DOMString &statement, int& ec )
{
//	qDebug() << "Parsing " << statement;
	xpathParseException = 0;
	_topExpr = 0;
	initTokenizer( statement );
	yyparse();

	if (xpathParseException)
		ec = xpathParseException;
	return _topExpr;
}

void khtmlxpathyyerror(const char *str)
{
	// XXX Clean xpathyylval.str up to avoid leaking it.
	fprintf(stderr, "error: %s\n", str);
	xpathParseException = XPathException::toCode(INVALID_EXPRESSION_ERR);
}

} // namespace XPath
} // namespace khtml

// kate: indent-width 4; replace-tabs off; tab-width 4; space-indent off;

