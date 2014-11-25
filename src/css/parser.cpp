/* A Bison parser, made by GNU Bison 2.5.1.  */

/* Bison implementation for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2012 Free Software Foundation, Inc.
   
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
#define YYBISON_VERSION "2.5.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 0

/* Substitute the variable and function names.  */
#define yyparse         cssyyparse
#define yylex           cssyylex
#define yyerror         cssyyerror
#define yylval          cssyylval
#define yychar          cssyychar
#define yydebug         cssyydebug
#define yynerrs         cssyynerrs


/* Copy the first part of user declarations.  */



/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 2002-2003 Lars Knoll (knoll@kde.org)
 *  Copyright (c) 2003 Apple Computer
 *  Copyright (C) 2003 Dirk Mueller (mueller@kde.org)
 *  Copyright (C) 2008 Germain Garand (germain@ebooksfrance.org)
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include <string.h>
#include <stdlib.h>

#include <dom/dom_string.h>
#include <xml/dom_docimpl.h>
#include <css/cssstyleselector.h>
#include <css/css_ruleimpl.h>
#include <css/css_stylesheetimpl.h>
#include <css/css_valueimpl.h>
#include <css/css_mediaquery.h>
#include "cssparser.h"



#include <assert.h>
#include <QDebug>

//#define CSS_DEBUG

using namespace DOM;

//
// The following file defines the function
//     const struct props *findProp(const char *word, int len)
//
// with 'props->id' a CSS property in the range from CSS_PROP_MIN to
// (and including) CSS_PROP_TOTAL-1

// Turn off gnu90 inlining to avoid linker errors
#undef __GNUC_STDC_INLINE__
#undef __GNUC_GNU_INLINE__
#include "cssproperties.c"
#include "cssvalues.c"

static QHash<QString,int> *sCompatibleProperties = 0;

static const int sMinCompatPropLen = 21; // shortest key in the hash below

static void initCompatibleProperties() {
     QHash<QString,int> *&cp = sCompatibleProperties;
     // Hash of (Property name, Vendor Prefix length)
     cp = new QHash<QString, int>;
     cp->insert("-webkit-background-clip", 7);
     cp->insert("-webkit-background-origin", 7);
     cp->insert("-webkit-background-size", 7);
     cp->insert("-webkit-border-top-right-radius", 7);
     cp->insert("-webkit-border-bottom-right-radius", 7);
     cp->insert("-webkit-border-bottom-left-radius", 7);
     cp->insert("-webkit-border-top-left-radius", 7);
     cp->insert("-webkit-border-radius", 7);
}

int DOM::getPropertyID(const char *tagStr, int len)
{
    { // HTML CSS Properties
        const struct css_prop *prop = findProp(tagStr, len);
        if (!prop) {
            return 0;
        }

        return prop->id;
    }
}

int DOM::getValueID(const char *tagStr, int len)
{
    { // HTML CSS Values
        const struct css_value *val = findValue(tagStr, len);
        if (!val) {
            return 0;
        }

        return val->id;
    }
}

#define YYDEBUG 0
#define YYMAXDEPTH 10000
#define YYPARSE_PARAM parser
#define YYENABLE_NLS 0
#define YYLTYPE_IS_TRIVIAL 1



# ifndef YY_NULL
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULL nullptr
#  else
#   define YY_NULL 0
#  endif
# endif

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
     REDUCE = 258,
     S = 259,
     SGML_CD = 260,
     INCLUDES = 261,
     DASHMATCH = 262,
     BEGINSWITH = 263,
     ENDSWITH = 264,
     CONTAINS = 265,
     STRING = 266,
     IDENT = 267,
     NTH = 268,
     HASH = 269,
     HEXCOLOR = 270,
     IMPORT_SYM = 271,
     PAGE_SYM = 272,
     MEDIA_SYM = 273,
     FONT_FACE_SYM = 274,
     CHARSET_SYM = 275,
     NAMESPACE_SYM = 276,
     KHTML_RULE_SYM = 277,
     KHTML_DECLS_SYM = 278,
     KHTML_VALUE_SYM = 279,
     KHTML_MEDIAQUERY_SYM = 280,
     KHTML_SELECTORS_SYM = 281,
     IMPORTANT_SYM = 282,
     MEDIA_ONLY = 283,
     MEDIA_NOT = 284,
     MEDIA_AND = 285,
     QEMS = 286,
     EMS = 287,
     EXS = 288,
     CHS = 289,
     PXS = 290,
     CMS = 291,
     MMS = 292,
     INS = 293,
     PTS = 294,
     PCS = 295,
     DEGS = 296,
     RADS = 297,
     GRADS = 298,
     MSECS = 299,
     SECS = 300,
     HERZ = 301,
     KHERZ = 302,
     DPI = 303,
     DPCM = 304,
     DIMEN = 305,
     PERCENTAGE = 306,
     FLOAT = 307,
     INTEGER = 308,
     URI = 309,
     FUNCTION = 310,
     NOTFUNCTION = 311,
     UNICODERANGE = 312
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{


    CSSRuleImpl *rule;
    CSSSelector *selector;
    QList<CSSSelector *> *selectorList;
    bool ok;
    MediaListImpl *mediaList;
    CSSMediaRuleImpl *mediaRule;
    CSSRuleListImpl *ruleList;
    ParseString string;
    double val;
    int prop_id;
    unsigned int attribute;
    unsigned int element;
    CSSSelector::Relation relation;
    CSSSelector::Match match;
    bool b;
    char tok;
    Value value;
    ValueList *valueList;

    khtml::MediaQuery *mediaQuery;
    khtml::MediaQueryExp *mediaQueryExp;
    QList<khtml::MediaQueryExp *> *mediaQueryExpList;
    khtml::MediaQuery::Restrictor mediaQueryRestrictor;



} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */



static inline int cssyyerror(const char *x)
{
#ifdef CSS_DEBUG
    qDebug("%s", x);
#else
    Q_UNUSED(x);
#endif
    return 1;
}

static int cssyylex(YYSTYPE *yylval) {
    return CSSParser::current()->lex(yylval);
}

#define null 1




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
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
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
#   if ! defined malloc && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
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

# define YYCOPY_NEEDED 1

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
      while (YYID (0))
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  19
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   607

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  77
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  69
/* YYNRULES -- Number of rules.  */
#define YYNRULES  200
/* YYNRULES -- Number of states.  */
#define YYNSTATES  380

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   312

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      67,    68,    19,    65,    69,    72,    17,    75,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    16,    66,
       2,    74,    71,     2,    76,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    18,     2,    73,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    63,    20,    64,    70,     2,     2,     2,
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
      15,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,    10,    13,    16,    19,    22,    25,    27,
      29,    31,    38,    41,    47,    53,    59,    60,    62,    63,
      66,    67,    70,    73,    74,    76,    82,    86,    90,    91,
      95,   102,   106,   110,   111,   115,   120,   124,   128,   132,
     133,   136,   137,   141,   143,   145,   147,   149,   151,   153,
     156,   158,   160,   162,   165,   167,   169,   172,   173,   178,
     186,   188,   194,   195,   199,   200,   202,   204,   206,   211,
     212,   214,   216,   221,   224,   232,   239,   240,   244,   247,
     251,   255,   259,   263,   267,   270,   273,   276,   278,   280,
     283,   285,   290,   293,   295,   298,   302,   306,   309,   311,
     314,   317,   319,   322,   324,   327,   331,   334,   336,   338,
     341,   344,   346,   348,   350,   353,   356,   358,   360,   362,
     364,   367,   370,   375,   384,   390,   400,   402,   404,   406,
     408,   410,   412,   414,   416,   419,   423,   430,   438,   445,
     452,   459,   464,   469,   474,   480,   486,   490,   494,   499,
     504,   510,   513,   516,   519,   520,   522,   526,   529,   532,
     533,   535,   538,   541,   544,   547,   550,   553,   555,   557,
     560,   563,   566,   569,   572,   575,   578,   581,   584,   587,
     590,   593,   596,   599,   602,   605,   608,   611,   614,   617,
     620,   623,   629,   633,   636,   640,   644,   647,   653,   657,
     659
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
      78,     0,    -1,    86,    88,    87,    90,    92,    96,    -1,
      80,    86,    -1,    81,    86,    -1,    82,    86,    -1,    84,
      86,    -1,    83,    86,    -1,   116,    -1,    91,    -1,    93,
      -1,    27,    63,    86,    79,    86,    64,    -1,    28,   131,
      -1,    29,    63,    86,   136,    64,    -1,    31,    63,    86,
     117,    64,    -1,    30,     4,    86,   106,    64,    -1,    -1,
      65,    -1,    -1,    86,     4,    -1,    -1,    87,     5,    -1,
      87,     4,    -1,    -1,    89,    -1,    25,    86,    11,    86,
      66,    -1,    25,     1,   144,    -1,    25,     1,    66,    -1,
      -1,    90,    91,    87,    -1,    21,    86,    99,    86,   107,
      66,    -1,    21,     1,   144,    -1,    21,     1,    66,    -1,
      -1,    92,    94,    87,    -1,    26,    86,    95,    99,    -1,
      93,    86,    66,    -1,    26,     1,   144,    -1,    26,     1,
      66,    -1,    -1,    12,     4,    -1,    -1,    96,    97,    87,
      -1,   116,    -1,   109,    -1,   112,    -1,   113,    -1,   143,
      -1,   142,    -1,    91,     1,    -1,   116,    -1,   143,    -1,
     142,    -1,    91,     1,    -1,    11,    -1,    59,    -1,    12,
      86,    -1,    -1,    16,    86,   136,    86,    -1,    67,    86,
     100,    86,   101,    68,    86,    -1,   102,    -1,   103,    86,
      35,    86,   102,    -1,    -1,    35,    86,   103,    -1,    -1,
      33,    -1,    34,    -1,   103,    -1,   105,    86,   111,   104,
      -1,    -1,   108,    -1,   106,    -1,   108,    69,    86,   106,
      -1,   108,     1,    -1,    23,    86,   108,    63,    86,   110,
      64,    -1,    23,    86,    63,    86,   110,    64,    -1,    -1,
     110,    98,    86,    -1,    12,    86,    -1,    22,     1,   144,
      -1,    22,     1,    66,    -1,    24,    86,   131,    -1,    24,
       1,   144,    -1,    24,     1,    66,    -1,    65,    86,    -1,
      70,    86,    -1,    71,    86,    -1,    72,    -1,    65,    -1,
     117,   131,    -1,   118,    -1,   117,    69,    86,   118,    -1,
     117,     1,    -1,   120,    -1,   118,     4,    -1,   118,     4,
     120,    -1,   118,   114,   120,    -1,   118,     1,    -1,    20,
      -1,    19,    20,    -1,    12,    20,    -1,   122,    -1,   122,
     123,    -1,   123,    -1,   119,   122,    -1,   119,   122,   123,
      -1,   119,   123,    -1,   122,    -1,   124,    -1,   119,   122,
      -1,   119,   124,    -1,    12,    -1,    19,    -1,   124,    -1,
     123,   124,    -1,   123,     1,    -1,    14,    -1,   125,    -1,
     127,    -1,   130,    -1,    17,    12,    -1,    12,    86,    -1,
      18,    86,   126,    73,    -1,    18,    86,   126,   128,    86,
     129,    86,    73,    -1,    18,    86,   119,   126,    73,    -1,
      18,    86,   119,   126,   128,    86,   129,    86,    73,    -1,
      74,    -1,     6,    -1,     7,    -1,     8,    -1,     9,    -1,
      10,    -1,    12,    -1,    11,    -1,    16,    12,    -1,    16,
      16,    12,    -1,    16,    60,    86,    13,    86,    68,    -1,
      16,    60,    86,    85,    58,    86,    68,    -1,    16,    60,
      86,    12,    86,    68,    -1,    16,    60,    86,    11,    86,
      68,    -1,    16,    61,    86,   121,    86,    68,    -1,    63,
      86,   133,    64,    -1,    63,    86,     1,    64,    -1,    63,
      86,   132,    64,    -1,    63,    86,   132,   133,    64,    -1,
      63,    86,   132,     1,    64,    -1,   133,    66,    86,    -1,
       1,    66,    86,    -1,   132,   133,    66,    86,    -1,   132,
       1,    66,    86,    -1,   134,    16,    86,   136,   135,    -1,
       1,   144,    -1,    12,    86,    -1,    32,    86,    -1,    -1,
     138,    -1,   136,   137,   138,    -1,    75,    86,    -1,    69,
      86,    -1,    -1,   139,    -1,   115,   139,    -1,    55,    86,
      -1,    11,    86,    -1,    12,    86,    -1,    59,    86,    -1,
      62,    86,    -1,   141,    -1,   140,    -1,    58,    86,    -1,
      57,    86,    -1,    56,    86,    -1,    40,    86,    -1,    41,
      86,    -1,    42,    86,    -1,    43,    86,    -1,    44,    86,
      -1,    45,    86,    -1,    46,    86,    -1,    47,    86,    -1,
      48,    86,    -1,    49,    86,    -1,    50,    86,    -1,    51,
      86,    -1,    52,    86,    -1,    37,    86,    -1,    36,    86,
      -1,    38,    86,    -1,    39,    86,    -1,    53,    86,    -1,
      54,    86,    -1,    60,    86,   136,    68,    86,    -1,    60,
      86,     1,    -1,    15,    86,    -1,    76,     1,   144,    -1,
      76,     1,    66,    -1,     1,   144,    -1,    63,     1,   145,
       1,    64,    -1,    63,     1,    64,    -1,   144,    -1,   145,
       1,   144,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   311,   311,   312,   313,   314,   315,   316,   320,   321,
     322,   326,   333,   339,   364,   376,   382,   384,   388,   389,
     392,   394,   395,   398,   400,   403,   412,   414,   418,   420,
     431,   441,   444,   450,   451,   455,   465,   473,   474,   478,
     479,   482,   484,   495,   496,   497,   498,   499,   500,   501,
     505,   506,   507,   508,   512,   513,   517,   523,   526,   532,
     538,   542,   549,   552,   558,   561,   564,   570,   573,   579,
     582,   587,   591,   596,   603,   614,   626,   627,   637,   655,
     658,   664,   671,   674,   680,   681,   682,   686,   687,   691,
     713,   726,   744,   754,   757,   760,   774,   788,   795,   796,
     797,   801,   806,   813,   820,   828,   838,   851,   856,   863,
     871,   884,   888,   894,   897,   907,   914,   928,   929,   930,
     934,   951,   958,   964,   971,   980,   993,   996,   999,  1002,
    1005,  1008,  1014,  1015,  1019,  1025,  1031,  1038,  1045,  1052,
    1059,  1068,  1071,  1074,  1077,  1082,  1088,  1092,  1095,  1100,
    1106,  1128,  1134,  1156,  1157,  1161,  1165,  1181,  1184,  1187,
    1193,  1194,  1196,  1197,  1198,  1204,  1205,  1206,  1208,  1214,
    1215,  1216,  1217,  1218,  1219,  1220,  1221,  1222,  1223,  1224,
    1225,  1226,  1227,  1228,  1229,  1230,  1231,  1232,  1233,  1234,
    1235,  1240,  1248,  1264,  1271,  1277,  1286,  1312,  1313,  1317,
    1318
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "REDUCE", "S", "SGML_CD", "INCLUDES",
  "DASHMATCH", "BEGINSWITH", "ENDSWITH", "CONTAINS", "STRING", "IDENT",
  "NTH", "HASH", "HEXCOLOR", "':'", "'.'", "'['", "'*'", "'|'",
  "IMPORT_SYM", "PAGE_SYM", "MEDIA_SYM", "FONT_FACE_SYM", "CHARSET_SYM",
  "NAMESPACE_SYM", "KHTML_RULE_SYM", "KHTML_DECLS_SYM", "KHTML_VALUE_SYM",
  "KHTML_MEDIAQUERY_SYM", "KHTML_SELECTORS_SYM", "IMPORTANT_SYM",
  "MEDIA_ONLY", "MEDIA_NOT", "MEDIA_AND", "QEMS", "EMS", "EXS", "CHS",
  "PXS", "CMS", "MMS", "INS", "PTS", "PCS", "DEGS", "RADS", "GRADS",
  "MSECS", "SECS", "HERZ", "KHERZ", "DPI", "DPCM", "DIMEN", "PERCENTAGE",
  "FLOAT", "INTEGER", "URI", "FUNCTION", "NOTFUNCTION", "UNICODERANGE",
  "'{'", "'}'", "'+'", "';'", "'('", "')'", "','", "'~'", "'>'", "'-'",
  "']'", "'='", "'/'", "'@'", "$accept", "stylesheet",
  "ruleset_or_import_or_namespace", "khtml_rule", "khtml_decls",
  "khtml_value", "khtml_selectors", "khtml_mediaquery", "maybe_plus",
  "maybe_space", "maybe_sgml", "maybe_charset", "charset", "import_list",
  "import", "namespace_list", "namespace_rule", "namespace",
  "maybe_ns_prefix", "rule_list", "rule", "safe_ruleset", "string_or_uri",
  "media_feature", "maybe_media_value", "media_query_exp",
  "media_query_exp_list", "maybe_and_media_query_exp_list",
  "maybe_media_restrictor", "media_query", "maybe_media_list",
  "media_list", "media", "ruleset_list", "medium", "page", "font_face",
  "combinator", "unary_operator", "ruleset", "selector_list", "selector",
  "namespace_selector", "simple_selector", "simple_css3_selector",
  "element_name", "specifier_list", "specifier", "class", "attrib_id",
  "attrib", "match", "ident_or_string", "pseudo", "declaration_block",
  "declaration_list", "declaration", "property", "prio", "expr",
  "operator", "term", "unary_term", "function", "hexcolor", "invalid_at",
  "invalid_rule", "invalid_block", "invalid_block_list", YY_NULL
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,    58,    46,    91,    42,
     124,   271,   272,   273,   274,   275,   276,   277,   278,   279,
     280,   281,   282,   283,   284,   285,   286,   287,   288,   289,
     290,   291,   292,   293,   294,   295,   296,   297,   298,   299,
     300,   301,   302,   303,   304,   305,   306,   307,   308,   309,
     310,   311,   312,   123,   125,    43,    59,    40,    41,    44,
     126,    62,    45,    93,    61,    47,    64
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    77,    78,    78,    78,    78,    78,    78,    79,    79,
      79,    80,    81,    82,    83,    84,    85,    85,    86,    86,
      87,    87,    87,    88,    88,    89,    89,    89,    90,    90,
      91,    91,    91,    92,    92,    93,    94,    94,    94,    95,
      95,    96,    96,    97,    97,    97,    97,    97,    97,    97,
      98,    98,    98,    98,    99,    99,   100,   101,   101,   102,
     103,   103,   104,   104,   105,   105,   105,   106,   106,   107,
     107,   108,   108,   108,   109,   109,   110,   110,   111,   112,
     112,   113,   113,   113,   114,   114,   114,   115,   115,   116,
     117,   117,   117,   118,   118,   118,   118,   118,   119,   119,
     119,   120,   120,   120,   120,   120,   120,   121,   121,   121,
     121,   122,   122,   123,   123,   123,   124,   124,   124,   124,
     125,   126,   127,   127,   127,   127,   128,   128,   128,   128,
     128,   128,   129,   129,   130,   130,   130,   130,   130,   130,
     130,   131,   131,   131,   131,   131,   132,   132,   132,   132,
     133,   133,   134,   135,   135,   136,   136,   137,   137,   137,
     138,   138,   138,   138,   138,   138,   138,   138,   138,   139,
     139,   139,   139,   139,   139,   139,   139,   139,   139,   139,
     139,   139,   139,   139,   139,   139,   139,   139,   139,   139,
     139,   140,   140,   141,   142,   142,   143,   144,   144,   145,
     145
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     6,     2,     2,     2,     2,     2,     1,     1,
       1,     6,     2,     5,     5,     5,     0,     1,     0,     2,
       0,     2,     2,     0,     1,     5,     3,     3,     0,     3,
       6,     3,     3,     0,     3,     4,     3,     3,     3,     0,
       2,     0,     3,     1,     1,     1,     1,     1,     1,     2,
       1,     1,     1,     2,     1,     1,     2,     0,     4,     7,
       1,     5,     0,     3,     0,     1,     1,     1,     4,     0,
       1,     1,     4,     2,     7,     6,     0,     3,     2,     3,
       3,     3,     3,     3,     2,     2,     2,     1,     1,     2,
       1,     4,     2,     1,     2,     3,     3,     2,     1,     2,
       2,     1,     2,     1,     2,     3,     2,     1,     1,     2,
       2,     1,     1,     1,     2,     2,     1,     1,     1,     1,
       2,     2,     4,     8,     5,     9,     1,     1,     1,     1,
       1,     1,     1,     1,     2,     3,     6,     7,     6,     6,
       6,     4,     4,     4,     5,     5,     3,     3,     4,     4,
       5,     2,     2,     2,     0,     1,     3,     2,     2,     0,
       1,     2,     2,     2,     2,     2,     2,     1,     1,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     5,     3,     2,     3,     3,     2,     5,     3,     1,
       3
};

/* YYDEFACT[STATE-NAME] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
      18,     0,     0,     0,     0,     0,     0,    18,    18,    18,
      18,    18,    23,    18,    18,    12,    18,    18,    18,     1,
       3,     4,     5,     7,     6,    19,     0,    20,    24,     0,
       0,     0,    64,     0,     0,     0,    28,   111,   116,     0,
       0,    18,   112,    98,     0,    18,    18,     9,    10,     8,
       0,     0,     0,    93,   101,     0,   113,   117,   118,   119,
       0,    18,     0,     0,     0,    18,    18,    18,    18,    18,
      18,    18,    18,    18,    18,    18,    18,    18,    18,    18,
      18,    18,    18,    18,    18,    18,    18,    18,    18,    18,
      18,    18,    18,    18,    88,    87,     0,   159,   155,   160,
     168,   167,    65,    66,    18,    60,    67,    18,     0,     0,
       0,    27,    26,    18,    22,    21,    33,   100,   134,     0,
      18,    18,   120,     0,    99,     0,     0,    39,     0,    92,
      18,    89,    97,    94,    18,    18,    18,     0,   111,   112,
     104,     0,     0,   115,   114,   142,    18,   151,   152,     0,
     143,     0,   141,    18,    18,   163,   164,   193,   186,   185,
     187,   188,   172,   173,   174,   175,   176,   177,   178,   179,
     180,   181,   182,   183,   184,   189,   190,   162,   171,   170,
     169,   165,     0,   166,   161,    13,    18,    18,     0,     0,
       0,     0,    15,    14,     0,     0,    20,    41,   135,    16,
       0,    18,     0,     0,     0,    32,    31,    54,    55,    18,
       0,     0,    11,     0,    95,    84,    85,    86,    96,     0,
     147,   145,    18,   144,    18,   146,     0,   192,   159,   158,
     157,   156,    18,    18,    18,    18,    62,   198,   199,     0,
      25,    29,     0,    18,    20,     0,    18,    18,    18,    17,
       0,     0,    18,   107,   108,   121,    18,     0,   127,   128,
     129,   130,   131,   122,   126,    18,    64,    40,    35,     0,
     149,   148,   159,    18,    56,    57,     0,    78,    18,    68,
       0,     0,     0,    34,     0,     0,    18,     0,     0,     0,
      20,    44,    45,    46,    43,    48,    47,     0,     0,     0,
      18,   109,   110,     0,   124,    18,     0,    71,     0,     0,
      18,   150,   191,    18,     0,    61,     0,   197,   200,    38,
      37,    36,   196,     0,    64,     0,     0,     0,    49,    42,
     139,   138,   136,     0,   140,     0,   133,   132,    18,    30,
      73,    18,   153,     0,    18,    63,    80,    79,    18,     0,
      83,    82,    81,   195,   194,   137,    18,     0,    64,   159,
      59,    76,    18,     0,   123,    72,    58,     0,    76,   125,
      75,     0,    18,    50,    52,    51,     0,    53,    77,    74
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     6,    46,     7,     8,     9,    10,    11,   250,   127,
      36,    27,    28,   116,   371,   197,    48,   244,   211,   245,
     290,   372,   209,   233,   314,   105,   106,   279,   107,   307,
     308,   309,   291,   367,   236,   292,   293,   137,    96,   373,
      50,    51,    52,    53,   252,    54,    55,    56,    57,   204,
      58,   265,   338,    59,    15,    62,    63,    64,   311,    97,
     188,    98,    99,   100,   101,   374,   375,   147,   239
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -191
static const yytype_int16 yypact[] =
{
     537,   -12,   113,   126,    59,   173,    36,  -191,  -191,  -191,
    -191,  -191,    20,  -191,  -191,  -191,  -191,  -191,  -191,  -191,
     211,   211,   211,   211,   211,  -191,    26,  -191,  -191,   507,
      52,   443,   287,   414,    -6,   227,   155,   206,  -191,   265,
     237,  -191,   252,  -191,   212,  -191,  -191,  -191,  -191,  -191,
      42,    54,   434,  -191,   589,   266,  -191,  -191,  -191,  -191,
     341,  -191,   157,   106,   250,  -191,  -191,  -191,  -191,  -191,
    -191,  -191,  -191,  -191,  -191,  -191,  -191,  -191,  -191,  -191,
    -191,  -191,  -191,  -191,  -191,  -191,  -191,  -191,  -191,  -191,
    -191,  -191,  -191,  -191,  -191,  -191,   535,   283,  -191,  -191,
    -191,  -191,  -191,  -191,  -191,  -191,    17,  -191,   230,    58,
     275,  -191,  -191,  -191,  -191,  -191,   278,  -191,  -191,   294,
    -191,  -191,  -191,   425,  -191,   246,   281,    30,   153,  -191,
    -191,  -191,  -191,   582,  -191,  -191,  -191,   582,  -191,  -191,
     589,   300,   398,  -191,  -191,  -191,  -191,  -191,   211,   356,
    -191,   138,  -191,  -191,  -191,   211,   211,   211,   211,   211,
     211,   211,   211,   211,   211,   211,   211,   211,   211,   211,
     211,   211,   211,   211,   211,   211,   211,   211,   211,   211,
     211,   211,   338,   211,  -191,  -191,  -191,  -191,   498,   241,
      27,   299,  -191,  -191,   269,   133,  -191,   282,  -191,   453,
     414,   206,   252,   303,   132,  -191,  -191,  -191,  -191,  -191,
     318,   229,  -191,   414,  -191,   211,   211,   211,  -191,   407,
     211,  -191,  -191,  -191,  -191,   211,   443,  -191,   221,   211,
     211,  -191,  -191,  -191,  -191,  -191,   289,  -191,  -191,   327,
    -191,   155,   199,  -191,  -191,   161,  -191,  -191,  -191,  -191,
     276,   434,  -191,  -191,  -191,   211,  -191,   433,  -191,  -191,
    -191,  -191,  -191,  -191,  -191,  -191,   208,  -191,  -191,   127,
     211,   211,   291,  -191,   211,    19,    43,   211,  -191,  -191,
     304,   280,   163,   155,   288,   360,  -191,   164,   400,   412,
    -191,  -191,  -191,  -191,  -191,  -191,  -191,    44,    46,   140,
    -191,  -191,  -191,   151,  -191,  -191,    28,  -191,   307,    32,
    -191,  -191,   211,  -191,   349,  -191,    43,  -191,  -191,  -191,
    -191,  -191,  -191,   343,   235,   372,   167,   393,  -191,   155,
    -191,  -191,  -191,   152,  -191,    28,  -191,  -191,  -191,  -191,
    -191,  -191,   211,   443,  -191,    17,  -191,  -191,  -191,   125,
    -191,  -191,  -191,  -191,  -191,  -191,  -191,    21,   287,    34,
     211,   211,  -191,    24,  -191,  -191,   211,   131,   211,  -191,
    -191,   426,  -191,  -191,  -191,  -191,   243,  -191,   211,  -191
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -191,  -191,  -191,  -191,  -191,  -191,  -191,  -191,  -191,     0,
    -190,  -191,  -191,  -191,   -17,  -191,   239,  -191,  -191,  -191,
    -191,  -191,   238,  -191,  -191,   184,   158,  -191,  -191,   -31,
    -191,   149,  -191,   107,  -191,  -191,  -191,  -191,  -191,   -27,
     471,   301,  -104,    -4,  -191,   -37,   -32,   -26,  -191,   309,
    -191,   259,   182,  -191,   -47,  -191,   458,  -191,  -191,  -177,
    -191,   334,   435,  -191,  -191,   284,   285,   -30,  -191
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -155
static const yytype_int16 yytable[] =
{
      12,   108,    49,   131,   112,   228,   241,    20,    21,    22,
      23,    24,    47,    29,    30,   140,    31,    32,    33,   203,
     141,   -18,   142,    25,    25,    25,    35,    34,    25,   144,
     -18,    25,    25,   340,    25,   313,    19,   -18,   -18,   336,
     337,   123,   210,   129,   126,    26,   128,    25,    25,   272,
      25,    13,   -18,    60,   283,   132,    25,   110,   133,   129,
     111,   148,   234,    17,    61,   155,   156,   157,   158,   159,
     160,   161,   162,   163,   164,   165,   166,   167,   168,   169,
     170,   171,   172,   173,   174,   175,   176,   177,   178,   179,
     180,   181,   182,   183,   364,   206,   251,   369,   -70,   196,
     329,   341,   -18,   186,   189,    14,   190,   191,   219,   187,
     104,   130,   330,   195,   331,   144,   144,   -90,   -90,   134,
     199,   200,   193,   -90,   135,   136,   340,   130,   132,   214,
     213,   133,   284,   218,   215,   216,   217,    25,   258,   259,
     260,   261,   262,    37,    25,    38,   220,    39,    40,    41,
      42,    43,    44,   225,   226,    25,    25,    25,   149,   114,
     115,    -2,   284,   253,   238,   325,   359,    25,   -18,    61,
     152,    25,   153,    37,   254,    38,    14,    39,    40,    41,
      42,    43,    44,   285,   286,   287,   229,   230,   362,    16,
     -91,   -91,   134,   144,   341,   370,   -91,   135,   136,   240,
     281,   255,   223,   -18,   224,   263,   264,   288,   332,   266,
     -18,   -18,    25,   125,   301,    25,   -18,   212,   294,   334,
     355,   150,   270,   -18,   271,   302,   117,   -18,   289,   321,
      14,    25,   274,   275,   276,   277,    18,   288,   113,    25,
     207,   102,   103,   282,   284,    25,   297,   298,   299,   122,
     318,   320,   303,   232,   322,    37,   255,    38,   -18,    39,
      40,    41,    42,    43,    44,   306,   154,   143,   102,   103,
    -103,   -18,   124,   312,   -69,   104,   194,   118,   316,   352,
      38,   119,    39,    40,    41,    25,   324,   326,   208,   273,
     186,    25,   207,   347,   192,   351,   187,   354,   348,    44,
     333,   143,   104,    25,  -106,   335,   198,   379,   242,   110,
     342,   235,   205,   343,    38,   256,    39,    40,    41,   288,
     102,   103,   267,   310,   278,   120,   121,   365,   280,  -103,
    -103,  -103,   110,   237,   300,  -103,  -103,  -103,   357,   227,
     208,   358,    25,   110,   360,   190,   319,   185,   361,    65,
      66,   110,   186,    67,   104,  -154,   363,  -154,   187,   366,
     186,   323,   368,  -106,  -106,  -106,   187,   110,   317,  -106,
    -106,  -106,   378,   339,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    92,   143,
      93,   327,  -102,    94,   110,   145,   110,   146,   143,   346,
      95,  -105,    38,   328,    39,    40,    41,   344,    25,   110,
     221,    38,   222,    39,    40,    41,    37,   377,    38,    25,
      39,    40,    41,    42,    43,   110,   243,   201,   350,   258,
     259,   260,   261,   262,   202,    43,   138,    25,    38,   268,
      39,    40,    41,   139,    65,    66,   110,    25,    67,   353,
     315,  -102,  -102,  -102,   246,   247,   248,  -102,  -102,  -102,
    -105,  -105,  -105,   349,   345,   376,  -105,  -105,  -105,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,   109,    93,   304,   264,    94,    65,
      66,    25,   257,    67,   269,    95,   305,   356,   249,    37,
     151,    38,   231,    39,    40,    41,    42,    43,    44,   295,
     296,   184,     0,    45,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    92,     0,
      93,     0,     0,    94,     1,     2,     3,     4,     5,     0,
      95,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
       0,    88,    89,    90,    37,     0,    38,     0,    39,    40,
      41,    42,    43,    38,     0,    39,    40,    41
};

#define yypact_value_is_default(yystate) \
  ((yystate) == (-191))

#define yytable_value_is_error(yytable_value) \
  YYID (0)

static const yytype_int16 yycheck[] =
{
       0,    32,    29,    50,    34,   182,   196,     7,     8,     9,
      10,    11,    29,    13,    14,    52,    16,    17,    18,   123,
      52,     4,    54,     4,     4,     4,    26,     1,     4,    55,
       4,     4,     4,     1,     4,    16,     0,    11,     4,    11,
      12,    41,    12,     1,    44,    25,    46,     4,     4,   226,
       4,    63,    35,     1,   244,     1,     4,    63,     4,     1,
      66,    61,    35,     4,    12,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    73,   125,   200,    73,    66,   116,
     290,    69,    68,    69,   104,    63,   106,   107,   140,    75,
      67,    69,    68,   113,    68,   141,   142,    63,    64,    65,
     120,   121,    64,    69,    70,    71,     1,    69,     1,   133,
     130,     4,     1,   137,   134,   135,   136,     4,     6,     7,
       8,     9,    10,    12,     4,    14,   146,    16,    17,    18,
      19,    20,    21,   153,   154,     4,     4,     4,     1,     4,
       5,     0,     1,   200,   194,     1,   343,     4,     4,    12,
      64,     4,    66,    12,   200,    14,    63,    16,    17,    18,
      19,    20,    21,    22,    23,    24,   186,   187,    63,    63,
      63,    64,    65,   219,    69,    64,    69,    70,    71,    66,
       1,   201,    64,     4,    66,    73,    74,    76,    68,   209,
      11,    12,     4,     1,   251,     4,     4,    64,   245,    68,
      68,    64,   222,    11,   224,   251,    20,    63,   245,    66,
      63,     4,   232,   233,   234,   235,    63,    76,    11,     4,
      11,    33,    34,   243,     1,     4,   246,   247,   248,    12,
     280,   281,   252,    12,   284,    12,   256,    14,    59,    16,
      17,    18,    19,    20,    21,   265,    16,     1,    33,    34,
       4,    59,    20,   273,    66,    67,     1,    12,   278,   326,
      14,    16,    16,    17,    18,     4,   286,   287,    59,    68,
      69,     4,    11,   323,    64,   325,    75,   327,    63,    21,
     300,     1,    67,     4,     4,   305,    12,    64,    26,    63,
     310,    12,    66,   313,    14,    12,    16,    17,    18,    76,
      33,    34,     4,    32,    35,    60,    61,   358,     1,    63,
      64,    65,    63,    64,    58,    69,    70,    71,   338,     1,
      59,   341,     4,    63,   344,   345,    66,    64,   348,    11,
      12,    63,    69,    15,    67,    64,   356,    66,    75,   359,
      69,     1,   362,    63,    64,    65,    75,    63,    64,    69,
      70,    71,   372,    66,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,     1,
      62,     1,     4,    65,    63,    64,    63,    66,     1,    66,
      72,     4,    14,     1,    16,    17,    18,    68,     4,    63,
      64,    14,    66,    16,    17,    18,    12,     1,    14,     4,
      16,    17,    18,    19,    20,    63,   197,    12,    66,     6,
       7,     8,     9,    10,    19,    20,    12,     4,    14,   211,
      16,    17,    18,    19,    11,    12,    63,     4,    15,    66,
     276,    63,    64,    65,    11,    12,    13,    69,    70,    71,
      63,    64,    65,   324,   316,   368,    69,    70,    71,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    33,    62,    73,    74,    65,    11,
      12,     4,   203,    15,   213,    72,   257,   335,    65,    12,
      62,    14,   188,    16,    17,    18,    19,    20,    21,   245,
     245,    96,    -1,    26,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    -1,
      62,    -1,    -1,    65,    27,    28,    29,    30,    31,    -1,
      72,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      -1,    56,    57,    58,    12,    -1,    14,    -1,    16,    17,
      18,    19,    20,    14,    -1,    16,    17,    18
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    27,    28,    29,    30,    31,    78,    80,    81,    82,
      83,    84,    86,    63,    63,   131,    63,     4,    63,     0,
      86,    86,    86,    86,    86,     4,    25,    88,    89,    86,
      86,    86,    86,    86,     1,    86,    87,    12,    14,    16,
      17,    18,    19,    20,    21,    26,    79,    91,    93,   116,
     117,   118,   119,   120,   122,   123,   124,   125,   127,   130,
       1,    12,   132,   133,   134,    11,    12,    15,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    62,    65,    72,   115,   136,   138,   139,
     140,   141,    33,    34,    67,   102,   103,   105,   106,   117,
      63,    66,   144,    11,     4,     5,    90,    20,    12,    16,
      60,    61,    12,    86,    20,     1,    86,    86,    86,     1,
      69,   131,     1,     4,    65,    70,    71,   114,    12,    19,
     122,   123,   123,     1,   124,    64,    66,   144,    86,     1,
      64,   133,    64,    66,    16,    86,    86,    86,    86,    86,
      86,    86,    86,    86,    86,    86,    86,    86,    86,    86,
      86,    86,    86,    86,    86,    86,    86,    86,    86,    86,
      86,    86,    86,    86,   139,    64,    69,    75,   137,    86,
      86,    86,    64,    64,     1,    86,    91,    92,    12,    86,
      86,    12,    19,   119,   126,    66,   144,    11,    59,    99,
      12,    95,    64,    86,   120,    86,    86,    86,   120,   123,
      86,    64,    66,    64,    66,    86,    86,     1,   136,    86,
      86,   138,    12,   100,    35,    12,   111,    64,   144,   145,
      66,    87,    26,    93,    94,    96,    11,    12,    13,    65,
      85,   119,   121,   122,   124,    86,    12,   126,     6,     7,
       8,     9,    10,    73,    74,   128,    86,     4,    99,   118,
      86,    86,   136,    68,    86,    86,    86,    86,    35,   104,
       1,     1,    86,    87,     1,    22,    23,    24,    76,    91,
      97,   109,   112,   113,   116,   142,   143,    86,    86,    86,
      58,   122,   124,    86,    73,   128,    86,   106,   107,   108,
      32,   135,    86,    16,   101,   102,    86,    64,   144,    66,
     144,    66,   144,     1,    86,     1,    86,     1,     1,    87,
      68,    68,    68,    86,    68,    86,    11,    12,   129,    66,
       1,    69,    86,    86,    68,   103,    66,   144,    63,   108,
      66,   144,   131,    66,   144,    68,   129,    86,    86,   136,
      86,    86,    63,    86,    73,   106,    86,   110,    86,    73,
      64,    91,    98,   116,   142,   143,   110,     1,    86,    64
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

#define YYBACKUP(Token, Value)                                  \
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


/* This macro is provided for backward compatibility. */

#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (&yylval, YYLEX_PARAM)
#else
# define YYLEX yylex (&yylval)
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
  FILE *yyo = yyoutput;
  YYUSE (yyo);
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
  YYSIZE_T yysize0 = yytnamerr (YY_NULL, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  YYSIZE_T yysize1;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULL;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - Assume YYFAIL is not used.  It's too flawed to consider.  See
       <http://lists.gnu.org/archive/html/bison-patches/2009-12/msg00024.html>
       for details.  YYERROR is fine as it does not invoke this
       function.
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
                yysize1 = yysize + yytnamerr (YY_NULL, yytname[yyx]);
                if (! (yysize <= yysize1
                       && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                  return 2;
                yysize = yysize1;
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  yysize1 = yysize + yystrlen (yyformat);
  if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
    return 2;
  yysize = yysize1;

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
      case 101: /* "maybe_media_value" */

	{ delete (yyvaluep->valueList); (yyvaluep->valueList) = 0; };

	break;
      case 103: /* "media_query_exp_list" */

	{ delete (yyvaluep->mediaQueryExpList); (yyvaluep->mediaQueryExpList) = 0; };

	break;
      case 104: /* "maybe_and_media_query_exp_list" */

	{ delete (yyvaluep->mediaQueryExpList); (yyvaluep->mediaQueryExpList) = 0; };

	break;
      case 107: /* "maybe_media_list" */

	{ delete (yyvaluep->mediaList); (yyvaluep->mediaList) = 0; };

	break;
      case 108: /* "media_list" */

	{ delete (yyvaluep->mediaList); (yyvaluep->mediaList) = 0; };

	break;
      case 110: /* "ruleset_list" */

	{ delete (yyvaluep->ruleList); (yyvaluep->ruleList) = 0; };

	break;
      case 117: /* "selector_list" */

	{ if ((yyvaluep->selectorList)) qDeleteAll(*(yyvaluep->selectorList)); delete (yyvaluep->selectorList); (yyvaluep->selectorList) = 0; };

	break;
      case 118: /* "selector" */

	{ delete (yyvaluep->selector); (yyvaluep->selector) = 0; };

	break;
      case 120: /* "simple_selector" */

	{ delete (yyvaluep->selector); (yyvaluep->selector) = 0; };

	break;
      case 121: /* "simple_css3_selector" */

	{ delete (yyvaluep->selector); (yyvaluep->selector) = 0; };

	break;
      case 123: /* "specifier_list" */

	{ delete (yyvaluep->selector); (yyvaluep->selector) = 0; };

	break;
      case 124: /* "specifier" */

	{ delete (yyvaluep->selector); (yyvaluep->selector) = 0; };

	break;
      case 125: /* "class" */

	{ delete (yyvaluep->selector); (yyvaluep->selector) = 0; };

	break;
      case 127: /* "attrib" */

	{ delete (yyvaluep->selector); (yyvaluep->selector) = 0; };

	break;
      case 130: /* "pseudo" */

	{ delete (yyvaluep->selector); (yyvaluep->selector) = 0; };

	break;
      case 136: /* "expr" */

	{ delete (yyvaluep->valueList); (yyvaluep->valueList) = 0; };

	break;
      case 140: /* "function" */

	{ if ((yyvaluep->value).function) delete (yyvaluep->value).function->args; delete (yyvaluep->value).function; (yyvaluep->value).function = 0; };

	break;

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


/*----------.
| yyparse.  |
`----------*/

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
/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

    /* Number of syntax errors so far.  */
    int yynerrs;

    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

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
  if (yypact_value_is_default (yyn))
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
        case 11:

    {
        CSSParser *p = static_cast<CSSParser *>(parser);
	p->rule = (yyvsp[(4) - (6)].rule);
    }
    break;

  case 12:

    {
	/* can be empty */
    }
    break;

  case 13:

    {
	CSSParser *p = static_cast<CSSParser *>(parser);
	if ( (yyvsp[(4) - (5)].valueList) ) {
	    p->valueList = (yyvsp[(4) - (5)].valueList);
#ifdef CSS_DEBUG
	    qDebug() << "   got property for " << p->id <<
		(p->important?" important":"");
	    bool ok =
#endif
		p->parseValue( p->id, p->important );
#ifdef CSS_DEBUG
	    if ( !ok )
		qDebug() << "     couldn't parse value!";
#endif
	}
#ifdef CSS_DEBUG
	else
	    qDebug() << "     no value found!";
#endif
	delete p->valueList;
	p->valueList = 0;
    }
    break;

  case 14:

    {
		CSSParser *p = static_cast<CSSParser *>(parser);
		if ((yyvsp[(4) - (5)].selectorList)) {
			p->selectors = *(yyvsp[(4) - (5)].selectorList);
			delete (yyvsp[(4) - (5)].selectorList);
			(yyvsp[(4) - (5)].selectorList) = 0;
		} else
			p->selectors.clear(); // parse error
	}
    break;

  case 15:

    {
         CSSParser *p = static_cast<CSSParser *>(parser);
         p->mediaQuery = (yyvsp[(4) - (5)].mediaQuery);
     }
    break;

  case 25:

    {
#ifdef CSS_DEBUG
     qDebug() << "charset rule: " << qString((yyvsp[(3) - (5)].string));
#endif
     CSSParser* p = static_cast<CSSParser*>(parser);
     if (p->styleElement && p->styleElement->isCSSStyleSheet()) {
         p->styleElement->append( new CSSCharsetRuleImpl(p->styleElement, domString((yyvsp[(3) - (5)].string))) );
     }
 }
    break;

  case 26:

    {
 }
    break;

  case 27:

    {
 }
    break;

  case 29:

    {
     CSSParser *p = static_cast<CSSParser *>(parser);
     if ( (yyvsp[(2) - (3)].rule) && p->styleElement && p->styleElement->isCSSStyleSheet() ) {
	 p->styleElement->append( (yyvsp[(2) - (3)].rule) );
     } else {
	 delete (yyvsp[(2) - (3)].rule);
     }
 }
    break;

  case 30:

    {
#ifdef CSS_DEBUG
	qDebug() << "@import: " << qString((yyvsp[(3) - (6)].string));
#endif
	CSSParser *p = static_cast<CSSParser *>(parser);
	if ( (yyvsp[(5) - (6)].mediaList) && p->styleElement && p->styleElement->isCSSStyleSheet() )
	    (yyval.rule) = new CSSImportRuleImpl( p->styleElement, domString((yyvsp[(3) - (6)].string)), (yyvsp[(5) - (6)].mediaList) );
	else
	    (yyval.rule) = 0;
    }
    break;

  case 31:

    {
        (yyval.rule) = 0;
    }
    break;

  case 32:

    {
        (yyval.rule) = 0;
    }
    break;

  case 35:

    {
#ifdef CSS_DEBUG
    qDebug() << "@namespace: " << qString((yyvsp[(3) - (4)].string)) << qString((yyvsp[(4) - (4)].string));
#endif
    CSSParser *p = static_cast<CSSParser *>(parser);
    (yyval.rule) = new CSSNamespaceRuleImpl(p->styleElement, domString((yyvsp[(3) - (4)].string)), domString((yyvsp[(4) - (4)].string)));
 }
    break;

  case 36:

    {
    CSSParser *p = static_cast<CSSParser *>(parser);
    if (p->styleElement && p->styleElement->isCSSStyleSheet())
	static_cast<CSSStyleSheetImpl*>(p->styleElement)->appendNamespaceRule
                    (static_cast<CSSNamespaceRuleImpl*>((yyvsp[(1) - (3)].rule))); 
                            // can't use ->append since it 
                            // wouldn't keep track of dirtiness
 }
    break;

  case 39:

    { (yyval.string).string = 0; }
    break;

  case 40:

    { (yyval.string) = (yyvsp[(1) - (2)].string); }
    break;

  case 42:

    {
     CSSParser *p = static_cast<CSSParser *>(parser);
     if ( (yyvsp[(2) - (3)].rule) && p->styleElement && p->styleElement->isCSSStyleSheet() ) {
	 p->styleElement->append( (yyvsp[(2) - (3)].rule) );
     } else {
	 delete (yyvsp[(2) - (3)].rule);
     }
 }
    break;

  case 49:

    { delete (yyvsp[(1) - (2)].rule); (yyval.rule) = 0; }
    break;

  case 53:

    { delete (yyvsp[(1) - (2)].rule); (yyval.rule) = 0; }
    break;

  case 56:

    {
        (yyval.string) = (yyvsp[(1) - (2)].string);
    }
    break;

  case 57:

    {
        (yyval.valueList) = 0;
    }
    break;

  case 58:

    {
        (yyval.valueList) = (yyvsp[(3) - (4)].valueList);
    }
    break;

  case 59:

    {
        (yyval.mediaQueryExp) = new khtml::MediaQueryExp(domString((yyvsp[(3) - (7)].string)).lower(), (yyvsp[(5) - (7)].valueList));
    }
    break;

  case 60:

    {
      (yyval.mediaQueryExpList) =  new QList<khtml::MediaQueryExp*>;
      (yyval.mediaQueryExpList)->append((yyvsp[(1) - (1)].mediaQueryExp));
    }
    break;

  case 61:

    {
      (yyval.mediaQueryExpList) = (yyvsp[(1) - (5)].mediaQueryExpList);
      (yyval.mediaQueryExpList)->append((yyvsp[(5) - (5)].mediaQueryExp));
    }
    break;

  case 62:

    {
        (yyval.mediaQueryExpList) = new QList<khtml::MediaQueryExp*>;
    }
    break;

  case 63:

    {
        (yyval.mediaQueryExpList) = (yyvsp[(3) - (3)].mediaQueryExpList);
    }
    break;

  case 64:

    {
        (yyval.mediaQueryRestrictor) = khtml::MediaQuery::None;
    }
    break;

  case 65:

    {
        (yyval.mediaQueryRestrictor) = khtml::MediaQuery::Only;
    }
    break;

  case 66:

    {
        (yyval.mediaQueryRestrictor) = khtml::MediaQuery::Not;
    }
    break;

  case 67:

    {
        (yyval.mediaQuery) = new khtml::MediaQuery(khtml::MediaQuery::None, "all", (yyvsp[(1) - (1)].mediaQueryExpList));
    }
    break;

  case 68:

    {
        (yyval.mediaQuery) = new khtml::MediaQuery((yyvsp[(1) - (4)].mediaQueryRestrictor), domString((yyvsp[(3) - (4)].string)).lower(), (yyvsp[(4) - (4)].mediaQueryExpList));
    }
    break;

  case 69:

    {
	(yyval.mediaList) = new MediaListImpl();
    }
    break;

  case 71:

    {
        (yyval.mediaList) = new MediaListImpl();
        (yyval.mediaList)->appendMediaQuery((yyvsp[(1) - (1)].mediaQuery));
    }
    break;

  case 72:

    {
	(yyval.mediaList) = (yyvsp[(1) - (4)].mediaList);
	if ((yyval.mediaList))
	    (yyval.mediaList)->appendMediaQuery( (yyvsp[(4) - (4)].mediaQuery) );
    }
    break;

  case 73:

    {
       delete (yyvsp[(1) - (2)].mediaList);
       (yyval.mediaList) = 0;
    }
    break;

  case 74:

    {
	CSSParser *p = static_cast<CSSParser *>(parser);
	if ( (yyvsp[(3) - (7)].mediaList) && (yyvsp[(6) - (7)].ruleList) &&
	     p->styleElement && p->styleElement->isCSSStyleSheet() ) {
	    (yyval.rule) = new CSSMediaRuleImpl( static_cast<CSSStyleSheetImpl*>(p->styleElement), (yyvsp[(3) - (7)].mediaList), (yyvsp[(6) - (7)].ruleList) );
	} else {
	    (yyval.rule) = 0;
	    delete (yyvsp[(3) - (7)].mediaList);
	    delete (yyvsp[(6) - (7)].ruleList);
	}
    }
    break;

  case 75:

    {
        CSSParser *p = static_cast<CSSParser *>(parser);
        if ((yyvsp[(5) - (6)].ruleList) && p->styleElement && p->styleElement->isCSSStyleSheet() ) {
            (yyval.rule) = new CSSMediaRuleImpl( static_cast<CSSStyleSheetImpl*>(p->styleElement), 0, (yyvsp[(5) - (6)].ruleList));
        } else {
            (yyval.rule) = 0;
            delete (yyvsp[(5) - (6)].ruleList);
        }
    }
    break;

  case 76:

    { (yyval.ruleList) = 0; }
    break;

  case 77:

    {
      (yyval.ruleList) = (yyvsp[(1) - (3)].ruleList);
      if ( (yyvsp[(2) - (3)].rule) ) {
	  if ( !(yyval.ruleList) ) (yyval.ruleList) = new CSSRuleListImpl();
	  (yyval.ruleList)->append( (yyvsp[(2) - (3)].rule) );
      }
  }
    break;

  case 78:

    {
      (yyval.string) = (yyvsp[(1) - (2)].string);
  }
    break;

  case 79:

    {
      (yyval.rule) = 0;
    }
    break;

  case 80:

    {
      (yyval.rule) = 0;
    }
    break;

  case 81:

    {
      CSSParser *p = static_cast<CSSParser *>(parser);
      CSSFontFaceRuleImpl *rule = new CSSFontFaceRuleImpl( p->styleElement );
      CSSStyleDeclarationImpl *decl = p->createFontFaceStyleDeclaration( rule );
      rule->setDeclaration(decl);
      (yyval.rule) = rule;
    }
    break;

  case 82:

    {
      (yyval.rule) = 0;
    }
    break;

  case 83:

    {
      (yyval.rule) = 0;
    }
    break;

  case 84:

    { (yyval.relation) = CSSSelector::DirectAdjacent; }
    break;

  case 85:

    { (yyval.relation) = CSSSelector::IndirectAdjacent; }
    break;

  case 86:

    { (yyval.relation) = CSSSelector::Child; }
    break;

  case 87:

    { (yyval.val) = -1; }
    break;

  case 88:

    { (yyval.val) = 1; }
    break;

  case 89:

    {
#ifdef CSS_DEBUG
	qDebug() << "got ruleset" << endl << "  selector:";
#endif
	CSSParser *p = static_cast<CSSParser *>(parser);
	if ( (yyvsp[(1) - (2)].selectorList)  ) {
	    CSSStyleRuleImpl *rule = new CSSStyleRuleImpl( p->styleElement );
	    CSSStyleDeclarationImpl *decl = p->createStyleDeclaration( rule );
	    rule->setSelector( (yyvsp[(1) - (2)].selectorList) );
	    rule->setDeclaration(decl);
	    (yyval.rule) = rule;
	} else {
	    (yyval.rule) = 0;
	    if ((yyvsp[(1) - (2)].selectorList)) qDeleteAll(*(yyvsp[(1) - (2)].selectorList));
	    delete (yyvsp[(1) - (2)].selectorList);
	    (yyvsp[(1) - (2)].selectorList) = 0;
	    p->clearProperties();
	}
    }
    break;

  case 90:

    {
	if ( (yyvsp[(1) - (1)].selector) ) {
	    (yyval.selectorList) = new QList<CSSSelector*>;
#ifdef CSS_DEBUG
	    qDebug() << "   got simple selector:";
	    (yyvsp[(1) - (1)].selector)->print();
#endif
	    (yyval.selectorList)->append( (yyvsp[(1) - (1)].selector) );
	    khtml::CSSStyleSelector::precomputeAttributeDependencies(static_cast<CSSParser *>(parser)->document(), (yyvsp[(1) - (1)].selector));
	} else {
	    (yyval.selectorList) = 0;
	}
    }
    break;

  case 91:

    {
	if ( (yyvsp[(1) - (4)].selectorList) && (yyvsp[(4) - (4)].selector) ) {
	    (yyval.selectorList) = (yyvsp[(1) - (4)].selectorList);
	    (yyval.selectorList)->append( (yyvsp[(4) - (4)].selector) );
	    khtml::CSSStyleSelector::precomputeAttributeDependencies(static_cast<CSSParser *>(parser)->document(), (yyvsp[(4) - (4)].selector));
#ifdef CSS_DEBUG
	    qDebug() << "   got simple selector:";
	    (yyvsp[(4) - (4)].selector)->print();
#endif
	} else {
            if ((yyvsp[(1) - (4)].selectorList)) qDeleteAll(*(yyvsp[(1) - (4)].selectorList));
	    delete (yyvsp[(1) - (4)].selectorList);
	    (yyvsp[(1) - (4)].selectorList)=0;

	    delete (yyvsp[(4) - (4)].selector);
	    (yyval.selectorList) = 0;
	}
    }
    break;

  case 92:

    {
        if ((yyvsp[(1) - (2)].selectorList)) qDeleteAll(*(yyvsp[(1) - (2)].selectorList));
	delete (yyvsp[(1) - (2)].selectorList);
	(yyvsp[(1) - (2)].selectorList) = 0;
	(yyval.selectorList) = 0;
    }
    break;

  case 93:

    {
	(yyval.selector) = (yyvsp[(1) - (1)].selector);
    }
    break;

  case 94:

    {
        (yyval.selector) = (yyvsp[(1) - (2)].selector);
    }
    break;

  case 95:

    {
        if ( !(yyvsp[(1) - (3)].selector) || !(yyvsp[(3) - (3)].selector) ) {
	    delete (yyvsp[(1) - (3)].selector);
	    delete (yyvsp[(3) - (3)].selector);
	    (yyval.selector) = 0;
        } else {
            (yyval.selector) = (yyvsp[(3) - (3)].selector);
            CSSSelector *end = (yyval.selector);
            while( end->tagHistory )
                end = end->tagHistory;
            end->relation = CSSSelector::Descendant;
            end->tagHistory = (yyvsp[(1) - (3)].selector);
        }
    }
    break;

  case 96:

    {
	if ( !(yyvsp[(1) - (3)].selector) || !(yyvsp[(3) - (3)].selector) ) {
	    delete (yyvsp[(1) - (3)].selector);
	    delete (yyvsp[(3) - (3)].selector);
	    (yyval.selector) = 0;
	} else {
	    (yyval.selector) = (yyvsp[(3) - (3)].selector);
	    CSSSelector *end = (yyvsp[(3) - (3)].selector);
	    while( end->tagHistory )
		end = end->tagHistory;
	    end->relation = (yyvsp[(2) - (3)].relation);
	    end->tagHistory = (yyvsp[(1) - (3)].selector);
	}
    }
    break;

  case 97:

    {
	delete (yyvsp[(1) - (2)].selector);
	(yyval.selector) = 0;
    }
    break;

  case 98:

    { (yyval.string).string = 0; (yyval.string).length = 0; }
    break;

  case 99:

    { static unsigned short star = '*'; (yyval.string).string = &star; (yyval.string).length = 1; }
    break;

  case 100:

    { (yyval.string) = (yyvsp[(1) - (2)].string); }
    break;

  case 101:

    {
	(yyval.selector) = new CSSSelector();
        (yyval.selector)->tagLocalName = LocalName::fromId(localNamePart((yyvsp[(1) - (1)].element)));
        (yyval.selector)->tagNamespace = NamespaceName::fromId(namespacePart((yyvsp[(1) - (1)].element)));
    }
    break;

  case 102:

    {
	(yyval.selector) = (yyvsp[(2) - (2)].selector);
        if ( (yyval.selector) ) {
            (yyval.selector)->tagLocalName = LocalName::fromId(localNamePart((yyvsp[(1) - (2)].element)));
            (yyval.selector)->tagNamespace = NamespaceName::fromId(namespacePart((yyvsp[(1) - (2)].element)));
        }
    }
    break;

  case 103:

    {
	(yyval.selector) = (yyvsp[(1) - (1)].selector);
        if ( (yyval.selector) ) {
            (yyval.selector)->tagLocalName = LocalName::fromId(anyLocalName);
            (yyval.selector)->tagNamespace = NamespaceName::fromId(static_cast<CSSParser*>(parser)->defaultNamespace());
        }
    }
    break;

  case 104:

    {
        (yyval.selector) = new CSSSelector();
        (yyval.selector)->tagLocalName = LocalName::fromId(localNamePart((yyvsp[(2) - (2)].element)));
        (yyval.selector)->tagNamespace = NamespaceName::fromId(namespacePart((yyvsp[(2) - (2)].element)));
	CSSParser *p = static_cast<CSSParser *>(parser);
        if (p->styleElement && p->styleElement->isCSSStyleSheet())
            static_cast<CSSStyleSheetImpl*>(p->styleElement)->determineNamespace((yyval.selector)->tagNamespace, domString((yyvsp[(1) - (2)].string)));
    }
    break;

  case 105:

    {
        (yyval.selector) = (yyvsp[(3) - (3)].selector);
        if ((yyval.selector)) {
            (yyval.selector)->tagLocalName = LocalName::fromId(localNamePart((yyvsp[(2) - (3)].element)));
            (yyval.selector)->tagNamespace = NamespaceName::fromId(namespacePart((yyvsp[(2) - (3)].element)));
            CSSParser *p = static_cast<CSSParser *>(parser);
            if (p->styleElement && p->styleElement->isCSSStyleSheet())
                static_cast<CSSStyleSheetImpl*>(p->styleElement)->determineNamespace((yyval.selector)->tagNamespace, domString((yyvsp[(1) - (3)].string)));
        }
    }
    break;

  case 106:

    {
        (yyval.selector) = (yyvsp[(2) - (2)].selector);
        if ((yyval.selector)) {
            (yyval.selector)->tagLocalName = LocalName::fromId(anyLocalName);
            (yyval.selector)->tagNamespace = NamespaceName::fromId(anyNamespace);
            CSSParser *p = static_cast<CSSParser *>(parser);
            if (p->styleElement && p->styleElement->isCSSStyleSheet())
                static_cast<CSSStyleSheetImpl*>(p->styleElement)->determineNamespace((yyval.selector)->tagNamespace, domString((yyvsp[(1) - (2)].string)));
        }
    }
    break;

  case 107:

    {
	(yyval.selector) = new CSSSelector();
        (yyval.selector)->tagLocalName = LocalName::fromId(localNamePart((yyvsp[(1) - (1)].element)));
        (yyval.selector)->tagNamespace = NamespaceName::fromId(namespacePart((yyvsp[(1) - (1)].element)));
    }
    break;

  case 108:

    {
	(yyval.selector) = (yyvsp[(1) - (1)].selector);
        if ( (yyval.selector) ) {
            (yyval.selector)->tagLocalName = LocalName::fromId(anyLocalName);
            (yyval.selector)->tagNamespace = NamespaceName::fromId(static_cast<CSSParser*>(parser)->defaultNamespace());
        }
    }
    break;

  case 109:

    {
        (yyval.selector) = new CSSSelector();
        (yyval.selector)->tagLocalName = LocalName::fromId(localNamePart((yyvsp[(2) - (2)].element)));
        (yyval.selector)->tagNamespace = NamespaceName::fromId(namespacePart((yyvsp[(2) - (2)].element)));
	CSSParser *p = static_cast<CSSParser *>(parser);
        if (p->styleElement && p->styleElement->isCSSStyleSheet())
            static_cast<CSSStyleSheetImpl*>(p->styleElement)->determineNamespace((yyval.selector)->tagNamespace, domString((yyvsp[(1) - (2)].string)));
    }
    break;

  case 110:

    {
        (yyval.selector) = (yyvsp[(2) - (2)].selector);
        if ((yyval.selector)) {
            (yyval.selector)->tagLocalName = LocalName::fromId(anyLocalName);
            (yyval.selector)->tagNamespace = NamespaceName::fromId(anyNamespace);
            CSSParser *p = static_cast<CSSParser *>(parser);
            if (p->styleElement && p->styleElement->isCSSStyleSheet())
                static_cast<CSSStyleSheetImpl*>(p->styleElement)->determineNamespace((yyval.selector)->tagNamespace, domString((yyvsp[(1) - (2)].string)));
        }
    }
    break;

  case 111:

    {
      CSSParser *p = static_cast<CSSParser *>(parser);
      (yyval.element) = makeId(p->defaultNamespace(), p->getLocalNameId(domString((yyvsp[(1) - (1)].string))));
    }
    break;

  case 112:

    {
	(yyval.element) = makeId(static_cast<CSSParser*>(parser)->defaultNamespace(), anyLocalName);
    }
    break;

  case 113:

    {
	(yyval.selector) = (yyvsp[(1) - (1)].selector);
    }
    break;

  case 114:

    {
	(yyval.selector) = (yyvsp[(1) - (2)].selector);
	if ( (yyval.selector) ) {
            CSSSelector *end = (yyvsp[(1) - (2)].selector);
            while( end->tagHistory )
                end = end->tagHistory;
            end->relation = CSSSelector::SubSelector;
            end->tagHistory = (yyvsp[(2) - (2)].selector);
	}
    }
    break;

  case 115:

    {
	delete (yyvsp[(1) - (2)].selector);
	(yyval.selector) = 0;
    }
    break;

  case 116:

    {
        CSSParser *p = static_cast<CSSParser *>(parser);

	(yyval.selector) = new CSSSelector();
	(yyval.selector)->match = CSSSelector::Id;
        (yyval.selector)->attrLocalName = LocalName::fromId(localNamePart(ATTR_ID));
        (yyval.selector)->attrNamespace = NamespaceName::fromId(namespacePart(ATTR_ID));

        bool caseSensitive = p->document()->htmlMode() == DocumentImpl::XHtml || !p->document()->inCompatMode();
        if (caseSensitive)
            (yyval.selector)->value = domString((yyvsp[(1) - (1)].string));
        else
            (yyval.selector)->value = domString((yyvsp[(1) - (1)].string)).lower();
    }
    break;

  case 120:

    {
        CSSParser *p = static_cast<CSSParser *>(parser);

	(yyval.selector) = new CSSSelector();
	(yyval.selector)->match = CSSSelector::Class;
        (yyval.selector)->attrLocalName = LocalName::fromId(localNamePart(ATTR_CLASS));
        (yyval.selector)->attrNamespace = NamespaceName::fromId(namespacePart(ATTR_CLASS));

        bool caseSensitive = p->document()->htmlMode() == DocumentImpl::XHtml || !p->document()->inCompatMode();
        if (caseSensitive)
            (yyval.selector)->value = domString((yyvsp[(2) - (2)].string));
        else
            (yyval.selector)->value = domString((yyvsp[(2) - (2)].string)).lower();
    }
    break;

  case 121:

    {
      CSSParser *p = static_cast<CSSParser *>(parser);
      (yyval.attribute) = makeId(emptyNamespace, p->getLocalNameId(domString((yyvsp[(1) - (2)].string))));
    }
    break;

  case 122:

    {
	(yyval.selector) = new CSSSelector();
        (yyval.selector)->attrLocalName = LocalName::fromId(localNamePart((yyvsp[(3) - (4)].attribute)));
        (yyval.selector)->attrNamespace = NamespaceName::fromId(namespacePart((yyvsp[(3) - (4)].attribute)));
	(yyval.selector)->match = CSSSelector::Set;
    }
    break;

  case 123:

    {
	(yyval.selector) = new CSSSelector();
        (yyval.selector)->attrLocalName = LocalName::fromId(localNamePart((yyvsp[(3) - (8)].attribute)));
        (yyval.selector)->attrNamespace = NamespaceName::fromId(namespacePart((yyvsp[(3) - (8)].attribute)));
	(yyval.selector)->match = (yyvsp[(4) - (8)].match);
	(yyval.selector)->value = domString((yyvsp[(6) - (8)].string));
    }
    break;

  case 124:

    {
        (yyval.selector) = new CSSSelector();
        (yyval.selector)->attrLocalName = LocalName::fromId(localNamePart((yyvsp[(4) - (5)].attribute)));
        (yyval.selector)->attrNamespace = NamespaceName::fromId(namespacePart((yyvsp[(4) - (5)].attribute)));
        (yyval.selector)->match = CSSSelector::Set;
        CSSParser *p = static_cast<CSSParser *>(parser);
        if (p->styleElement && p->styleElement->isCSSStyleSheet())
            static_cast<CSSStyleSheetImpl*>(p->styleElement)->determineNamespace((yyval.selector)->attrNamespace, domString((yyvsp[(3) - (5)].string)));
    }
    break;

  case 125:

    {
        (yyval.selector) = new CSSSelector();
        (yyval.selector)->attrLocalName = LocalName::fromId(localNamePart((yyvsp[(4) - (9)].attribute)));
        (yyval.selector)->attrNamespace = NamespaceName::fromId(namespacePart((yyvsp[(4) - (9)].attribute)));
        (yyval.selector)->match = (CSSSelector::Match)(yyvsp[(5) - (9)].match);
        (yyval.selector)->value = domString((yyvsp[(7) - (9)].string));
        CSSParser *p = static_cast<CSSParser *>(parser);
        if (p->styleElement && p->styleElement->isCSSStyleSheet())
            static_cast<CSSStyleSheetImpl*>(p->styleElement)->determineNamespace((yyval.selector)->attrNamespace, domString((yyvsp[(3) - (9)].string)));
   }
    break;

  case 126:

    {
	(yyval.match) = CSSSelector::Exact;
    }
    break;

  case 127:

    {
	(yyval.match) = CSSSelector::List;
    }
    break;

  case 128:

    {
	(yyval.match) = CSSSelector::Hyphen;
    }
    break;

  case 129:

    {
	(yyval.match) = CSSSelector::Begin;
    }
    break;

  case 130:

    {
	(yyval.match) = CSSSelector::End;
    }
    break;

  case 131:

    {
	(yyval.match) = CSSSelector::Contain;
    }
    break;

  case 134:

    {
	(yyval.selector) = new CSSSelector();
	(yyval.selector)->match = CSSSelector::PseudoClass;
	(yyval.selector)->value = domString((yyvsp[(2) - (2)].string));
    }
    break;

  case 135:

    {
	(yyval.selector) = new CSSSelector();
	(yyval.selector)->match = CSSSelector::PseudoElement;
        (yyval.selector)->value = domString((yyvsp[(3) - (3)].string));
    }
    break;

  case 136:

    {
        (yyval.selector) = new CSSSelector();
        (yyval.selector)->match = CSSSelector::PseudoClass;
        (yyval.selector)->string_arg = domString((yyvsp[(4) - (6)].string));
        (yyval.selector)->value = domString((yyvsp[(2) - (6)].string));
    }
    break;

  case 137:

    {
        (yyval.selector) = new CSSSelector();
        (yyval.selector)->match = CSSSelector::PseudoClass;
        (yyval.selector)->string_arg = QString::number((yyvsp[(5) - (7)].val));
        (yyval.selector)->value = domString((yyvsp[(2) - (7)].string));
    }
    break;

  case 138:

    {
        (yyval.selector) = new CSSSelector();
        (yyval.selector)->match = CSSSelector::PseudoClass;
        (yyval.selector)->string_arg = domString((yyvsp[(4) - (6)].string));
        (yyval.selector)->value = domString((yyvsp[(2) - (6)].string));
    }
    break;

  case 139:

    {
        (yyval.selector) = new CSSSelector();
        (yyval.selector)->match = CSSSelector::PseudoClass;
        (yyval.selector)->string_arg = domString((yyvsp[(4) - (6)].string));
        (yyval.selector)->value = domString((yyvsp[(2) - (6)].string));
    }
    break;

  case 140:

    {
        (yyval.selector) = new CSSSelector();
        (yyval.selector)->match = CSSSelector::PseudoClass;
        (yyval.selector)->simpleSelector = (yyvsp[(4) - (6)].selector);
        (yyval.selector)->value = domString((yyvsp[(2) - (6)].string));
    }
    break;

  case 141:

    {
	(yyval.ok) = (yyvsp[(3) - (4)].ok);
    }
    break;

  case 142:

    {
	(yyval.ok) = false;
    }
    break;

  case 143:

    {
	(yyval.ok) = (yyvsp[(3) - (4)].ok);
    }
    break;

  case 144:

    {
	(yyval.ok) = (yyvsp[(3) - (5)].ok);
	if ( (yyvsp[(4) - (5)].ok) )
	    (yyval.ok) = (yyvsp[(4) - (5)].ok);
    }
    break;

  case 145:

    {
	(yyval.ok) = (yyvsp[(3) - (5)].ok);
    }
    break;

  case 146:

    {
	(yyval.ok) = (yyvsp[(1) - (3)].ok);
    }
    break;

  case 147:

    {
        (yyval.ok) = false;
    }
    break;

  case 148:

    {
	(yyval.ok) = (yyvsp[(1) - (4)].ok);
	if ( (yyvsp[(2) - (4)].ok) )
	    (yyval.ok) = (yyvsp[(2) - (4)].ok);
    }
    break;

  case 149:

    {
        (yyval.ok) = (yyvsp[(1) - (4)].ok);
    }
    break;

  case 150:

    {
	(yyval.ok) = false;
	CSSParser *p = static_cast<CSSParser *>(parser);
	if ( (yyvsp[(1) - (5)].prop_id) && (yyvsp[(4) - (5)].valueList) ) {
	    p->valueList = (yyvsp[(4) - (5)].valueList);
#ifdef CSS_DEBUG
	    qDebug() << "   got property: " << (yyvsp[(1) - (5)].prop_id) <<
		((yyvsp[(5) - (5)].b)?" important":"");
#endif
	        bool ok = p->parseValue( (yyvsp[(1) - (5)].prop_id), (yyvsp[(5) - (5)].b) );
                if ( ok )
		    (yyval.ok) = ok;
#ifdef CSS_DEBUG
	        else
		    qDebug() << "     couldn't parse value!";
#endif
	} else {
            delete (yyvsp[(4) - (5)].valueList);
        }
	delete p->valueList;
	p->valueList = 0;
    }
    break;

  case 151:

    {
        (yyval.ok) = false;
    }
    break;

  case 152:

    {
	QString str = qString((yyvsp[(1) - (2)].string));
	str = str.toLower();
	if (str.length() >= sMinCompatPropLen && str[0] == '-' && str[1] != 'k') {
	    // vendor extension. Lets try and convert a selected few
	    if (!sCompatibleProperties)
	        initCompatibleProperties();
            QHash<QString,int>::iterator it = sCompatibleProperties->find( str );
            if (it != sCompatibleProperties->end()) {
                str = "-khtml" + str.mid( it.value() );
              
                (yyval.prop_id) = getPropertyID( str.toLatin1(), str.length() );
            } else {
                (yyval.prop_id) = 0;
            }
	} else {
	    (yyval.prop_id) = getPropertyID( str.toLatin1(), str.length() );
        }
    }
    break;

  case 153:

    { (yyval.b) = true; }
    break;

  case 154:

    { (yyval.b) = false; }
    break;

  case 155:

    {
	(yyval.valueList) = new ValueList;
	(yyval.valueList)->addValue( (yyvsp[(1) - (1)].value) );
    }
    break;

  case 156:

    {
	(yyval.valueList) = (yyvsp[(1) - (3)].valueList);
	if ( (yyval.valueList) ) {
	    if ( (yyvsp[(2) - (3)].tok) ) {
		Value v;
		v.id = 0;
		v.unit = Value::Operator;
		v.iValue = (yyvsp[(2) - (3)].tok);
		(yyval.valueList)->addValue( v );
	    }
	    (yyval.valueList)->addValue( (yyvsp[(3) - (3)].value) );
	}
    }
    break;

  case 157:

    {
	(yyval.tok) = '/';
    }
    break;

  case 158:

    {
	(yyval.tok) = ',';
    }
    break;

  case 159:

    {
        (yyval.tok) = 0;
  }
    break;

  case 160:

    { (yyval.value) = (yyvsp[(1) - (1)].value); }
    break;

  case 161:

    { (yyval.value) = (yyvsp[(2) - (2)].value); (yyval.value).fValue *= (yyvsp[(1) - (2)].val); }
    break;

  case 162:

    { (yyval.value).id = 0; (yyval.value).string = (yyvsp[(1) - (2)].string); (yyval.value).unit = CSSPrimitiveValue::CSS_DIMENSION; }
    break;

  case 163:

    { (yyval.value).id = 0; (yyval.value).string = (yyvsp[(1) - (2)].string); (yyval.value).unit = CSSPrimitiveValue::CSS_STRING; }
    break;

  case 164:

    {
      QString str = qString( (yyvsp[(1) - (2)].string) );
      (yyval.value).id = getValueID( str.toLower().toLatin1(), str.length() );
      (yyval.value).unit = CSSPrimitiveValue::CSS_IDENT;
      (yyval.value).string = (yyvsp[(1) - (2)].string);
  }
    break;

  case 165:

    { (yyval.value).id = 0; (yyval.value).string = (yyvsp[(1) - (2)].string); (yyval.value).unit = CSSPrimitiveValue::CSS_URI; }
    break;

  case 166:

    { (yyval.value).id = 0; (yyval.value).iValue = 0; (yyval.value).unit = CSSPrimitiveValue::CSS_UNKNOWN;/* ### */ }
    break;

  case 167:

    { (yyval.value).id = 0; (yyval.value).string = (yyvsp[(1) - (1)].string); (yyval.value).unit = CSSPrimitiveValue::CSS_RGBCOLOR; }
    break;

  case 168:

    {
      (yyval.value) = (yyvsp[(1) - (1)].value);
  }
    break;

  case 169:

    { (yyval.value).id = 0; (yyval.value).isInt = true; (yyval.value).fValue = (yyvsp[(1) - (2)].val); (yyval.value).unit = CSSPrimitiveValue::CSS_NUMBER; }
    break;

  case 170:

    { (yyval.value).id = 0; (yyval.value).isInt = false; (yyval.value).fValue = (yyvsp[(1) - (2)].val); (yyval.value).unit = CSSPrimitiveValue::CSS_NUMBER; }
    break;

  case 171:

    { (yyval.value).id = 0; (yyval.value).fValue = (yyvsp[(1) - (2)].val); (yyval.value).unit = CSSPrimitiveValue::CSS_PERCENTAGE; }
    break;

  case 172:

    { (yyval.value).id = 0; (yyval.value).fValue = (yyvsp[(1) - (2)].val); (yyval.value).unit = CSSPrimitiveValue::CSS_PX; }
    break;

  case 173:

    { (yyval.value).id = 0; (yyval.value).fValue = (yyvsp[(1) - (2)].val); (yyval.value).unit = CSSPrimitiveValue::CSS_CM; }
    break;

  case 174:

    { (yyval.value).id = 0; (yyval.value).fValue = (yyvsp[(1) - (2)].val); (yyval.value).unit = CSSPrimitiveValue::CSS_MM; }
    break;

  case 175:

    { (yyval.value).id = 0; (yyval.value).fValue = (yyvsp[(1) - (2)].val); (yyval.value).unit = CSSPrimitiveValue::CSS_IN; }
    break;

  case 176:

    { (yyval.value).id = 0; (yyval.value).fValue = (yyvsp[(1) - (2)].val); (yyval.value).unit = CSSPrimitiveValue::CSS_PT; }
    break;

  case 177:

    { (yyval.value).id = 0; (yyval.value).fValue = (yyvsp[(1) - (2)].val); (yyval.value).unit = CSSPrimitiveValue::CSS_PC; }
    break;

  case 178:

    { (yyval.value).id = 0; (yyval.value).fValue = (yyvsp[(1) - (2)].val); (yyval.value).unit = CSSPrimitiveValue::CSS_DEG; }
    break;

  case 179:

    { (yyval.value).id = 0; (yyval.value).fValue = (yyvsp[(1) - (2)].val); (yyval.value).unit = CSSPrimitiveValue::CSS_RAD; }
    break;

  case 180:

    { (yyval.value).id = 0; (yyval.value).fValue = (yyvsp[(1) - (2)].val); (yyval.value).unit = CSSPrimitiveValue::CSS_GRAD; }
    break;

  case 181:

    { (yyval.value).id = 0; (yyval.value).fValue = (yyvsp[(1) - (2)].val); (yyval.value).unit = CSSPrimitiveValue::CSS_MS; }
    break;

  case 182:

    { (yyval.value).id = 0; (yyval.value).fValue = (yyvsp[(1) - (2)].val); (yyval.value).unit = CSSPrimitiveValue::CSS_S; }
    break;

  case 183:

    { (yyval.value).id = 0; (yyval.value).fValue = (yyvsp[(1) - (2)].val); (yyval.value).unit = CSSPrimitiveValue::CSS_HZ; }
    break;

  case 184:

    { (yyval.value).id = 0; (yyval.value).fValue = (yyvsp[(1) - (2)].val); (yyval.value).unit = CSSPrimitiveValue::CSS_KHZ; }
    break;

  case 185:

    { (yyval.value).id = 0; (yyval.value).fValue = (yyvsp[(1) - (2)].val); (yyval.value).unit = CSSPrimitiveValue::CSS_EMS; }
    break;

  case 186:

    { (yyval.value).id = 0; (yyval.value).fValue = (yyvsp[(1) - (2)].val); (yyval.value).unit = Value::Q_EMS; }
    break;

  case 187:

    { (yyval.value).id = 0; (yyval.value).fValue = (yyvsp[(1) - (2)].val); (yyval.value).unit = CSSPrimitiveValue::CSS_EXS; }
    break;

  case 188:

    { (yyval.value).id = 0; (yyval.value).fValue = (yyvsp[(1) - (2)].val); (yyval.value).unit = CSSPrimitiveValue::CSS_CHS; }
    break;

  case 189:

    { (yyval.value).id = 0; (yyval.value).fValue = (yyvsp[(1) - (2)].val); (yyval.value).unit = CSSPrimitiveValue::CSS_DPI; }
    break;

  case 190:

    { (yyval.value).id = 0; (yyval.value).fValue = (yyvsp[(1) - (2)].val); (yyval.value).unit = CSSPrimitiveValue::CSS_DPCM; }
    break;

  case 191:

    {
      Function *f = new Function;
      f->name = (yyvsp[(1) - (5)].string);
      f->args = (yyvsp[(3) - (5)].valueList);
      (yyval.value).id = 0;
      (yyval.value).unit = Value::Function;
      (yyval.value).function = f;
  }
    break;

  case 192:

    {
      Function *f = new Function;
      f->name = (yyvsp[(1) - (3)].string);
      f->args = 0;
      (yyval.value).id = 0;
      (yyval.value).unit = Value::Function;
      (yyval.value).function = f;
  }
    break;

  case 193:

    { (yyval.string) = (yyvsp[(1) - (2)].string); }
    break;

  case 194:

    {
	(yyval.rule) = 0;
#ifdef CSS_DEBUG
	qDebug() << "skipped invalid @-rule";
#endif
    }
    break;

  case 195:

    {
	(yyval.rule) = 0;
#ifdef CSS_DEBUG
	qDebug() << "skipped invalid @-rule";
#endif
    }
    break;

  case 196:

    {
	(yyval.rule) = 0;
#ifdef CSS_DEBUG
	qDebug() << "skipped invalid rule";
#endif
    }
    break;



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

#if !defined yyoverflow || YYERROR_VERBOSE
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
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
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






