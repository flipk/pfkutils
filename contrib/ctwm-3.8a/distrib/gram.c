
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
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
#define YYBISON_VERSION "2.4.1"

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



/* Copy the first part of user declarations.  */

/* Line 189 of yacc.c  */
#line 67 "gram.y"

#include <stdio.h>
#include <ctype.h>
#include "twm.h"
#include "menus.h"
#include "icons.h"
#include "windowbox.h"
#include "add_window.h"
#include "list.h"
#include "util.h"
#include "screen.h"
#include "parse.h"
#include "cursor.h"
#ifdef VMS
#  include <decw$include/Xos.h>
#  include <X11Xmu/CharSet.h>
#else
#  include <X11/Xos.h>
#  include <X11/Xmu/CharSet.h>
#endif

static char *Action = "";
static char *Name = "";
static char *defstring = "default";
static MenuRoot	*root, *pull = NULL;
static char *curWorkSpc;
static char *client, *workspace;
static MenuItem *lastmenuitem = (MenuItem*) 0;

extern void yyerror(char *s);
extern void RemoveDQuote(char *str);

static MenuRoot *GetRoot(char *name, char *fore, char *back);

static Bool CheckWarpScreenArg(register char *s);
static Bool CheckWarpRingArg(register char *s);
static Bool CheckColormapArg(register char *s);
static void GotButton(int butt, int func);
static void GotKey(char *key, int func);
static void GotTitleButton(char *bitmapname, int func, Bool rightside);
static char *ptr;
static name_list **list;
static int cont = 0;
static int color;
Bool donttoggleworkspacemanagerstate = FALSE;
int mods = 0;
unsigned int mods_used = (ShiftMask | ControlMask | Mod1Mask);

extern void twmrc_error_prefix(void);

extern int yylex(void);


/* Line 189 of yacc.c  */
#line 127 "y.tab.c"

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
     LB = 258,
     RB = 259,
     LP = 260,
     RP = 261,
     MENUS = 262,
     MENU = 263,
     BUTTON = 264,
     DEFAULT_FUNCTION = 265,
     PLUS = 266,
     MINUS = 267,
     ALL = 268,
     OR = 269,
     CURSORS = 270,
     PIXMAPS = 271,
     ICONS = 272,
     COLOR = 273,
     SAVECOLOR = 274,
     MONOCHROME = 275,
     FUNCTION = 276,
     ICONMGR_SHOW = 277,
     ICONMGR = 278,
     ALTER = 279,
     WINDOW_FUNCTION = 280,
     ZOOM = 281,
     ICONMGRS = 282,
     ICONMGR_GEOMETRY = 283,
     ICONMGR_NOSHOW = 284,
     MAKE_TITLE = 285,
     ICONIFY_BY_UNMAPPING = 286,
     DONT_ICONIFY_BY_UNMAPPING = 287,
     NO_BORDER = 288,
     NO_ICON_TITLE = 289,
     NO_TITLE = 290,
     AUTO_RAISE = 291,
     NO_HILITE = 292,
     ICON_REGION = 293,
     WINDOW_REGION = 294,
     META = 295,
     SHIFT = 296,
     LOCK = 297,
     CONTROL = 298,
     WINDOW = 299,
     TITLE = 300,
     ICON = 301,
     ROOT = 302,
     FRAME = 303,
     COLON = 304,
     EQUALS = 305,
     SQUEEZE_TITLE = 306,
     DONT_SQUEEZE_TITLE = 307,
     START_ICONIFIED = 308,
     NO_TITLE_HILITE = 309,
     TITLE_HILITE = 310,
     MOVE = 311,
     RESIZE = 312,
     WAITC = 313,
     SELECT = 314,
     KILL = 315,
     LEFT_TITLEBUTTON = 316,
     RIGHT_TITLEBUTTON = 317,
     NUMBER = 318,
     KEYWORD = 319,
     NKEYWORD = 320,
     CKEYWORD = 321,
     CLKEYWORD = 322,
     FKEYWORD = 323,
     FSKEYWORD = 324,
     SKEYWORD = 325,
     SSKEYWORD = 326,
     DKEYWORD = 327,
     JKEYWORD = 328,
     WINDOW_RING = 329,
     WINDOW_RING_EXCLUDE = 330,
     WARP_CURSOR = 331,
     ERRORTOKEN = 332,
     NO_STACKMODE = 333,
     ALWAYS_ON_TOP = 334,
     WORKSPACE = 335,
     WORKSPACES = 336,
     WORKSPCMGR_GEOMETRY = 337,
     OCCUPYALL = 338,
     OCCUPYLIST = 339,
     MAPWINDOWCURRENTWORKSPACE = 340,
     MAPWINDOWDEFAULTWORKSPACE = 341,
     UNMAPBYMOVINGFARAWAY = 342,
     OPAQUEMOVE = 343,
     NOOPAQUEMOVE = 344,
     OPAQUERESIZE = 345,
     NOOPAQUERESIZE = 346,
     DONTSETINACTIVE = 347,
     CHANGE_WORKSPACE_FUNCTION = 348,
     DEICONIFY_FUNCTION = 349,
     ICONIFY_FUNCTION = 350,
     AUTOSQUEEZE = 351,
     STARTSQUEEZED = 352,
     DONT_SAVE = 353,
     AUTO_LOWER = 354,
     ICONMENU_DONTSHOW = 355,
     WINDOW_BOX = 356,
     IGNOREMODIFIER = 357,
     WINDOW_GEOMETRIES = 358,
     ALWAYSSQUEEZETOGRAVITY = 359,
     VIRTUAL_SCREENS = 360,
     IGNORE_TRANSIENT = 361,
     DONTTOGGLEWORKSPACEMANAGERSTATE = 362,
     STRING = 363
   };
#endif
/* Tokens.  */
#define LB 258
#define RB 259
#define LP 260
#define RP 261
#define MENUS 262
#define MENU 263
#define BUTTON 264
#define DEFAULT_FUNCTION 265
#define PLUS 266
#define MINUS 267
#define ALL 268
#define OR 269
#define CURSORS 270
#define PIXMAPS 271
#define ICONS 272
#define COLOR 273
#define SAVECOLOR 274
#define MONOCHROME 275
#define FUNCTION 276
#define ICONMGR_SHOW 277
#define ICONMGR 278
#define ALTER 279
#define WINDOW_FUNCTION 280
#define ZOOM 281
#define ICONMGRS 282
#define ICONMGR_GEOMETRY 283
#define ICONMGR_NOSHOW 284
#define MAKE_TITLE 285
#define ICONIFY_BY_UNMAPPING 286
#define DONT_ICONIFY_BY_UNMAPPING 287
#define NO_BORDER 288
#define NO_ICON_TITLE 289
#define NO_TITLE 290
#define AUTO_RAISE 291
#define NO_HILITE 292
#define ICON_REGION 293
#define WINDOW_REGION 294
#define META 295
#define SHIFT 296
#define LOCK 297
#define CONTROL 298
#define WINDOW 299
#define TITLE 300
#define ICON 301
#define ROOT 302
#define FRAME 303
#define COLON 304
#define EQUALS 305
#define SQUEEZE_TITLE 306
#define DONT_SQUEEZE_TITLE 307
#define START_ICONIFIED 308
#define NO_TITLE_HILITE 309
#define TITLE_HILITE 310
#define MOVE 311
#define RESIZE 312
#define WAITC 313
#define SELECT 314
#define KILL 315
#define LEFT_TITLEBUTTON 316
#define RIGHT_TITLEBUTTON 317
#define NUMBER 318
#define KEYWORD 319
#define NKEYWORD 320
#define CKEYWORD 321
#define CLKEYWORD 322
#define FKEYWORD 323
#define FSKEYWORD 324
#define SKEYWORD 325
#define SSKEYWORD 326
#define DKEYWORD 327
#define JKEYWORD 328
#define WINDOW_RING 329
#define WINDOW_RING_EXCLUDE 330
#define WARP_CURSOR 331
#define ERRORTOKEN 332
#define NO_STACKMODE 333
#define ALWAYS_ON_TOP 334
#define WORKSPACE 335
#define WORKSPACES 336
#define WORKSPCMGR_GEOMETRY 337
#define OCCUPYALL 338
#define OCCUPYLIST 339
#define MAPWINDOWCURRENTWORKSPACE 340
#define MAPWINDOWDEFAULTWORKSPACE 341
#define UNMAPBYMOVINGFARAWAY 342
#define OPAQUEMOVE 343
#define NOOPAQUEMOVE 344
#define OPAQUERESIZE 345
#define NOOPAQUERESIZE 346
#define DONTSETINACTIVE 347
#define CHANGE_WORKSPACE_FUNCTION 348
#define DEICONIFY_FUNCTION 349
#define ICONIFY_FUNCTION 350
#define AUTOSQUEEZE 351
#define STARTSQUEEZED 352
#define DONT_SAVE 353
#define AUTO_LOWER 354
#define ICONMENU_DONTSHOW 355
#define WINDOW_BOX 356
#define IGNOREMODIFIER 357
#define WINDOW_GEOMETRIES 358
#define ALWAYSSQUEEZETOGRAVITY 359
#define VIRTUAL_SCREENS 360
#define IGNORE_TRANSIENT 361
#define DONTTOGGLEWORKSPACEMANAGERSTATE 362
#define STRING 363




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 214 of yacc.c  */
#line 121 "gram.y"

    int num;
    char *ptr;



/* Line 214 of yacc.c  */
#line 386 "y.tab.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 398 "y.tab.c"

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
# if YYENABLE_NLS
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
#define YYFINAL  3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   858

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  109
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  141
/* YYNRULES -- Number of rules.  */
#define YYNRULES  340
/* YYNRULES -- Number of states.  */
#define YYNSTATES  489

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   363

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
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
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     5,     6,     9,    11,    13,    15,    17,
      19,    26,    34,    43,    53,    54,    63,    64,    74,    75,
      86,    87,    99,   100,   107,   108,   114,   118,   121,   125,
     128,   129,   133,   134,   138,   141,   143,   146,   149,   150,
     154,   156,   157,   161,   163,   164,   168,   170,   171,   175,
     177,   178,   182,   184,   189,   194,   195,   200,   201,   206,
     209,   212,   215,   218,   219,   223,   224,   228,   229,   230,
     237,   238,   242,   243,   247,   248,   252,   253,   257,   258,
     262,   263,   267,   269,   270,   274,   275,   279,   280,   284,
     286,   287,   291,   292,   296,   297,   301,   303,   304,   308,
     310,   311,   315,   316,   320,   322,   323,   327,   328,   332,
     333,   337,   339,   340,   344,   346,   347,   351,   353,   354,
     358,   359,   363,   364,   368,   370,   371,   375,   377,   378,
     388,   389,   394,   395,   400,   401,   405,   406,   410,   411,
     415,   416,   420,   423,   426,   429,   432,   435,   436,   440,
     442,   443,   447,   449,   450,   454,   455,   459,   460,   464,
     466,   469,   471,   475,   478,   480,   483,   488,   495,   502,
     503,   506,   508,   510,   512,   514,   517,   520,   522,   523,
     526,   528,   530,   532,   534,   536,   538,   540,   542,   544,
     546,   548,   549,   552,   554,   556,   558,   560,   562,   564,
     566,   568,   570,   572,   574,   576,   580,   581,   584,   587,
     591,   595,   599,   600,   603,   606,   610,   611,   614,   618,
     621,   625,   628,   632,   635,   639,   642,   646,   649,   653,
     656,   660,   663,   667,   670,   674,   677,   681,   684,   688,
     691,   695,   696,   699,   702,   703,   708,   711,   715,   716,
     719,   721,   723,   727,   728,   731,   734,   738,   739,   742,
     745,   749,   750,   753,   755,   757,   758,   764,   766,   767,
     771,   772,   778,   782,   783,   786,   790,   795,   799,   800,
     803,   805,   806,   810,   814,   815,   818,   820,   823,   827,
     832,   838,   842,   847,   853,   860,   864,   869,   875,   882,
     886,   887,   890,   892,   896,   897,   900,   901,   905,   906,
     911,   912,   917,   921,   922,   925,   927,   931,   932,   935,
     937,   941,   942,   945,   948,   952,   953,   956,   958,   962,
     963,   966,   969,   977,   979,   982,   984,   987,   990,   993,
     995
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     110,     0,    -1,   111,    -1,    -1,   111,   112,    -1,     1,
      -1,   168,    -1,   169,    -1,   170,    -1,   205,    -1,    38,
     248,    72,    72,   249,   249,    -1,    38,   248,    72,    72,
     249,   249,   248,    -1,    38,   248,    72,    72,   249,   249,
     248,   248,    -1,    38,   248,    72,    72,   249,   249,   248,
     248,   248,    -1,    -1,    38,   248,    72,    72,   249,   249,
     113,   221,    -1,    -1,    38,   248,    72,    72,   249,   249,
     248,   114,   221,    -1,    -1,    38,   248,    72,    72,   249,
     249,   248,   248,   115,   221,    -1,    -1,    38,   248,    72,
      72,   249,   249,   248,   248,   248,   116,   221,    -1,    -1,
      39,   248,    72,    72,   117,   221,    -1,    -1,   101,   248,
     248,   118,   221,    -1,    28,   248,   249,    -1,    28,   248,
      -1,    82,   248,   249,    -1,    82,   248,    -1,    -1,    85,
     119,   219,    -1,    -1,    86,   120,   220,    -1,    26,   249,
      -1,    26,    -1,    16,   183,    -1,    15,   186,    -1,    -1,
      31,   121,   221,    -1,    31,    -1,    -1,    88,   122,   221,
      -1,    88,    -1,    -1,    89,   123,   221,    -1,    89,    -1,
      -1,    90,   124,   221,    -1,    90,    -1,    -1,    91,   125,
     221,    -1,    91,    -1,    61,   248,    50,   245,    -1,    62,
     248,    50,   245,    -1,    -1,    61,   248,   126,   180,    -1,
      -1,    62,   248,   127,   180,    -1,   247,   248,    -1,   247,
     245,    -1,   248,   173,    -1,   247,   172,    -1,    -1,    32,
     128,   221,    -1,    -1,    81,   129,   212,    -1,    -1,    -1,
     102,   130,     3,   174,   131,     4,    -1,    -1,    83,   132,
     221,    -1,    -1,   100,   133,   221,    -1,    -1,    84,   134,
     224,    -1,    -1,    87,   135,   221,    -1,    -1,    96,   136,
     221,    -1,    -1,    97,   137,   221,    -1,   104,    -1,    -1,
     104,   138,   221,    -1,    -1,    92,   139,   221,    -1,    -1,
      29,   140,   221,    -1,    29,    -1,    -1,    27,   141,   209,
      -1,    -1,    22,   142,   221,    -1,    -1,    54,   143,   221,
      -1,    54,    -1,    -1,    37,   144,   221,    -1,    37,    -1,
      -1,    79,   145,   221,    -1,    -1,    78,   146,   221,    -1,
      78,    -1,    -1,    33,   147,   221,    -1,    -1,    98,   148,
     221,    -1,    -1,    34,   149,   221,    -1,    34,    -1,    -1,
      35,   150,   221,    -1,    35,    -1,    -1,   106,   151,   221,
      -1,   107,    -1,    -1,    30,   152,   221,    -1,    -1,    53,
     153,   221,    -1,    -1,    36,   154,   221,    -1,    36,    -1,
      -1,    99,   155,   221,    -1,    99,    -1,    -1,     8,   248,
       5,   248,    49,   248,     6,   156,   242,    -1,    -1,     8,
     248,   157,   242,    -1,    -1,    21,   248,   158,   239,    -1,
      -1,    17,   159,   236,    -1,    -1,    18,   160,   189,    -1,
      -1,    19,   161,   193,    -1,    -1,    20,   162,   189,    -1,
      10,   245,    -1,    25,   245,    -1,    93,   245,    -1,    94,
     245,    -1,    95,   245,    -1,    -1,    76,   163,   221,    -1,
      76,    -1,    -1,    74,   164,   221,    -1,    74,    -1,    -1,
      75,   165,   221,    -1,    -1,   103,   166,   199,    -1,    -1,
     105,   167,   202,    -1,    64,    -1,    70,   248,    -1,    70,
      -1,    71,   248,   248,    -1,    71,   248,    -1,    71,    -1,
      65,   249,    -1,    50,   174,    49,   245,    -1,    50,   174,
      49,   176,    49,   245,    -1,    50,   174,    49,   178,    49,
     245,    -1,    -1,   174,   175,    -1,    40,    -1,    41,    -1,
      42,    -1,    43,    -1,    24,   249,    -1,    40,   249,    -1,
      14,    -1,    -1,   176,   177,    -1,    44,    -1,    45,    -1,
      46,    -1,    47,    -1,    48,    -1,    80,    -1,    23,    -1,
      40,    -1,    24,    -1,    13,    -1,    14,    -1,    -1,   178,
     179,    -1,    44,    -1,    45,    -1,    46,    -1,    47,    -1,
      48,    -1,    80,    -1,    23,    -1,    40,    -1,    13,    -1,
      24,    -1,    14,    -1,   248,    -1,     3,   181,     4,    -1,
      -1,   181,   182,    -1,   247,   171,    -1,   247,    50,   245,
      -1,   247,    49,   245,    -1,     3,   184,     4,    -1,    -1,
     184,   185,    -1,    55,   248,    -1,     3,   187,     4,    -1,
      -1,   187,   188,    -1,    48,   248,   248,    -1,    48,   248,
      -1,    45,   248,   248,    -1,    45,   248,    -1,    46,   248,
     248,    -1,    46,   248,    -1,    23,   248,   248,    -1,    23,
     248,    -1,     9,   248,   248,    -1,     9,   248,    -1,    56,
     248,   248,    -1,    56,   248,    -1,    57,   248,   248,    -1,
      57,   248,    -1,    58,   248,   248,    -1,    58,   248,    -1,
       8,   248,   248,    -1,     8,   248,    -1,    59,   248,   248,
      -1,    59,   248,    -1,    60,   248,   248,    -1,    60,   248,
      -1,     3,   190,     4,    -1,    -1,   190,   191,    -1,    67,
     248,    -1,    -1,    67,   248,   192,   196,    -1,    66,   248,
      -1,     3,   194,     4,    -1,    -1,   194,   195,    -1,   248,
      -1,    67,    -1,     3,   197,     4,    -1,    -1,   197,   198,
      -1,   248,   248,    -1,     3,   200,     4,    -1,    -1,   200,
     201,    -1,   248,   248,    -1,     3,   203,     4,    -1,    -1,
     203,   204,    -1,   248,    -1,    51,    -1,    -1,    51,   206,
       3,   208,     4,    -1,    52,    -1,    -1,    52,   207,   221,
      -1,    -1,   208,   248,    73,   246,   249,    -1,     3,   210,
       4,    -1,    -1,   210,   211,    -1,   248,   248,   249,    -1,
     248,   248,   248,   249,    -1,     3,   213,     4,    -1,    -1,
     213,   214,    -1,   248,    -1,    -1,   248,   215,   216,    -1,
       3,   217,     4,    -1,    -1,   217,   218,    -1,   248,    -1,
     248,   248,    -1,   248,   248,   248,    -1,   248,   248,   248,
     248,    -1,   248,   248,   248,   248,   248,    -1,     3,   248,
       4,    -1,     3,   248,   248,     4,    -1,     3,   248,   248,
     248,     4,    -1,     3,   248,   248,   248,   248,     4,    -1,
       3,   248,     4,    -1,     3,   248,   248,     4,    -1,     3,
     248,   248,   248,     4,    -1,     3,   248,   248,   248,   248,
       4,    -1,     3,   222,     4,    -1,    -1,   222,   223,    -1,
     248,    -1,     3,   225,     4,    -1,    -1,   225,   226,    -1,
      -1,   248,   227,   230,    -1,    -1,    44,   248,   228,   230,
      -1,    -1,    80,   248,   229,   233,    -1,     3,   231,     4,
      -1,    -1,   231,   232,    -1,   248,    -1,     3,   234,     4,
      -1,    -1,   234,   235,    -1,   248,    -1,     3,   237,     4,
      -1,    -1,   237,   238,    -1,   248,   248,    -1,     3,   240,
       4,    -1,    -1,   240,   241,    -1,   245,    -1,     3,   243,
       4,    -1,    -1,   243,   244,    -1,   248,   245,    -1,   248,
       5,   248,    49,   248,     6,   245,    -1,    68,    -1,    69,
     248,    -1,   249,    -1,    11,   249,    -1,    12,   249,    -1,
       9,   249,    -1,   108,    -1,    63,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   154,   154,   157,   158,   161,   162,   163,   164,   165,
     166,   169,   172,   175,   178,   178,   182,   182,   186,   186,
     190,   190,   195,   195,   200,   200,   205,   211,   214,   220,
     223,   223,   226,   226,   229,   235,   237,   238,   239,   239,
     241,   244,   244,   246,   247,   247,   249,   250,   250,   252,
     253,   253,   255,   257,   260,   263,   263,   265,   265,   267,
     271,   287,   288,   290,   290,   292,   292,   294,   294,   294,
     295,   295,   297,   297,   299,   299,   301,   301,   303,   303,
     305,   305,   307,   308,   308,   310,   310,   312,   312,   314,
     315,   315,   317,   317,   319,   319,   321,   323,   323,   325,
     327,   327,   329,   329,   331,   333,   333,   335,   335,   337,
     337,   339,   341,   341,   343,   345,   345,   347,   348,   348,
     350,   350,   352,   352,   354,   355,   355,   357,   358,   358,
     361,   361,   363,   363,   365,   365,   367,   367,   369,   369,
     371,   371,   373,   389,   397,   405,   413,   421,   421,   423,
     425,   425,   427,   428,   428,   432,   432,   434,   434,   438,
     448,   456,   466,   475,   483,   493,   505,   508,   511,   514,
     515,   518,   519,   520,   521,   522,   532,   542,   545,   546,
     549,   550,   551,   552,   553,   554,   555,   556,   557,   558,
     559,   562,   563,   566,   567,   568,   569,   570,   571,   572,
     573,   574,   575,   576,   577,   581,   584,   585,   588,   589,
     591,   595,   598,   599,   602,   606,   609,   610,   613,   615,
     617,   619,   621,   623,   625,   627,   629,   631,   633,   635,
     637,   639,   641,   643,   645,   647,   649,   651,   653,   655,
     659,   663,   664,   667,   676,   676,   687,   698,   701,   702,
     705,   706,   709,   712,   713,   716,   721,   724,   725,   728,
     731,   734,   735,   738,   741,   744,   744,   749,   750,   750,
     754,   755,   763,   766,   767,   770,   775,   783,   786,   787,
     790,   793,   793,   799,   802,   803,   806,   809,   812,   815,
     818,   823,   826,   829,   832,   837,   840,   843,   846,   851,
     854,   855,   858,   863,   866,   867,   870,   870,   872,   872,
     874,   874,   878,   881,   882,   885,   890,   893,   894,   897,
     902,   905,   906,   909,   912,   915,   916,   919,   925,   928,
     929,   932,   942,   954,   955,   996,   997,   998,  1001,  1013,
    1020
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "LB", "RB", "LP", "RP", "MENUS", "MENU",
  "BUTTON", "DEFAULT_FUNCTION", "PLUS", "MINUS", "ALL", "OR", "CURSORS",
  "PIXMAPS", "ICONS", "COLOR", "SAVECOLOR", "MONOCHROME", "FUNCTION",
  "ICONMGR_SHOW", "ICONMGR", "ALTER", "WINDOW_FUNCTION", "ZOOM",
  "ICONMGRS", "ICONMGR_GEOMETRY", "ICONMGR_NOSHOW", "MAKE_TITLE",
  "ICONIFY_BY_UNMAPPING", "DONT_ICONIFY_BY_UNMAPPING", "NO_BORDER",
  "NO_ICON_TITLE", "NO_TITLE", "AUTO_RAISE", "NO_HILITE", "ICON_REGION",
  "WINDOW_REGION", "META", "SHIFT", "LOCK", "CONTROL", "WINDOW", "TITLE",
  "ICON", "ROOT", "FRAME", "COLON", "EQUALS", "SQUEEZE_TITLE",
  "DONT_SQUEEZE_TITLE", "START_ICONIFIED", "NO_TITLE_HILITE",
  "TITLE_HILITE", "MOVE", "RESIZE", "WAITC", "SELECT", "KILL",
  "LEFT_TITLEBUTTON", "RIGHT_TITLEBUTTON", "NUMBER", "KEYWORD", "NKEYWORD",
  "CKEYWORD", "CLKEYWORD", "FKEYWORD", "FSKEYWORD", "SKEYWORD",
  "SSKEYWORD", "DKEYWORD", "JKEYWORD", "WINDOW_RING",
  "WINDOW_RING_EXCLUDE", "WARP_CURSOR", "ERRORTOKEN", "NO_STACKMODE",
  "ALWAYS_ON_TOP", "WORKSPACE", "WORKSPACES", "WORKSPCMGR_GEOMETRY",
  "OCCUPYALL", "OCCUPYLIST", "MAPWINDOWCURRENTWORKSPACE",
  "MAPWINDOWDEFAULTWORKSPACE", "UNMAPBYMOVINGFARAWAY", "OPAQUEMOVE",
  "NOOPAQUEMOVE", "OPAQUERESIZE", "NOOPAQUERESIZE", "DONTSETINACTIVE",
  "CHANGE_WORKSPACE_FUNCTION", "DEICONIFY_FUNCTION", "ICONIFY_FUNCTION",
  "AUTOSQUEEZE", "STARTSQUEEZED", "DONT_SAVE", "AUTO_LOWER",
  "ICONMENU_DONTSHOW", "WINDOW_BOX", "IGNOREMODIFIER", "WINDOW_GEOMETRIES",
  "ALWAYSSQUEEZETOGRAVITY", "VIRTUAL_SCREENS", "IGNORE_TRANSIENT",
  "DONTTOGGLEWORKSPACEMANAGERSTATE", "STRING", "$accept", "twmrc", "stmts",
  "stmt", "$@1", "$@2", "$@3", "$@4", "$@5", "$@6", "$@7", "$@8", "$@9",
  "$@10", "$@11", "$@12", "$@13", "$@14", "$@15", "$@16", "$@17", "$@18",
  "$@19", "$@20", "$@21", "$@22", "$@23", "$@24", "$@25", "$@26", "$@27",
  "$@28", "$@29", "$@30", "$@31", "$@32", "$@33", "$@34", "$@35", "$@36",
  "$@37", "$@38", "$@39", "$@40", "$@41", "$@42", "$@43", "$@44", "$@45",
  "$@46", "$@47", "$@48", "$@49", "$@50", "$@51", "$@52", "$@53", "$@54",
  "$@55", "noarg", "sarg", "narg", "keyaction", "full", "fullkey", "keys",
  "key", "contexts", "context", "contextkeys", "contextkey",
  "binding_list", "binding_entries", "binding_entry", "pixmap_list",
  "pixmap_entries", "pixmap_entry", "cursor_list", "cursor_entries",
  "cursor_entry", "color_list", "color_entries", "color_entry", "$@56",
  "save_color_list", "s_color_entries", "s_color_entry", "win_color_list",
  "win_color_entries", "win_color_entry", "wingeom_list",
  "wingeom_entries", "wingeom_entry", "geom_list", "geom_entries",
  "geom_entry", "squeeze", "$@57", "$@58", "win_sqz_entries", "iconm_list",
  "iconm_entries", "iconm_entry", "workspc_list", "workspc_entries",
  "workspc_entry", "$@59", "workapp_list", "workapp_entries",
  "workapp_entry", "curwork", "defwork", "win_list", "win_entries",
  "win_entry", "occupy_list", "occupy_entries", "occupy_entry", "$@60",
  "$@61", "$@62", "occupy_workspc_list", "occupy_workspc_entries",
  "occupy_workspc_entry", "occupy_window_list", "occupy_window_entries",
  "occupy_window_entry", "icon_list", "icon_entries", "icon_entry",
  "function", "function_entries", "function_entry", "menu", "menu_entries",
  "menu_entry", "action", "signed_number", "button", "string", "number", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,   362,   363
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,   109,   110,   111,   111,   112,   112,   112,   112,   112,
     112,   112,   112,   112,   113,   112,   114,   112,   115,   112,
     116,   112,   117,   112,   118,   112,   112,   112,   112,   112,
     119,   112,   120,   112,   112,   112,   112,   112,   121,   112,
     112,   122,   112,   112,   123,   112,   112,   124,   112,   112,
     125,   112,   112,   112,   112,   126,   112,   127,   112,   112,
     112,   112,   112,   128,   112,   129,   112,   130,   131,   112,
     132,   112,   133,   112,   134,   112,   135,   112,   136,   112,
     137,   112,   112,   138,   112,   139,   112,   140,   112,   112,
     141,   112,   142,   112,   143,   112,   112,   144,   112,   112,
     145,   112,   146,   112,   112,   147,   112,   148,   112,   149,
     112,   112,   150,   112,   112,   151,   112,   112,   152,   112,
     153,   112,   154,   112,   112,   155,   112,   112,   156,   112,
     157,   112,   158,   112,   159,   112,   160,   112,   161,   112,
     162,   112,   112,   112,   112,   112,   112,   163,   112,   112,
     164,   112,   112,   165,   112,   166,   112,   167,   112,   168,
     169,   169,   169,   169,   169,   170,   171,   172,   173,   174,
     174,   175,   175,   175,   175,   175,   175,   175,   176,   176,
     177,   177,   177,   177,   177,   177,   177,   177,   177,   177,
     177,   178,   178,   179,   179,   179,   179,   179,   179,   179,
     179,   179,   179,   179,   179,   180,   181,   181,   182,   182,
     182,   183,   184,   184,   185,   186,   187,   187,   188,   188,
     188,   188,   188,   188,   188,   188,   188,   188,   188,   188,
     188,   188,   188,   188,   188,   188,   188,   188,   188,   188,
     189,   190,   190,   191,   192,   191,   191,   193,   194,   194,
     195,   195,   196,   197,   197,   198,   199,   200,   200,   201,
     202,   203,   203,   204,   205,   206,   205,   205,   207,   205,
     208,   208,   209,   210,   210,   211,   211,   212,   213,   213,
     214,   215,   214,   216,   217,   217,   218,   218,   218,   218,
     218,   219,   219,   219,   219,   220,   220,   220,   220,   221,
     222,   222,   223,   224,   225,   225,   227,   226,   228,   226,
     229,   226,   230,   231,   231,   232,   233,   234,   234,   235,
     236,   237,   237,   238,   239,   240,   240,   241,   242,   243,
     243,   244,   244,   245,   245,   246,   246,   246,   247,   248,
     249
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     0,     2,     1,     1,     1,     1,     1,
       6,     7,     8,     9,     0,     8,     0,     9,     0,    10,
       0,    11,     0,     6,     0,     5,     3,     2,     3,     2,
       0,     3,     0,     3,     2,     1,     2,     2,     0,     3,
       1,     0,     3,     1,     0,     3,     1,     0,     3,     1,
       0,     3,     1,     4,     4,     0,     4,     0,     4,     2,
       2,     2,     2,     0,     3,     0,     3,     0,     0,     6,
       0,     3,     0,     3,     0,     3,     0,     3,     0,     3,
       0,     3,     1,     0,     3,     0,     3,     0,     3,     1,
       0,     3,     0,     3,     0,     3,     1,     0,     3,     1,
       0,     3,     0,     3,     1,     0,     3,     0,     3,     0,
       3,     1,     0,     3,     1,     0,     3,     1,     0,     3,
       0,     3,     0,     3,     1,     0,     3,     1,     0,     9,
       0,     4,     0,     4,     0,     3,     0,     3,     0,     3,
       0,     3,     2,     2,     2,     2,     2,     0,     3,     1,
       0,     3,     1,     0,     3,     0,     3,     0,     3,     1,
       2,     1,     3,     2,     1,     2,     4,     6,     6,     0,
       2,     1,     1,     1,     1,     2,     2,     1,     0,     2,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     0,     2,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     3,     0,     2,     2,     3,
       3,     3,     0,     2,     2,     3,     0,     2,     3,     2,
       3,     2,     3,     2,     3,     2,     3,     2,     3,     2,
       3,     2,     3,     2,     3,     2,     3,     2,     3,     2,
       3,     0,     2,     2,     0,     4,     2,     3,     0,     2,
       1,     1,     3,     0,     2,     2,     3,     0,     2,     2,
       3,     0,     2,     1,     1,     0,     5,     1,     0,     3,
       0,     5,     3,     0,     2,     3,     4,     3,     0,     2,
       1,     0,     3,     3,     0,     2,     1,     2,     3,     4,
       5,     3,     4,     5,     6,     3,     4,     5,     6,     3,
       0,     2,     1,     3,     0,     2,     0,     3,     0,     4,
       0,     4,     3,     0,     2,     1,     3,     0,     2,     1,
       3,     0,     2,     2,     3,     0,     2,     1,     3,     0,
       2,     2,     7,     1,     2,     1,     2,     2,     2,     1,
       1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       3,     0,     0,     1,     5,     0,     0,     0,     0,     0,
     134,   136,   138,   140,     0,    92,     0,    35,    90,     0,
      89,   118,    40,    63,   105,   111,   114,   124,    99,     0,
       0,   264,   267,   120,    96,     0,     0,   159,     0,   161,
     164,   152,   153,   149,   104,   100,    65,     0,    70,    74,
      30,    32,    76,    43,    46,    49,    52,    85,     0,     0,
       0,    78,    80,   107,   127,    72,     0,    67,   155,    82,
     157,   115,   117,   339,     4,     6,     7,     8,     9,     0,
       0,   130,   340,   338,   333,     0,   142,   216,    37,   212,
      36,     0,     0,     0,     0,   132,     0,   143,    34,     0,
      27,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    55,    57,   165,   160,
     163,     0,     0,     0,     0,     0,     0,    29,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   144,   145,
     146,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   169,    62,    60,    59,   169,    61,     0,     0,
     334,     0,     0,   321,   135,   241,   137,   248,   139,   141,
       0,   300,    93,   273,    91,    26,    88,   119,    39,    64,
     106,   110,   113,   123,    98,     0,     0,   270,   269,   121,
      95,     0,     0,     0,     0,   162,   151,   154,   148,   103,
     101,   278,    66,    28,    71,   304,    75,     0,    31,     0,
      33,    77,    42,    45,    48,    51,    86,    79,    81,   108,
     126,    73,    24,   169,   257,   156,    84,   261,   158,   116,
       0,     0,     0,   329,   131,   215,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   217,   211,     0,
     213,     0,     0,     0,   325,   133,     0,     0,     0,    22,
       0,    53,   206,    56,    54,    58,     0,     0,     0,     0,
       0,    68,     0,     0,   177,     0,   171,   172,   173,   174,
     178,   170,   191,     0,     0,   235,   227,   225,   221,   223,
     219,   229,   231,   233,   237,   239,   214,   320,   322,     0,
     240,     0,     0,   242,   247,   251,   249,   250,     0,   299,
     301,   302,   272,   274,     0,     0,     0,   266,     0,     0,
     277,   279,   280,   303,     0,     0,   305,   306,   291,     0,
     295,     0,    25,     0,   256,   258,     0,   260,   262,   263,
     175,   176,     0,     0,     0,   328,   330,     0,   234,   226,
     224,   220,   222,   218,   228,   230,   232,   236,   238,   323,
     246,   243,   324,   326,   327,     0,    10,    23,     0,   205,
     207,     0,     0,   308,   310,     0,   292,     0,   296,     0,
      69,   259,   189,   190,   186,   188,   187,   180,   181,   182,
     183,   184,     0,   185,   179,   201,   203,   199,   202,   200,
     193,   194,   195,   196,   197,     0,   198,   192,   204,   128,
       0,   331,     0,     0,   275,     0,    11,     0,     0,     0,
     335,     0,   169,   208,   284,   282,     0,     0,   313,   307,
     293,     0,   297,     0,   167,   168,     0,     0,   253,   245,
     276,    15,     0,    12,   336,   337,   271,   210,     0,   209,
       0,   309,   317,   311,     0,   294,   298,   129,     0,     0,
      17,     0,    13,     0,   283,   285,   286,     0,   312,   314,
     315,     0,   252,   254,     0,    19,     0,   166,   287,   316,
     318,   319,     0,   255,    21,   288,   332,   289,   290
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,     2,    74,   415,   442,   461,   476,   316,   270,
     130,   131,   103,   133,   134,   135,   136,   192,   194,   104,
     126,   147,   333,   128,   145,   129,   132,   141,   142,   149,
     137,   101,    99,    96,   115,   109,   125,   124,   105,   143,
     106,   107,   151,   102,   114,   108,   144,   436,   159,   170,
      91,    92,    93,    94,   123,   121,   122,   148,   150,    75,
      76,    77,   423,   153,   157,   230,   281,   342,   394,   343,
     407,   263,   319,   370,    90,   162,   250,    88,   161,   247,
     166,   252,   303,   412,   168,   253,   306,   439,   459,   473,
     225,   272,   335,   228,   273,   338,    78,   112,   113,   260,
     174,   257,   313,   202,   266,   321,   372,   425,   450,   465,
     208,   210,   172,   256,   310,   206,   267,   326,   375,   426,
     427,   429,   454,   469,   453,   467,   480,   164,   251,   298,
     255,   308,   363,   234,   284,   346,    86,   419,    79,    80,
      83
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -220
static const yytype_int16 yypact[] =
{
    -220,    39,   520,  -220,  -220,   -86,   -14,   -22,    52,    54,
    -220,  -220,  -220,  -220,   -86,  -220,   -22,   -14,  -220,   -86,
      55,  -220,    63,  -220,  -220,    64,    68,    74,    75,   -86,
     -86,    76,    78,  -220,    79,   -86,   -86,  -220,   -14,   -86,
     -86,    80,  -220,    83,    84,  -220,  -220,   -86,  -220,  -220,
    -220,  -220,  -220,    85,    87,    88,    97,  -220,   -22,   -22,
     -22,  -220,  -220,  -220,    98,  -220,   -86,  -220,  -220,    99,
    -220,  -220,  -220,  -220,  -220,  -220,  -220,  -220,  -220,     0,
      57,   108,  -220,  -220,  -220,   -86,  -220,  -220,  -220,  -220,
    -220,   115,   117,   125,   117,  -220,   135,  -220,  -220,   136,
     -14,   135,   135,   135,   135,   135,   135,   135,   135,   135,
      71,    77,   143,   135,   135,   135,   100,   101,  -220,  -220,
     -86,   135,   135,   135,   135,   135,   149,   -14,   135,   152,
     153,   157,   135,   135,   135,   135,   135,   135,  -220,  -220,
    -220,   135,   135,   135,   135,   135,   -86,   160,   161,   135,
     168,   135,  -220,  -220,  -220,  -220,  -220,  -220,   -86,   171,
    -220,   150,    37,  -220,  -220,  -220,  -220,  -220,  -220,  -220,
     172,  -220,  -220,  -220,  -220,  -220,  -220,  -220,  -220,  -220,
    -220,  -220,  -220,  -220,  -220,   105,   107,  -220,  -220,  -220,
    -220,   -22,   177,   -22,   177,  -220,  -220,  -220,  -220,  -220,
    -220,  -220,  -220,  -220,  -220,  -220,  -220,   -86,  -220,   -86,
    -220,  -220,  -220,  -220,  -220,  -220,  -220,  -220,  -220,  -220,
    -220,  -220,  -220,  -220,  -220,  -220,  -220,  -220,  -220,  -220,
     148,   179,   132,  -220,  -220,  -220,   -86,   -86,   -86,   -86,
     -86,   -86,   -86,   -86,   -86,   -86,   -86,  -220,  -220,   -86,
    -220,     6,    81,    -2,  -220,  -220,     8,     9,   -14,  -220,
      11,  -220,  -220,  -220,  -220,  -220,    13,     4,    14,    15,
     135,   299,    16,    17,  -220,   -14,   -14,  -220,  -220,  -220,
    -220,  -220,  -220,   -86,    19,   -86,   -86,   -86,   -86,   -86,
     -86,   -86,   -86,   -86,   -86,   -86,  -220,  -220,  -220,   -86,
    -220,   -86,   -86,  -220,  -220,  -220,  -220,  -220,     7,  -220,
    -220,  -220,  -220,  -220,   -86,   -14,   135,  -220,   109,    47,
    -220,  -220,   180,  -220,   -86,   -86,  -220,  -220,  -220,    22,
    -220,    23,  -220,   181,  -220,  -220,   -86,  -220,  -220,  -220,
    -220,  -220,   121,    49,   178,  -220,  -220,    35,  -220,  -220,
    -220,  -220,  -220,  -220,  -220,  -220,  -220,  -220,  -220,  -220,
    -220,   183,  -220,  -220,  -220,   -56,     1,  -220,    42,  -220,
    -220,    10,   184,  -220,  -220,   189,  -220,    24,  -220,    25,
    -220,  -220,  -220,  -220,  -220,  -220,  -220,  -220,  -220,  -220,
    -220,  -220,   -22,  -220,  -220,  -220,  -220,  -220,  -220,  -220,
    -220,  -220,  -220,  -220,  -220,   -22,  -220,  -220,  -220,  -220,
     -86,  -220,   191,   -14,  -220,   135,     2,   -14,   -14,   -14,
    -220,   -22,   -22,  -220,  -220,  -220,   189,   196,  -220,  -220,
    -220,   201,  -220,   207,  -220,  -220,   171,   151,  -220,  -220,
    -220,  -220,   135,     3,  -220,  -220,  -220,  -220,   287,  -220,
      28,  -220,  -220,  -220,    29,  -220,  -220,  -220,   -86,    32,
    -220,   135,   209,   -22,  -220,  -220,   -86,    34,  -220,  -220,
    -220,   208,  -220,  -220,   -86,  -220,   135,  -220,   -86,  -220,
    -220,  -220,   -22,  -220,  -220,   -86,  -220,   -86,  -220
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -220,  -220,  -220,  -220,  -220,  -220,  -220,  -220,  -220,  -220,
    -220,  -220,  -220,  -220,  -220,  -220,  -220,  -220,  -220,  -220,
    -220,  -220,  -220,  -220,  -220,  -220,  -220,  -220,  -220,  -220,
    -220,  -220,  -220,  -220,  -220,  -220,  -220,  -220,  -220,  -220,
    -220,  -220,  -220,  -220,  -220,  -220,  -220,  -220,  -220,  -220,
    -220,  -220,  -220,  -220,  -220,  -220,  -220,  -220,  -220,  -220,
    -220,  -220,  -220,  -220,  -220,  -153,  -220,  -220,  -220,  -220,
    -220,    21,  -220,  -220,  -220,  -220,  -220,  -220,  -220,  -220,
     119,  -220,  -220,  -220,  -220,  -220,  -220,  -220,  -220,  -220,
    -220,  -220,  -220,  -220,  -220,  -220,  -220,  -220,  -220,  -220,
    -220,  -220,  -220,  -220,  -220,  -220,  -220,  -220,  -220,  -220,
    -220,  -220,   382,  -220,  -220,  -220,  -220,  -220,  -220,  -220,
    -220,  -210,  -220,  -220,  -220,  -220,  -220,  -220,  -220,  -220,
    -220,  -220,  -220,  -219,  -220,  -220,   -15,  -220,  -101,    -5,
      -1
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -282
static const yytype_int16 yytable[] =
{
      81,    97,   304,   231,   -14,   -16,   -18,    82,   323,    95,
     297,   362,   309,   312,   100,   317,    98,   320,   328,   330,
     334,   337,    73,   345,   110,   111,   376,   378,   430,   432,
     116,   117,   464,   468,   119,   120,   472,   118,   479,     3,
     410,   248,   127,   138,   139,   140,    84,    85,   324,    82,
     152,   369,    73,   417,   418,    87,     6,    89,   -87,   421,
     422,   146,   395,   396,   154,   305,   -38,  -109,    84,    85,
     271,  -112,   397,   398,   155,    84,    85,  -122,   -97,  -265,
     160,  -268,   -94,  -150,   325,   300,  -147,  -102,   -41,   399,
     -44,   -47,   249,   400,   401,   402,   403,   404,   405,   175,
     -50,  -125,   -83,    84,    85,    82,    73,   156,    73,    73,
      73,    73,    73,   158,    73,   195,    73,    73,   163,    73,
     165,    73,    73,    73,    73,    73,   203,    73,   167,   406,
      73,    73,    73,    73,   382,   383,    73,    73,   171,   173,
      73,   222,    73,   185,   384,   385,   187,   301,   302,   186,
     191,   193,   201,   232,   235,   205,   207,    73,   236,   237,
     209,   386,   274,   223,   224,   387,   388,   389,   390,   391,
     392,   227,   275,   238,   233,   254,   261,   258,   264,   259,
     262,   283,   368,  -281,   409,   380,  -244,   424,   276,   277,
     278,   279,   428,   274,   438,   239,   240,   280,   241,   452,
     458,   393,   268,   275,   269,   455,   242,   243,   244,   245,
     246,   456,   -20,   169,   482,   265,   451,   457,   371,   276,
     277,   278,   279,     0,     0,     0,     0,     0,   282,     0,
       0,   285,   286,   287,   288,   289,   290,   291,   292,   293,
     294,   295,     0,     0,   296,     0,   299,     0,   307,     0,
       0,   311,   314,     0,     0,   318,     0,   315,     0,     0,
       0,   322,   327,   329,   331,     0,     0,   336,   339,   448,
       0,     0,     0,     0,   340,   341,     0,     0,   344,   347,
     348,   349,   350,   351,   352,   353,   354,   355,   356,   357,
     358,     0,     0,   364,   359,     0,   360,   361,     0,     0,
       0,   274,     0,     0,     0,     0,     0,     0,     0,   365,
       0,   275,     0,   274,   366,     0,     0,     0,     0,   373,
     374,     0,     0,   275,   377,     0,   379,   276,   277,   278,
     279,   381,   411,     0,     0,     0,   463,     0,   408,   276,
     277,   278,   279,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     413,   416,     0,     0,   414,     0,     0,   420,     0,     0,
       0,     0,   431,     0,   433,     0,     0,   434,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     435,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   437,   447,   449,     0,     0,
       0,   443,   440,     0,     0,     0,   444,   445,   446,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   462,     0,
       0,     0,     0,     0,     0,   466,     0,     0,   477,   470,
       0,     0,     0,   471,   474,     0,     0,     0,     0,     0,
       0,   478,   481,     0,     0,     0,     0,   486,     0,   483,
       0,     0,     0,   485,     0,     0,     0,     0,     0,     0,
     487,     0,   488,   176,   177,   178,   179,   180,   181,   182,
     183,   184,     0,     0,     0,   188,   189,   190,     0,     0,
       0,     0,     0,   196,   197,   198,   199,   200,     0,     0,
     204,     0,     0,     0,   211,   212,   213,   214,   215,   216,
      -2,     4,     0,   217,   218,   219,   220,   221,     5,     6,
       7,   226,     0,   229,     0,     8,     9,    10,    11,    12,
      13,    14,    15,     0,     0,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    31,    32,    33,    34,     0,     0,     0,     0,     0,
       0,    35,    36,     0,    37,    38,     0,     0,     0,     0,
      39,    40,     0,     0,    41,    42,    43,     0,    44,    45,
       0,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   332,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   367,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   441,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   460,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   475,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   484
};

static const yytype_int16 yycheck[] =
{
       5,    16,     4,   156,     3,     3,     3,    63,     4,    14,
       4,     4,     4,     4,    19,     4,    17,     4,     4,     4,
       4,     4,   108,     4,    29,    30,     4,     4,     4,     4,
      35,    36,     4,     4,    39,    40,     4,    38,     4,     0,
       5,     4,    47,    58,    59,    60,    68,    69,    44,    63,
      50,     4,   108,    11,    12,     3,     9,     3,     3,    49,
      50,    66,    13,    14,    79,    67,     3,     3,    68,    69,
     223,     3,    23,    24,    79,    68,    69,     3,     3,     3,
      85,     3,     3,     3,    80,     4,     3,     3,     3,    40,
       3,     3,    55,    44,    45,    46,    47,    48,    49,   100,
       3,     3,     3,    68,    69,    63,   108,    50,   108,   108,
     108,   108,   108,     5,   108,   120,   108,   108,     3,   108,
       3,   108,   108,   108,   108,   108,   127,   108,     3,    80,
     108,   108,   108,   108,    13,    14,   108,   108,     3,     3,
     108,   146,   108,    72,    23,    24,     3,    66,    67,    72,
      50,    50,     3,   158,     4,     3,     3,   108,     8,     9,
       3,    40,    14,     3,     3,    44,    45,    46,    47,    48,
      49,     3,    24,    23,     3,     3,   191,    72,   193,    72,
       3,    49,    73,     3,     6,     4,     3,     3,    40,    41,
      42,    43,     3,    14,     3,    45,    46,    49,    48,     3,
      49,    80,   207,    24,   209,     4,    56,    57,    58,    59,
      60,     4,     3,    94,     6,   194,   426,   436,   319,    40,
      41,    42,    43,    -1,    -1,    -1,    -1,    -1,    49,    -1,
      -1,   236,   237,   238,   239,   240,   241,   242,   243,   244,
     245,   246,    -1,    -1,   249,    -1,   251,    -1,   253,    -1,
      -1,   256,   257,    -1,    -1,   260,    -1,   258,    -1,    -1,
      -1,   266,   267,   268,   269,    -1,    -1,   272,   273,   422,
      -1,    -1,    -1,    -1,   275,   276,    -1,    -1,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,    -1,    -1,   308,   299,    -1,   301,   302,    -1,    -1,
      -1,    14,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   314,
      -1,    24,    -1,    14,   315,    -1,    -1,    -1,    -1,   324,
     325,    -1,    -1,    24,   329,    -1,   331,    40,    41,    42,
      43,   336,   347,    -1,    -1,    -1,    49,    -1,   343,    40,
      41,    42,    43,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     365,   366,    -1,    -1,   365,    -1,    -1,   368,    -1,    -1,
      -1,    -1,   377,    -1,   379,    -1,    -1,   392,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     405,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   410,   421,   422,    -1,    -1,
      -1,   416,   413,    -1,    -1,    -1,   417,   418,   419,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   443,    -1,
      -1,    -1,    -1,    -1,    -1,   450,    -1,    -1,   463,   454,
      -1,    -1,    -1,   458,   459,    -1,    -1,    -1,    -1,    -1,
      -1,   466,   467,    -1,    -1,    -1,    -1,   482,    -1,   474,
      -1,    -1,    -1,   478,    -1,    -1,    -1,    -1,    -1,    -1,
     485,    -1,   487,   101,   102,   103,   104,   105,   106,   107,
     108,   109,    -1,    -1,    -1,   113,   114,   115,    -1,    -1,
      -1,    -1,    -1,   121,   122,   123,   124,   125,    -1,    -1,
     128,    -1,    -1,    -1,   132,   133,   134,   135,   136,   137,
       0,     1,    -1,   141,   142,   143,   144,   145,     8,     9,
      10,   149,    -1,   151,    -1,    15,    16,    17,    18,    19,
      20,    21,    22,    -1,    -1,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    51,    52,    53,    54,    -1,    -1,    -1,    -1,    -1,
      -1,    61,    62,    -1,    64,    65,    -1,    -1,    -1,    -1,
      70,    71,    -1,    -1,    74,    75,    76,    -1,    78,    79,
      -1,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   270,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   316,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   415,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   442,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   461,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   476
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,   110,   111,     0,     1,     8,     9,    10,    15,    16,
      17,    18,    19,    20,    21,    22,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    51,    52,    53,    54,    61,    62,    64,    65,    70,
      71,    74,    75,    76,    78,    79,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   112,   168,   169,   170,   205,   247,
     248,   248,    63,   249,    68,    69,   245,     3,   186,     3,
     183,   159,   160,   161,   162,   248,   142,   245,   249,   141,
     248,   140,   152,   121,   128,   147,   149,   150,   154,   144,
     248,   248,   206,   207,   153,   143,   248,   248,   249,   248,
     248,   164,   165,   163,   146,   145,   129,   248,   132,   134,
     119,   120,   135,   122,   123,   124,   125,   139,   245,   245,
     245,   136,   137,   148,   155,   133,   248,   130,   166,   138,
     167,   151,    50,   172,   245,   248,    50,   173,     5,   157,
     248,   187,   184,     3,   236,     3,   189,     3,   193,   189,
     158,     3,   221,     3,   209,   249,   221,   221,   221,   221,
     221,   221,   221,   221,   221,    72,    72,     3,   221,   221,
     221,    50,   126,    50,   127,   248,   221,   221,   221,   221,
     221,     3,   212,   249,   221,     3,   224,     3,   219,     3,
     220,   221,   221,   221,   221,   221,   221,   221,   221,   221,
     221,   221,   248,     3,     3,   199,   221,     3,   202,   221,
     174,   174,   248,     3,   242,     4,     8,     9,    23,    45,
      46,    48,    56,    57,    58,    59,    60,   188,     4,    55,
     185,   237,   190,   194,     3,   239,   222,   210,    72,    72,
     208,   245,     3,   180,   245,   180,   213,   225,   248,   248,
     118,   174,   200,   203,    14,    24,    40,    41,    42,    43,
      49,   175,    49,    49,   243,   248,   248,   248,   248,   248,
     248,   248,   248,   248,   248,   248,   248,     4,   238,   248,
       4,    66,    67,   191,     4,    67,   195,   248,   240,     4,
     223,   248,     4,   211,   248,   249,   117,     4,   248,   181,
       4,   214,   248,     4,    44,    80,   226,   248,     4,   248,
       4,   248,   221,   131,     4,   201,   248,     4,   204,   248,
     249,   249,   176,   178,   248,     4,   244,   248,   248,   248,
     248,   248,   248,   248,   248,   248,   248,   248,   248,   248,
     248,   248,     4,   241,   245,   248,   249,   221,    73,     4,
     182,   247,   215,   248,   248,   227,     4,   248,     4,   248,
       4,   248,    13,    14,    23,    24,    40,    44,    45,    46,
      47,    48,    49,    80,   177,    13,    14,    23,    24,    40,
      44,    45,    46,    47,    48,    49,    80,   179,   248,     6,
       5,   245,   192,   248,   249,   113,   248,    11,    12,   246,
     249,    49,    50,   171,     3,   216,   228,   229,     3,   230,
       4,   248,     4,   248,   245,   245,   156,   248,     3,   196,
     249,   221,   114,   248,   249,   249,   249,   245,   174,   245,
     217,   230,     3,   233,   231,     4,     4,   242,    49,   197,
     221,   115,   248,    49,     4,   218,   248,   234,     4,   232,
     248,   248,     4,   198,   248,   221,   116,   245,   248,     4,
     235,   248,     6,   248,   221,   248,   245,   248,   248
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
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

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
# if YYLTYPE_IS_TRIVIAL
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
        case 10:

/* Line 1455 of yacc.c  */
#line 166 "gram.y"
    {
		      (void) AddIconRegion((yyvsp[(2) - (6)].ptr), (yyvsp[(3) - (6)].num), (yyvsp[(4) - (6)].num), (yyvsp[(5) - (6)].num), (yyvsp[(6) - (6)].num), "undef", "undef", "undef");
		  }
    break;

  case 11:

/* Line 1455 of yacc.c  */
#line 169 "gram.y"
    {
		      (void) AddIconRegion((yyvsp[(2) - (7)].ptr), (yyvsp[(3) - (7)].num), (yyvsp[(4) - (7)].num), (yyvsp[(5) - (7)].num), (yyvsp[(6) - (7)].num), (yyvsp[(7) - (7)].ptr), "undef", "undef");
		  }
    break;

  case 12:

/* Line 1455 of yacc.c  */
#line 172 "gram.y"
    {
		      (void) AddIconRegion((yyvsp[(2) - (8)].ptr), (yyvsp[(3) - (8)].num), (yyvsp[(4) - (8)].num), (yyvsp[(5) - (8)].num), (yyvsp[(6) - (8)].num), (yyvsp[(7) - (8)].ptr), (yyvsp[(8) - (8)].ptr), "undef");
		  }
    break;

  case 13:

/* Line 1455 of yacc.c  */
#line 175 "gram.y"
    {
		      (void) AddIconRegion((yyvsp[(2) - (9)].ptr), (yyvsp[(3) - (9)].num), (yyvsp[(4) - (9)].num), (yyvsp[(5) - (9)].num), (yyvsp[(6) - (9)].num), (yyvsp[(7) - (9)].ptr), (yyvsp[(8) - (9)].ptr), (yyvsp[(9) - (9)].ptr));
		  }
    break;

  case 14:

/* Line 1455 of yacc.c  */
#line 178 "gram.y"
    {
		      list = AddIconRegion((yyvsp[(2) - (6)].ptr), (yyvsp[(3) - (6)].num), (yyvsp[(4) - (6)].num), (yyvsp[(5) - (6)].num), (yyvsp[(6) - (6)].num), "undef", "undef", "undef");
		  }
    break;

  case 16:

/* Line 1455 of yacc.c  */
#line 182 "gram.y"
    {
		      list = AddIconRegion((yyvsp[(2) - (7)].ptr), (yyvsp[(3) - (7)].num), (yyvsp[(4) - (7)].num), (yyvsp[(5) - (7)].num), (yyvsp[(6) - (7)].num), (yyvsp[(7) - (7)].ptr), "undef", "undef");
		  }
    break;

  case 18:

/* Line 1455 of yacc.c  */
#line 186 "gram.y"
    {
		      list = AddIconRegion((yyvsp[(2) - (8)].ptr), (yyvsp[(3) - (8)].num), (yyvsp[(4) - (8)].num), (yyvsp[(5) - (8)].num), (yyvsp[(6) - (8)].num), (yyvsp[(7) - (8)].ptr), (yyvsp[(8) - (8)].ptr), "undef");
		  }
    break;

  case 20:

/* Line 1455 of yacc.c  */
#line 190 "gram.y"
    {
		      list = AddIconRegion((yyvsp[(2) - (9)].ptr), (yyvsp[(3) - (9)].num), (yyvsp[(4) - (9)].num), (yyvsp[(5) - (9)].num), (yyvsp[(6) - (9)].num), (yyvsp[(7) - (9)].ptr), (yyvsp[(8) - (9)].ptr), (yyvsp[(9) - (9)].ptr));
		  }
    break;

  case 22:

/* Line 1455 of yacc.c  */
#line 195 "gram.y"
    {
		      list = AddWindowRegion ((yyvsp[(2) - (4)].ptr), (yyvsp[(3) - (4)].num), (yyvsp[(4) - (4)].num));
		  }
    break;

  case 24:

/* Line 1455 of yacc.c  */
#line 200 "gram.y"
    {
		      list = addWindowBox ((yyvsp[(2) - (3)].ptr), (yyvsp[(3) - (3)].ptr));
		  }
    break;

  case 26:

/* Line 1455 of yacc.c  */
#line 205 "gram.y"
    { if (Scr->FirstTime)
						  {
						    Scr->iconmgr->geometry= (char*)(yyvsp[(2) - (3)].ptr);
						    Scr->iconmgr->columns=(yyvsp[(3) - (3)].num);
						  }
						}
    break;

  case 27:

/* Line 1455 of yacc.c  */
#line 211 "gram.y"
    { if (Scr->FirstTime)
						    Scr->iconmgr->geometry = (char*)(yyvsp[(2) - (2)].ptr);
						}
    break;

  case 28:

/* Line 1455 of yacc.c  */
#line 214 "gram.y"
    { if (Scr->FirstTime)
				{
				    Scr->workSpaceMgr.geometry= (char*)(yyvsp[(2) - (3)].ptr);
				    Scr->workSpaceMgr.columns=(yyvsp[(3) - (3)].num);
				}
						}
    break;

  case 29:

/* Line 1455 of yacc.c  */
#line 220 "gram.y"
    { if (Scr->FirstTime)
				    Scr->workSpaceMgr.geometry = (char*)(yyvsp[(2) - (2)].ptr);
						}
    break;

  case 30:

/* Line 1455 of yacc.c  */
#line 223 "gram.y"
    {}
    break;

  case 32:

/* Line 1455 of yacc.c  */
#line 226 "gram.y"
    {}
    break;

  case 34:

/* Line 1455 of yacc.c  */
#line 229 "gram.y"
    { if (Scr->FirstTime)
					  {
						Scr->DoZoom = TRUE;
						Scr->ZoomCount = (yyvsp[(2) - (2)].num);
					  }
					}
    break;

  case 35:

/* Line 1455 of yacc.c  */
#line 235 "gram.y"
    { if (Scr->FirstTime)
						Scr->DoZoom = TRUE; }
    break;

  case 36:

/* Line 1455 of yacc.c  */
#line 237 "gram.y"
    {}
    break;

  case 37:

/* Line 1455 of yacc.c  */
#line 238 "gram.y"
    {}
    break;

  case 38:

/* Line 1455 of yacc.c  */
#line 239 "gram.y"
    { list = &Scr->IconifyByUn; }
    break;

  case 40:

/* Line 1455 of yacc.c  */
#line 241 "gram.y"
    { if (Scr->FirstTime)
		    Scr->IconifyByUnmapping = TRUE; }
    break;

  case 41:

/* Line 1455 of yacc.c  */
#line 244 "gram.y"
    { list = &Scr->OpaqueMoveList; }
    break;

  case 43:

/* Line 1455 of yacc.c  */
#line 246 "gram.y"
    { if (Scr->FirstTime) Scr->DoOpaqueMove = TRUE; }
    break;

  case 44:

/* Line 1455 of yacc.c  */
#line 247 "gram.y"
    { list = &Scr->NoOpaqueMoveList; }
    break;

  case 46:

/* Line 1455 of yacc.c  */
#line 249 "gram.y"
    { if (Scr->FirstTime) Scr->DoOpaqueMove = FALSE; }
    break;

  case 47:

/* Line 1455 of yacc.c  */
#line 250 "gram.y"
    { list = &Scr->OpaqueMoveList; }
    break;

  case 49:

/* Line 1455 of yacc.c  */
#line 252 "gram.y"
    { if (Scr->FirstTime) Scr->DoOpaqueResize = TRUE; }
    break;

  case 50:

/* Line 1455 of yacc.c  */
#line 253 "gram.y"
    { list = &Scr->NoOpaqueResizeList; }
    break;

  case 52:

/* Line 1455 of yacc.c  */
#line 255 "gram.y"
    { if (Scr->FirstTime) Scr->DoOpaqueResize = FALSE; }
    break;

  case 53:

/* Line 1455 of yacc.c  */
#line 257 "gram.y"
    {
					  GotTitleButton ((yyvsp[(2) - (4)].ptr), (yyvsp[(4) - (4)].num), False);
					}
    break;

  case 54:

/* Line 1455 of yacc.c  */
#line 260 "gram.y"
    {
					  GotTitleButton ((yyvsp[(2) - (4)].ptr), (yyvsp[(4) - (4)].num), True);
					}
    break;

  case 55:

/* Line 1455 of yacc.c  */
#line 263 "gram.y"
    { CreateTitleButton((yyvsp[(2) - (2)].ptr), 0, NULL, NULL, FALSE, TRUE); }
    break;

  case 57:

/* Line 1455 of yacc.c  */
#line 265 "gram.y"
    { CreateTitleButton((yyvsp[(2) - (2)].ptr), 0, NULL, NULL, TRUE, TRUE); }
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 267 "gram.y"
    {
		    root = GetRoot((yyvsp[(2) - (2)].ptr), NULLSTR, NULLSTR);
		    AddFuncButton ((yyvsp[(1) - (2)].num), C_ROOT, 0, F_MENU, root, (MenuItem*) 0);
		}
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 271 "gram.y"
    {
			if ((yyvsp[(2) - (2)].num) == F_MENU) {
			    pull->prev = NULL;
			    AddFuncButton ((yyvsp[(1) - (2)].num), C_ROOT, 0, (yyvsp[(2) - (2)].num), pull, (MenuItem*) 0);
			}
			else {
			    MenuItem *item;

			    root = GetRoot(TWM_ROOT,NULLSTR,NULLSTR);
			    item = AddToMenu (root, "x", Action,
					NULL, (yyvsp[(2) - (2)].num), NULLSTR, NULLSTR);
			    AddFuncButton ((yyvsp[(1) - (2)].num), C_ROOT, 0, (yyvsp[(2) - (2)].num), (MenuRoot*) 0, item);
			}
			Action = "";
			pull = NULL;
		}
    break;

  case 61:

/* Line 1455 of yacc.c  */
#line 287 "gram.y"
    { GotKey((yyvsp[(1) - (2)].ptr), (yyvsp[(2) - (2)].num)); }
    break;

  case 62:

/* Line 1455 of yacc.c  */
#line 288 "gram.y"
    { GotButton((yyvsp[(1) - (2)].num), (yyvsp[(2) - (2)].num)); }
    break;

  case 63:

/* Line 1455 of yacc.c  */
#line 290 "gram.y"
    { list = &Scr->DontIconify; }
    break;

  case 65:

/* Line 1455 of yacc.c  */
#line 292 "gram.y"
    {}
    break;

  case 67:

/* Line 1455 of yacc.c  */
#line 294 "gram.y"
    {}
    break;

  case 68:

/* Line 1455 of yacc.c  */
#line 294 "gram.y"
    { Scr->IgnoreModifier = mods; mods = 0; }
    break;

  case 70:

/* Line 1455 of yacc.c  */
#line 295 "gram.y"
    { list = &Scr->OccupyAll; }
    break;

  case 72:

/* Line 1455 of yacc.c  */
#line 297 "gram.y"
    { list = &Scr->IconMenuDontShow; }
    break;

  case 74:

/* Line 1455 of yacc.c  */
#line 299 "gram.y"
    {}
    break;

  case 76:

/* Line 1455 of yacc.c  */
#line 301 "gram.y"
    { list = &Scr->UnmapByMovingFarAway; }
    break;

  case 78:

/* Line 1455 of yacc.c  */
#line 303 "gram.y"
    { list = &Scr->AutoSqueeze; }
    break;

  case 80:

/* Line 1455 of yacc.c  */
#line 305 "gram.y"
    { list = &Scr->StartSqueezed; }
    break;

  case 82:

/* Line 1455 of yacc.c  */
#line 307 "gram.y"
    { Scr->AlwaysSqueezeToGravity = TRUE; }
    break;

  case 83:

/* Line 1455 of yacc.c  */
#line 308 "gram.y"
    { list = &Scr->AlwaysSqueezeToGravityL; }
    break;

  case 85:

/* Line 1455 of yacc.c  */
#line 310 "gram.y"
    { list = &Scr->DontSetInactive; }
    break;

  case 87:

/* Line 1455 of yacc.c  */
#line 312 "gram.y"
    { list = &Scr->IconMgrNoShow; }
    break;

  case 89:

/* Line 1455 of yacc.c  */
#line 314 "gram.y"
    { Scr->IconManagerDontShow = TRUE; }
    break;

  case 90:

/* Line 1455 of yacc.c  */
#line 315 "gram.y"
    { list = &Scr->IconMgrs; }
    break;

  case 92:

/* Line 1455 of yacc.c  */
#line 317 "gram.y"
    { list = &Scr->IconMgrShow; }
    break;

  case 94:

/* Line 1455 of yacc.c  */
#line 319 "gram.y"
    { list = &Scr->NoTitleHighlight; }
    break;

  case 96:

/* Line 1455 of yacc.c  */
#line 321 "gram.y"
    { if (Scr->FirstTime)
						Scr->TitleHighlight = FALSE; }
    break;

  case 97:

/* Line 1455 of yacc.c  */
#line 323 "gram.y"
    { list = &Scr->NoHighlight; }
    break;

  case 99:

/* Line 1455 of yacc.c  */
#line 325 "gram.y"
    { if (Scr->FirstTime)
						Scr->Highlight = FALSE; }
    break;

  case 100:

/* Line 1455 of yacc.c  */
#line 327 "gram.y"
    { list = &Scr->AlwaysOnTopL; }
    break;

  case 102:

/* Line 1455 of yacc.c  */
#line 329 "gram.y"
    { list = &Scr->NoStackModeL; }
    break;

  case 104:

/* Line 1455 of yacc.c  */
#line 331 "gram.y"
    { if (Scr->FirstTime)
						Scr->StackMode = FALSE; }
    break;

  case 105:

/* Line 1455 of yacc.c  */
#line 333 "gram.y"
    { list = &Scr->NoBorder; }
    break;

  case 107:

/* Line 1455 of yacc.c  */
#line 335 "gram.y"
    { list = &Scr->DontSave; }
    break;

  case 109:

/* Line 1455 of yacc.c  */
#line 337 "gram.y"
    { list = &Scr->NoIconTitle; }
    break;

  case 111:

/* Line 1455 of yacc.c  */
#line 339 "gram.y"
    { if (Scr->FirstTime)
						Scr->NoIconTitlebar = TRUE; }
    break;

  case 112:

/* Line 1455 of yacc.c  */
#line 341 "gram.y"
    { list = &Scr->NoTitle; }
    break;

  case 114:

/* Line 1455 of yacc.c  */
#line 343 "gram.y"
    { if (Scr->FirstTime)
						Scr->NoTitlebar = TRUE; }
    break;

  case 115:

/* Line 1455 of yacc.c  */
#line 345 "gram.y"
    { list = &Scr->IgnoreTransientL; }
    break;

  case 117:

/* Line 1455 of yacc.c  */
#line 347 "gram.y"
    { donttoggleworkspacemanagerstate = TRUE; }
    break;

  case 118:

/* Line 1455 of yacc.c  */
#line 348 "gram.y"
    { list = &Scr->MakeTitle; }
    break;

  case 120:

/* Line 1455 of yacc.c  */
#line 350 "gram.y"
    { list = &Scr->StartIconified; }
    break;

  case 122:

/* Line 1455 of yacc.c  */
#line 352 "gram.y"
    { list = &Scr->AutoRaise; }
    break;

  case 124:

/* Line 1455 of yacc.c  */
#line 354 "gram.y"
    { Scr->AutoRaiseDefault = TRUE; }
    break;

  case 125:

/* Line 1455 of yacc.c  */
#line 355 "gram.y"
    { list = &Scr->AutoLower; }
    break;

  case 127:

/* Line 1455 of yacc.c  */
#line 357 "gram.y"
    { Scr->AutoLowerDefault = TRUE; }
    break;

  case 128:

/* Line 1455 of yacc.c  */
#line 358 "gram.y"
    {
					root = GetRoot((yyvsp[(2) - (7)].ptr), (yyvsp[(4) - (7)].ptr), (yyvsp[(6) - (7)].ptr)); }
    break;

  case 129:

/* Line 1455 of yacc.c  */
#line 360 "gram.y"
    { root->real_menu = TRUE;}
    break;

  case 130:

/* Line 1455 of yacc.c  */
#line 361 "gram.y"
    { root = GetRoot((yyvsp[(2) - (2)].ptr), NULLSTR, NULLSTR); }
    break;

  case 131:

/* Line 1455 of yacc.c  */
#line 362 "gram.y"
    { root->real_menu = TRUE; }
    break;

  case 132:

/* Line 1455 of yacc.c  */
#line 363 "gram.y"
    { root = GetRoot((yyvsp[(2) - (2)].ptr), NULLSTR, NULLSTR); }
    break;

  case 134:

/* Line 1455 of yacc.c  */
#line 365 "gram.y"
    { list = &Scr->IconNames; }
    break;

  case 136:

/* Line 1455 of yacc.c  */
#line 367 "gram.y"
    { color = COLOR; }
    break;

  case 138:

/* Line 1455 of yacc.c  */
#line 369 "gram.y"
    {}
    break;

  case 140:

/* Line 1455 of yacc.c  */
#line 371 "gram.y"
    { color = MONOCHROME; }
    break;

  case 142:

/* Line 1455 of yacc.c  */
#line 373 "gram.y"
    { Scr->DefaultFunction.func = (yyvsp[(2) - (2)].num);
					  if ((yyvsp[(2) - (2)].num) == F_MENU)
					  {
					    pull->prev = NULL;
					    Scr->DefaultFunction.menu = pull;
					  }
					  else
					  {
					    root = GetRoot(TWM_ROOT,NULLSTR,NULLSTR);
					    Scr->DefaultFunction.item =
						AddToMenu(root,"x",Action,
							  NULL,(yyvsp[(2) - (2)].num), NULLSTR, NULLSTR);
					  }
					  Action = "";
					  pull = NULL;
					}
    break;

  case 143:

/* Line 1455 of yacc.c  */
#line 389 "gram.y"
    { Scr->WindowFunction.func = (yyvsp[(2) - (2)].num);
					   root = GetRoot(TWM_ROOT,NULLSTR,NULLSTR);
					   Scr->WindowFunction.item =
						AddToMenu(root,"x",Action,
							  NULL,(yyvsp[(2) - (2)].num), NULLSTR, NULLSTR);
					   Action = "";
					   pull = NULL;
					}
    break;

  case 144:

/* Line 1455 of yacc.c  */
#line 397 "gram.y"
    { Scr->ChangeWorkspaceFunction.func = (yyvsp[(2) - (2)].num);
					   root = GetRoot(TWM_ROOT,NULLSTR,NULLSTR);
					   Scr->ChangeWorkspaceFunction.item =
						AddToMenu(root,"x",Action,
							  NULL,(yyvsp[(2) - (2)].num), NULLSTR, NULLSTR);
					   Action = "";
					   pull = NULL;
					}
    break;

  case 145:

/* Line 1455 of yacc.c  */
#line 405 "gram.y"
    { Scr->DeIconifyFunction.func = (yyvsp[(2) - (2)].num);
					   root = GetRoot(TWM_ROOT,NULLSTR,NULLSTR);
					   Scr->DeIconifyFunction.item =
						AddToMenu(root,"x",Action,
							  NULL,(yyvsp[(2) - (2)].num), NULLSTR, NULLSTR);
					   Action = "";
					   pull = NULL;
					}
    break;

  case 146:

/* Line 1455 of yacc.c  */
#line 413 "gram.y"
    { Scr->IconifyFunction.func = (yyvsp[(2) - (2)].num);
					   root = GetRoot(TWM_ROOT,NULLSTR,NULLSTR);
					   Scr->IconifyFunction.item =
						AddToMenu(root,"x",Action,
							  NULL,(yyvsp[(2) - (2)].num), NULLSTR, NULLSTR);
					   Action = "";
					   pull = NULL;
					}
    break;

  case 147:

/* Line 1455 of yacc.c  */
#line 421 "gram.y"
    { list = &Scr->WarpCursorL; }
    break;

  case 149:

/* Line 1455 of yacc.c  */
#line 423 "gram.y"
    { if (Scr->FirstTime)
					    Scr->WarpCursor = TRUE; }
    break;

  case 150:

/* Line 1455 of yacc.c  */
#line 425 "gram.y"
    { list = &Scr->WindowRingL; }
    break;

  case 152:

/* Line 1455 of yacc.c  */
#line 427 "gram.y"
    { Scr->WindowRingAll = TRUE; }
    break;

  case 153:

/* Line 1455 of yacc.c  */
#line 428 "gram.y"
    { if (!Scr->WindowRingL)
					    Scr->WindowRingAll = TRUE;
					  list = &Scr->WindowRingExcludeL; }
    break;

  case 155:

/* Line 1455 of yacc.c  */
#line 432 "gram.y"
    {  }
    break;

  case 157:

/* Line 1455 of yacc.c  */
#line 434 "gram.y"
    { }
    break;

  case 159:

/* Line 1455 of yacc.c  */
#line 438 "gram.y"
    { if (!do_single_keyword ((yyvsp[(1) - (1)].num))) {
					    twmrc_error_prefix();
					    fprintf (stderr,
					"unknown singleton keyword %d\n",
						     (yyvsp[(1) - (1)].num));
					    ParseError = 1;
					  }
					}
    break;

  case 160:

/* Line 1455 of yacc.c  */
#line 448 "gram.y"
    { if (!do_string_keyword ((yyvsp[(1) - (2)].num), (yyvsp[(2) - (2)].ptr))) {
					    twmrc_error_prefix();
					    fprintf (stderr,
				"unknown string keyword %d (value \"%s\")\n",
						     (yyvsp[(1) - (2)].num), (yyvsp[(2) - (2)].ptr));
					    ParseError = 1;
					  }
					}
    break;

  case 161:

/* Line 1455 of yacc.c  */
#line 456 "gram.y"
    { if (!do_string_keyword ((yyvsp[(1) - (1)].num), defstring)) {
					    twmrc_error_prefix();
					    fprintf (stderr,
				"unknown string keyword %d (no value)\n",
						     (yyvsp[(1) - (1)].num));
					    ParseError = 1;
					  }
					}
    break;

  case 162:

/* Line 1455 of yacc.c  */
#line 467 "gram.y"
    { if (!do_string_string_keyword ((yyvsp[(1) - (3)].num), (yyvsp[(2) - (3)].ptr), (yyvsp[(3) - (3)].ptr))) {
					    twmrc_error_prefix();
					    fprintf (stderr,
				"unknown strings keyword %d (value \"%s\" and \"%s\")\n",
						     (yyvsp[(1) - (3)].num), (yyvsp[(2) - (3)].ptr), (yyvsp[(3) - (3)].ptr));
					    ParseError = 1;
					  }
					}
    break;

  case 163:

/* Line 1455 of yacc.c  */
#line 475 "gram.y"
    { if (!do_string_string_keyword ((yyvsp[(1) - (2)].num), (yyvsp[(2) - (2)].ptr), defstring)) {
					    twmrc_error_prefix();
					    fprintf (stderr,
				"unknown string keyword %d (value \"%s\")\n",
						     (yyvsp[(1) - (2)].num), (yyvsp[(2) - (2)].ptr));
					    ParseError = 1;
					  }
					}
    break;

  case 164:

/* Line 1455 of yacc.c  */
#line 483 "gram.y"
    { if (!do_string_string_keyword ((yyvsp[(1) - (1)].num), defstring, defstring)) {
					    twmrc_error_prefix();
					    fprintf (stderr,
				"unknown string keyword %d (no value)\n",
						     (yyvsp[(1) - (1)].num));
					    ParseError = 1;
					  }
					}
    break;

  case 165:

/* Line 1455 of yacc.c  */
#line 493 "gram.y"
    { if (!do_number_keyword ((yyvsp[(1) - (2)].num), (yyvsp[(2) - (2)].num))) {
					    twmrc_error_prefix();
					    fprintf (stderr,
				"unknown numeric keyword %d (value %d)\n",
						     (yyvsp[(1) - (2)].num), (yyvsp[(2) - (2)].num));
					    ParseError = 1;
					  }
					}
    break;

  case 166:

/* Line 1455 of yacc.c  */
#line 505 "gram.y"
    { (yyval.num) = (yyvsp[(4) - (4)].num); }
    break;

  case 167:

/* Line 1455 of yacc.c  */
#line 508 "gram.y"
    { (yyval.num) = (yyvsp[(6) - (6)].num); }
    break;

  case 168:

/* Line 1455 of yacc.c  */
#line 511 "gram.y"
    { (yyval.num) = (yyvsp[(6) - (6)].num); }
    break;

  case 171:

/* Line 1455 of yacc.c  */
#line 518 "gram.y"
    { mods |= Mod1Mask; }
    break;

  case 172:

/* Line 1455 of yacc.c  */
#line 519 "gram.y"
    { mods |= ShiftMask; }
    break;

  case 173:

/* Line 1455 of yacc.c  */
#line 520 "gram.y"
    { mods |= LockMask; }
    break;

  case 174:

/* Line 1455 of yacc.c  */
#line 521 "gram.y"
    { mods |= ControlMask; }
    break;

  case 175:

/* Line 1455 of yacc.c  */
#line 522 "gram.y"
    { if ((yyvsp[(2) - (2)].num) < 1 || (yyvsp[(2) - (2)].num) > 5) {
					     twmrc_error_prefix();
					     fprintf (stderr,
				"bad modifier number (%d), must be 1-5\n",
						      (yyvsp[(2) - (2)].num));
					     ParseError = 1;
					  } else {
					     mods |= (Alt1Mask << ((yyvsp[(2) - (2)].num) - 1));
					  }
					}
    break;

  case 176:

/* Line 1455 of yacc.c  */
#line 532 "gram.y"
    { if ((yyvsp[(2) - (2)].num) < 1 || (yyvsp[(2) - (2)].num) > 5) {
					     twmrc_error_prefix();
					     fprintf (stderr,
				"bad modifier number (%d), must be 1-5\n",
						      (yyvsp[(2) - (2)].num));
					     ParseError = 1;
					  } else {
					     mods |= (Mod1Mask << ((yyvsp[(2) - (2)].num) - 1));
					  }
					}
    break;

  case 177:

/* Line 1455 of yacc.c  */
#line 542 "gram.y"
    { }
    break;

  case 180:

/* Line 1455 of yacc.c  */
#line 549 "gram.y"
    { cont |= C_WINDOW_BIT; }
    break;

  case 181:

/* Line 1455 of yacc.c  */
#line 550 "gram.y"
    { cont |= C_TITLE_BIT; }
    break;

  case 182:

/* Line 1455 of yacc.c  */
#line 551 "gram.y"
    { cont |= C_ICON_BIT; }
    break;

  case 183:

/* Line 1455 of yacc.c  */
#line 552 "gram.y"
    { cont |= C_ROOT_BIT; }
    break;

  case 184:

/* Line 1455 of yacc.c  */
#line 553 "gram.y"
    { cont |= C_FRAME_BIT; }
    break;

  case 185:

/* Line 1455 of yacc.c  */
#line 554 "gram.y"
    { cont |= C_WORKSPACE_BIT; }
    break;

  case 186:

/* Line 1455 of yacc.c  */
#line 555 "gram.y"
    { cont |= C_ICONMGR_BIT; }
    break;

  case 187:

/* Line 1455 of yacc.c  */
#line 556 "gram.y"
    { cont |= C_ICONMGR_BIT; }
    break;

  case 188:

/* Line 1455 of yacc.c  */
#line 557 "gram.y"
    { cont |= C_ALTER_BIT; }
    break;

  case 189:

/* Line 1455 of yacc.c  */
#line 558 "gram.y"
    { cont |= C_ALL_BITS; }
    break;

  case 190:

/* Line 1455 of yacc.c  */
#line 559 "gram.y"
    {  }
    break;

  case 193:

/* Line 1455 of yacc.c  */
#line 566 "gram.y"
    { cont |= C_WINDOW_BIT; }
    break;

  case 194:

/* Line 1455 of yacc.c  */
#line 567 "gram.y"
    { cont |= C_TITLE_BIT; }
    break;

  case 195:

/* Line 1455 of yacc.c  */
#line 568 "gram.y"
    { cont |= C_ICON_BIT; }
    break;

  case 196:

/* Line 1455 of yacc.c  */
#line 569 "gram.y"
    { cont |= C_ROOT_BIT; }
    break;

  case 197:

/* Line 1455 of yacc.c  */
#line 570 "gram.y"
    { cont |= C_FRAME_BIT; }
    break;

  case 198:

/* Line 1455 of yacc.c  */
#line 571 "gram.y"
    { cont |= C_WORKSPACE_BIT; }
    break;

  case 199:

/* Line 1455 of yacc.c  */
#line 572 "gram.y"
    { cont |= C_ICONMGR_BIT; }
    break;

  case 200:

/* Line 1455 of yacc.c  */
#line 573 "gram.y"
    { cont |= C_ICONMGR_BIT; }
    break;

  case 201:

/* Line 1455 of yacc.c  */
#line 574 "gram.y"
    { cont |= C_ALL_BITS; }
    break;

  case 202:

/* Line 1455 of yacc.c  */
#line 575 "gram.y"
    { cont |= C_ALTER_BIT; }
    break;

  case 203:

/* Line 1455 of yacc.c  */
#line 576 "gram.y"
    { }
    break;

  case 204:

/* Line 1455 of yacc.c  */
#line 577 "gram.y"
    { Name = (char*)(yyvsp[(1) - (1)].ptr); cont |= C_NAME_BIT; }
    break;

  case 205:

/* Line 1455 of yacc.c  */
#line 581 "gram.y"
    {}
    break;

  case 208:

/* Line 1455 of yacc.c  */
#line 588 "gram.y"
    { ModifyCurrentTB((yyvsp[(1) - (2)].num), mods, (yyvsp[(2) - (2)].num), Action, pull); mods = 0;}
    break;

  case 209:

/* Line 1455 of yacc.c  */
#line 589 "gram.y"
    { ModifyCurrentTB((yyvsp[(1) - (3)].num), 0, (yyvsp[(3) - (3)].num), Action, pull);}
    break;

  case 210:

/* Line 1455 of yacc.c  */
#line 591 "gram.y"
    { ModifyCurrentTB((yyvsp[(1) - (3)].num), 0, (yyvsp[(3) - (3)].num), Action, pull);}
    break;

  case 211:

/* Line 1455 of yacc.c  */
#line 595 "gram.y"
    {}
    break;

  case 214:

/* Line 1455 of yacc.c  */
#line 602 "gram.y"
    { SetHighlightPixmap ((yyvsp[(2) - (2)].ptr)); }
    break;

  case 215:

/* Line 1455 of yacc.c  */
#line 606 "gram.y"
    {}
    break;

  case 218:

/* Line 1455 of yacc.c  */
#line 613 "gram.y"
    {
			NewBitmapCursor(&Scr->FrameCursor, (yyvsp[(2) - (3)].ptr), (yyvsp[(3) - (3)].ptr)); }
    break;

  case 219:

/* Line 1455 of yacc.c  */
#line 615 "gram.y"
    {
			NewFontCursor(&Scr->FrameCursor, (yyvsp[(2) - (2)].ptr)); }
    break;

  case 220:

/* Line 1455 of yacc.c  */
#line 617 "gram.y"
    {
			NewBitmapCursor(&Scr->TitleCursor, (yyvsp[(2) - (3)].ptr), (yyvsp[(3) - (3)].ptr)); }
    break;

  case 221:

/* Line 1455 of yacc.c  */
#line 619 "gram.y"
    {
			NewFontCursor(&Scr->TitleCursor, (yyvsp[(2) - (2)].ptr)); }
    break;

  case 222:

/* Line 1455 of yacc.c  */
#line 621 "gram.y"
    {
			NewBitmapCursor(&Scr->IconCursor, (yyvsp[(2) - (3)].ptr), (yyvsp[(3) - (3)].ptr)); }
    break;

  case 223:

/* Line 1455 of yacc.c  */
#line 623 "gram.y"
    {
			NewFontCursor(&Scr->IconCursor, (yyvsp[(2) - (2)].ptr)); }
    break;

  case 224:

/* Line 1455 of yacc.c  */
#line 625 "gram.y"
    {
			NewBitmapCursor(&Scr->IconMgrCursor, (yyvsp[(2) - (3)].ptr), (yyvsp[(3) - (3)].ptr)); }
    break;

  case 225:

/* Line 1455 of yacc.c  */
#line 627 "gram.y"
    {
			NewFontCursor(&Scr->IconMgrCursor, (yyvsp[(2) - (2)].ptr)); }
    break;

  case 226:

/* Line 1455 of yacc.c  */
#line 629 "gram.y"
    {
			NewBitmapCursor(&Scr->ButtonCursor, (yyvsp[(2) - (3)].ptr), (yyvsp[(3) - (3)].ptr)); }
    break;

  case 227:

/* Line 1455 of yacc.c  */
#line 631 "gram.y"
    {
			NewFontCursor(&Scr->ButtonCursor, (yyvsp[(2) - (2)].ptr)); }
    break;

  case 228:

/* Line 1455 of yacc.c  */
#line 633 "gram.y"
    {
			NewBitmapCursor(&Scr->MoveCursor, (yyvsp[(2) - (3)].ptr), (yyvsp[(3) - (3)].ptr)); }
    break;

  case 229:

/* Line 1455 of yacc.c  */
#line 635 "gram.y"
    {
			NewFontCursor(&Scr->MoveCursor, (yyvsp[(2) - (2)].ptr)); }
    break;

  case 230:

/* Line 1455 of yacc.c  */
#line 637 "gram.y"
    {
			NewBitmapCursor(&Scr->ResizeCursor, (yyvsp[(2) - (3)].ptr), (yyvsp[(3) - (3)].ptr)); }
    break;

  case 231:

/* Line 1455 of yacc.c  */
#line 639 "gram.y"
    {
			NewFontCursor(&Scr->ResizeCursor, (yyvsp[(2) - (2)].ptr)); }
    break;

  case 232:

/* Line 1455 of yacc.c  */
#line 641 "gram.y"
    {
			NewBitmapCursor(&Scr->WaitCursor, (yyvsp[(2) - (3)].ptr), (yyvsp[(3) - (3)].ptr)); }
    break;

  case 233:

/* Line 1455 of yacc.c  */
#line 643 "gram.y"
    {
			NewFontCursor(&Scr->WaitCursor, (yyvsp[(2) - (2)].ptr)); }
    break;

  case 234:

/* Line 1455 of yacc.c  */
#line 645 "gram.y"
    {
			NewBitmapCursor(&Scr->MenuCursor, (yyvsp[(2) - (3)].ptr), (yyvsp[(3) - (3)].ptr)); }
    break;

  case 235:

/* Line 1455 of yacc.c  */
#line 647 "gram.y"
    {
			NewFontCursor(&Scr->MenuCursor, (yyvsp[(2) - (2)].ptr)); }
    break;

  case 236:

/* Line 1455 of yacc.c  */
#line 649 "gram.y"
    {
			NewBitmapCursor(&Scr->SelectCursor, (yyvsp[(2) - (3)].ptr), (yyvsp[(3) - (3)].ptr)); }
    break;

  case 237:

/* Line 1455 of yacc.c  */
#line 651 "gram.y"
    {
			NewFontCursor(&Scr->SelectCursor, (yyvsp[(2) - (2)].ptr)); }
    break;

  case 238:

/* Line 1455 of yacc.c  */
#line 653 "gram.y"
    {
			NewBitmapCursor(&Scr->DestroyCursor, (yyvsp[(2) - (3)].ptr), (yyvsp[(3) - (3)].ptr)); }
    break;

  case 239:

/* Line 1455 of yacc.c  */
#line 655 "gram.y"
    {
			NewFontCursor(&Scr->DestroyCursor, (yyvsp[(2) - (2)].ptr)); }
    break;

  case 240:

/* Line 1455 of yacc.c  */
#line 659 "gram.y"
    {}
    break;

  case 243:

/* Line 1455 of yacc.c  */
#line 667 "gram.y"
    { if (!do_colorlist_keyword ((yyvsp[(1) - (2)].num), color,
								     (yyvsp[(2) - (2)].ptr))) {
					    twmrc_error_prefix();
					    fprintf (stderr,
			"unhandled list color keyword %d (string \"%s\")\n",
						     (yyvsp[(1) - (2)].num), (yyvsp[(2) - (2)].ptr));
					    ParseError = 1;
					  }
					}
    break;

  case 244:

/* Line 1455 of yacc.c  */
#line 676 "gram.y"
    { list = do_colorlist_keyword((yyvsp[(1) - (2)].num),color,
								      (yyvsp[(2) - (2)].ptr));
					  if (!list) {
					    twmrc_error_prefix();
					    fprintf (stderr,
			"unhandled color list keyword %d (string \"%s\")\n",
						     (yyvsp[(1) - (2)].num), (yyvsp[(2) - (2)].ptr));
					    ParseError = 1;
					  }
					}
    break;

  case 246:

/* Line 1455 of yacc.c  */
#line 687 "gram.y"
    { if (!do_color_keyword ((yyvsp[(1) - (2)].num), color,
								 (yyvsp[(2) - (2)].ptr))) {
					    twmrc_error_prefix();
					    fprintf (stderr,
			"unhandled color keyword %d (string \"%s\")\n",
						     (yyvsp[(1) - (2)].num), (yyvsp[(2) - (2)].ptr));
					    ParseError = 1;
					  }
					}
    break;

  case 247:

/* Line 1455 of yacc.c  */
#line 698 "gram.y"
    {}
    break;

  case 250:

/* Line 1455 of yacc.c  */
#line 705 "gram.y"
    { do_string_savecolor(color, (yyvsp[(1) - (1)].ptr)); }
    break;

  case 251:

/* Line 1455 of yacc.c  */
#line 706 "gram.y"
    { do_var_savecolor((yyvsp[(1) - (1)].num)); }
    break;

  case 252:

/* Line 1455 of yacc.c  */
#line 709 "gram.y"
    {}
    break;

  case 255:

/* Line 1455 of yacc.c  */
#line 716 "gram.y"
    { if (Scr->FirstTime &&
					      color == Scr->Monochrome)
					    AddToList(list, (yyvsp[(1) - (2)].ptr), (yyvsp[(2) - (2)].ptr)); }
    break;

  case 256:

/* Line 1455 of yacc.c  */
#line 721 "gram.y"
    {}
    break;

  case 259:

/* Line 1455 of yacc.c  */
#line 728 "gram.y"
    { AddToList (&Scr->WindowGeometries, (yyvsp[(1) - (2)].ptr), (yyvsp[(2) - (2)].ptr)); }
    break;

  case 260:

/* Line 1455 of yacc.c  */
#line 731 "gram.y"
    {}
    break;

  case 263:

/* Line 1455 of yacc.c  */
#line 738 "gram.y"
    { AddToList (&Scr->VirtualScreens, (yyvsp[(1) - (1)].ptr), ""); }
    break;

  case 264:

/* Line 1455 of yacc.c  */
#line 741 "gram.y"
    {
				    if (HasShape) Scr->SqueezeTitle = TRUE;
				}
    break;

  case 265:

/* Line 1455 of yacc.c  */
#line 744 "gram.y"
    { list = &Scr->SqueezeTitleL;
				  if (HasShape && Scr->SqueezeTitle == -1)
				    Scr->SqueezeTitle = TRUE;
				}
    break;

  case 267:

/* Line 1455 of yacc.c  */
#line 749 "gram.y"
    { Scr->SqueezeTitle = FALSE; }
    break;

  case 268:

/* Line 1455 of yacc.c  */
#line 750 "gram.y"
    { list = &Scr->DontSqueezeTitleL; }
    break;

  case 271:

/* Line 1455 of yacc.c  */
#line 755 "gram.y"
    {
				if (Scr->FirstTime) {
				   do_squeeze_entry (list, (yyvsp[(2) - (5)].ptr), (yyvsp[(3) - (5)].num), (yyvsp[(4) - (5)].num), (yyvsp[(5) - (5)].num));
				}
			}
    break;

  case 272:

/* Line 1455 of yacc.c  */
#line 763 "gram.y"
    {}
    break;

  case 275:

/* Line 1455 of yacc.c  */
#line 770 "gram.y"
    { if (Scr->FirstTime)
					    AddToList(list, (yyvsp[(1) - (3)].ptr), (char *)
						AllocateIconManager((yyvsp[(1) - (3)].ptr), NULLSTR,
							(yyvsp[(2) - (3)].ptr),(yyvsp[(3) - (3)].num)));
					}
    break;

  case 276:

/* Line 1455 of yacc.c  */
#line 776 "gram.y"
    { if (Scr->FirstTime)
					    AddToList(list, (yyvsp[(1) - (4)].ptr), (char *)
						AllocateIconManager((yyvsp[(1) - (4)].ptr),(yyvsp[(2) - (4)].ptr),
						(yyvsp[(3) - (4)].ptr), (yyvsp[(4) - (4)].num)));
					}
    break;

  case 277:

/* Line 1455 of yacc.c  */
#line 783 "gram.y"
    {}
    break;

  case 280:

/* Line 1455 of yacc.c  */
#line 790 "gram.y"
    {
			AddWorkSpace ((yyvsp[(1) - (1)].ptr), NULLSTR, NULLSTR, NULLSTR, NULLSTR, NULLSTR);
		}
    break;

  case 281:

/* Line 1455 of yacc.c  */
#line 793 "gram.y"
    {
			curWorkSpc = (char*)(yyvsp[(1) - (1)].ptr);
		}
    break;

  case 283:

/* Line 1455 of yacc.c  */
#line 799 "gram.y"
    {}
    break;

  case 286:

/* Line 1455 of yacc.c  */
#line 806 "gram.y"
    {
			AddWorkSpace (curWorkSpc, (yyvsp[(1) - (1)].ptr), NULLSTR, NULLSTR, NULLSTR, NULLSTR);
		}
    break;

  case 287:

/* Line 1455 of yacc.c  */
#line 809 "gram.y"
    {
			AddWorkSpace (curWorkSpc, (yyvsp[(1) - (2)].ptr), (yyvsp[(2) - (2)].ptr), NULLSTR, NULLSTR, NULLSTR);
		}
    break;

  case 288:

/* Line 1455 of yacc.c  */
#line 812 "gram.y"
    {
			AddWorkSpace (curWorkSpc, (yyvsp[(1) - (3)].ptr), (yyvsp[(2) - (3)].ptr), (yyvsp[(3) - (3)].ptr), NULLSTR, NULLSTR);
		}
    break;

  case 289:

/* Line 1455 of yacc.c  */
#line 815 "gram.y"
    {
			AddWorkSpace (curWorkSpc, (yyvsp[(1) - (4)].ptr), (yyvsp[(2) - (4)].ptr), (yyvsp[(3) - (4)].ptr), (yyvsp[(4) - (4)].ptr), NULLSTR);
		}
    break;

  case 290:

/* Line 1455 of yacc.c  */
#line 818 "gram.y"
    {
			AddWorkSpace (curWorkSpc, (yyvsp[(1) - (5)].ptr), (yyvsp[(2) - (5)].ptr), (yyvsp[(3) - (5)].ptr), (yyvsp[(4) - (5)].ptr), (yyvsp[(5) - (5)].ptr));
		}
    break;

  case 291:

/* Line 1455 of yacc.c  */
#line 823 "gram.y"
    {
		    WMapCreateCurrentBackGround ((yyvsp[(2) - (3)].ptr), NULL, NULL, NULL);
		}
    break;

  case 292:

/* Line 1455 of yacc.c  */
#line 826 "gram.y"
    {
		    WMapCreateCurrentBackGround ((yyvsp[(2) - (4)].ptr), (yyvsp[(3) - (4)].ptr), NULL, NULL);
		}
    break;

  case 293:

/* Line 1455 of yacc.c  */
#line 829 "gram.y"
    {
		    WMapCreateCurrentBackGround ((yyvsp[(2) - (5)].ptr), (yyvsp[(3) - (5)].ptr), (yyvsp[(4) - (5)].ptr), NULL);
		}
    break;

  case 294:

/* Line 1455 of yacc.c  */
#line 832 "gram.y"
    {
		    WMapCreateCurrentBackGround ((yyvsp[(2) - (6)].ptr), (yyvsp[(3) - (6)].ptr), (yyvsp[(4) - (6)].ptr), (yyvsp[(5) - (6)].ptr));
		}
    break;

  case 295:

/* Line 1455 of yacc.c  */
#line 837 "gram.y"
    {
		    WMapCreateDefaultBackGround ((yyvsp[(2) - (3)].ptr), NULL, NULL, NULL);
		}
    break;

  case 296:

/* Line 1455 of yacc.c  */
#line 840 "gram.y"
    {
		    WMapCreateDefaultBackGround ((yyvsp[(2) - (4)].ptr), (yyvsp[(3) - (4)].ptr), NULL, NULL);
		}
    break;

  case 297:

/* Line 1455 of yacc.c  */
#line 843 "gram.y"
    {
		    WMapCreateDefaultBackGround ((yyvsp[(2) - (5)].ptr), (yyvsp[(3) - (5)].ptr), (yyvsp[(4) - (5)].ptr), NULL);
		}
    break;

  case 298:

/* Line 1455 of yacc.c  */
#line 846 "gram.y"
    {
		    WMapCreateDefaultBackGround ((yyvsp[(2) - (6)].ptr), (yyvsp[(3) - (6)].ptr), (yyvsp[(4) - (6)].ptr), (yyvsp[(5) - (6)].ptr));
		}
    break;

  case 299:

/* Line 1455 of yacc.c  */
#line 851 "gram.y"
    {}
    break;

  case 302:

/* Line 1455 of yacc.c  */
#line 858 "gram.y"
    { if (Scr->FirstTime)
					    AddToList(list, (yyvsp[(1) - (1)].ptr), 0);
					}
    break;

  case 303:

/* Line 1455 of yacc.c  */
#line 863 "gram.y"
    {}
    break;

  case 306:

/* Line 1455 of yacc.c  */
#line 870 "gram.y"
    {client = (char*)(yyvsp[(1) - (1)].ptr);}
    break;

  case 308:

/* Line 1455 of yacc.c  */
#line 872 "gram.y"
    {client = (char*)(yyvsp[(2) - (2)].ptr);}
    break;

  case 310:

/* Line 1455 of yacc.c  */
#line 874 "gram.y"
    {workspace = (char*)(yyvsp[(2) - (2)].ptr);}
    break;

  case 312:

/* Line 1455 of yacc.c  */
#line 878 "gram.y"
    {}
    break;

  case 315:

/* Line 1455 of yacc.c  */
#line 885 "gram.y"
    {
				AddToClientsList ((yyvsp[(1) - (1)].ptr), client);
			  }
    break;

  case 316:

/* Line 1455 of yacc.c  */
#line 890 "gram.y"
    {}
    break;

  case 319:

/* Line 1455 of yacc.c  */
#line 897 "gram.y"
    {
				AddToClientsList (workspace, (yyvsp[(1) - (1)].ptr));
			  }
    break;

  case 320:

/* Line 1455 of yacc.c  */
#line 902 "gram.y"
    {}
    break;

  case 323:

/* Line 1455 of yacc.c  */
#line 909 "gram.y"
    { if (Scr->FirstTime) AddToList(list, (yyvsp[(1) - (2)].ptr), (yyvsp[(2) - (2)].ptr)); }
    break;

  case 324:

/* Line 1455 of yacc.c  */
#line 912 "gram.y"
    {}
    break;

  case 327:

/* Line 1455 of yacc.c  */
#line 919 "gram.y"
    { AddToMenu(root, "", Action, NULL, (yyvsp[(1) - (1)].num),
						    NULLSTR, NULLSTR);
					  Action = "";
					}
    break;

  case 328:

/* Line 1455 of yacc.c  */
#line 925 "gram.y"
    {lastmenuitem = (MenuItem*) 0;}
    break;

  case 331:

/* Line 1455 of yacc.c  */
#line 932 "gram.y"
    {
			if ((yyvsp[(2) - (2)].num) == F_SEPARATOR) {
			    if (lastmenuitem) lastmenuitem->separated = 1;
			}
			else {
			    lastmenuitem = AddToMenu(root, (yyvsp[(1) - (2)].ptr), Action, pull, (yyvsp[(2) - (2)].num), NULLSTR, NULLSTR);
			    Action = "";
			    pull = NULL;
			}
		}
    break;

  case 332:

/* Line 1455 of yacc.c  */
#line 942 "gram.y"
    {
			if ((yyvsp[(7) - (7)].num) == F_SEPARATOR) {
			    if (lastmenuitem) lastmenuitem->separated = 1;
			}
			else {
			    lastmenuitem = AddToMenu(root, (yyvsp[(1) - (7)].ptr), Action, pull, (yyvsp[(7) - (7)].num), (yyvsp[(3) - (7)].ptr), (yyvsp[(5) - (7)].ptr));
			    Action = "";
			    pull = NULL;
			}
		}
    break;

  case 333:

/* Line 1455 of yacc.c  */
#line 954 "gram.y"
    { (yyval.num) = (yyvsp[(1) - (1)].num); }
    break;

  case 334:

/* Line 1455 of yacc.c  */
#line 955 "gram.y"
    {
				(yyval.num) = (yyvsp[(1) - (2)].num);
				Action = (char*)(yyvsp[(2) - (2)].ptr);
				switch ((yyvsp[(1) - (2)].num)) {
				  case F_MENU:
				    pull = GetRoot ((yyvsp[(2) - (2)].ptr), NULLSTR,NULLSTR);
				    pull->prev = root;
				    break;
				  case F_WARPRING:
				    if (!CheckWarpRingArg (Action)) {
					twmrc_error_prefix();
					fprintf (stderr,
			"ignoring invalid f.warptoring argument \"%s\"\n",
						 Action);
					(yyval.num) = F_NOP;
				    }
				  case F_WARPTOSCREEN:
				    if (!CheckWarpScreenArg (Action)) {
					twmrc_error_prefix();
					fprintf (stderr,
			"ignoring invalid f.warptoscreen argument \"%s\"\n",
						 Action);
					(yyval.num) = F_NOP;
				    }
				    break;
				  case F_COLORMAP:
				    if (CheckColormapArg (Action)) {
					(yyval.num) = F_COLORMAP;
				    } else {
					twmrc_error_prefix();
					fprintf (stderr,
			"ignoring invalid f.colormap argument \"%s\"\n",
						 Action);
					(yyval.num) = F_NOP;
				    }
				    break;
				} /* end switch */
				   }
    break;

  case 335:

/* Line 1455 of yacc.c  */
#line 996 "gram.y"
    { (yyval.num) = (yyvsp[(1) - (1)].num); }
    break;

  case 336:

/* Line 1455 of yacc.c  */
#line 997 "gram.y"
    { (yyval.num) = (yyvsp[(2) - (2)].num); }
    break;

  case 337:

/* Line 1455 of yacc.c  */
#line 998 "gram.y"
    { (yyval.num) = -((yyvsp[(2) - (2)].num)); }
    break;

  case 338:

/* Line 1455 of yacc.c  */
#line 1001 "gram.y"
    { (yyval.num) = (yyvsp[(2) - (2)].num);
					  if ((yyvsp[(2) - (2)].num) == 0)
						yyerror("bad button 0");

					  if ((yyvsp[(2) - (2)].num) > MAX_BUTTONS)
					  {
						(yyval.num) = 0;
						yyerror("button number too large");
					  }
					}
    break;

  case 339:

/* Line 1455 of yacc.c  */
#line 1013 "gram.y"
    { ptr = (char *)malloc(strlen((char*)(yyvsp[(1) - (1)].ptr))+1);
					  strcpy(ptr, (yyvsp[(1) - (1)].ptr));
					  RemoveDQuote(ptr);
					  (yyval.ptr) = ptr;
					}
    break;

  case 340:

/* Line 1455 of yacc.c  */
#line 1020 "gram.y"
    { (yyval.num) = (yyvsp[(1) - (1)].num); }
    break;



/* Line 1455 of yacc.c  */
#line 4177 "y.tab.c"
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



/* Line 1675 of yacc.c  */
#line 1023 "gram.y"

void yyerror(char *s)
{
    twmrc_error_prefix();
    fprintf (stderr, "error in input file:  %s\n", s ? s : "");
    ParseError = 1;
}

void RemoveDQuote(char *str)
{
    register char *i, *o;
    register int n;
    register int count;

    for (i=str+1, o=str; *i && *i != '\"'; o++)
    {
	if (*i == '\\')
	{
	    switch (*++i)
	    {
	    case 'n':
		*o = '\n';
		i++;
		break;
	    case 'b':
		*o = '\b';
		i++;
		break;
	    case 'r':
		*o = '\r';
		i++;
		break;
	    case 't':
		*o = '\t';
		i++;
		break;
	    case 'f':
		*o = '\f';
		i++;
		break;
	    case '0':
		if (*++i == 'x')
		    goto hex;
		else
		    --i;
	    case '1': case '2': case '3':
	    case '4': case '5': case '6': case '7':
		n = 0;
		count = 0;
		while (*i >= '0' && *i <= '7' && count < 3)
		{
		    n = (n<<3) + (*i++ - '0');
		    count++;
		}
		*o = n;
		break;
	    hex:
	    case 'x':
		n = 0;
		count = 0;
		while (i++, count++ < 2)
		{
		    if (*i >= '0' && *i <= '9')
			n = (n<<4) + (*i - '0');
		    else if (*i >= 'a' && *i <= 'f')
			n = (n<<4) + (*i - 'a') + 10;
		    else if (*i >= 'A' && *i <= 'F')
			n = (n<<4) + (*i - 'A') + 10;
		    else
			break;
		}
		*o = n;
		break;
	    case '\n':
		i++;	/* punt */
		o--;	/* to account for o++ at end of loop */
		break;
	    case '\"':
	    case '\'':
	    case '\\':
	    default:
		*o = *i++;
		break;
	    }
	}
	else
	    *o = *i++;
    }
    *o = '\0';
}

static MenuRoot *GetRoot(char *name, char *fore, char *back)
{
    MenuRoot *tmp;

    tmp = FindMenuRoot(name);
    if (tmp == NULL)
	tmp = NewMenuRoot(name);

    if (fore)
    {
	int save;

	save = Scr->FirstTime;
	Scr->FirstTime = TRUE;
	GetColor(COLOR, &tmp->highlight.fore, fore);
	GetColor(COLOR, &tmp->highlight.back, back);
	Scr->FirstTime = save;
    }

    return tmp;
}

static void GotButton (int butt, int func)
{
    int i;
    MenuItem *item;

    for (i = 0; i < NUM_CONTEXTS; i++) {
	if ((cont & (1 << i)) == 0) continue;

	if (func == F_MENU) {
	    pull->prev = NULL;
	    AddFuncButton (butt, i, mods, func, pull, (MenuItem*) 0);
	}
	else {
	    root = GetRoot (TWM_ROOT, NULLSTR, NULLSTR);
	    item = AddToMenu (root, "x", Action, NULL, func, NULLSTR, NULLSTR);
	    AddFuncButton (butt, i, mods, func, (MenuRoot*) 0, item);
	}
    }

    Action = "";
    pull = NULL;
    cont = 0;
    mods_used |= mods;
    mods = 0;
}

static void GotKey(char *key, int func)
{
    int i;

    for (i = 0; i < NUM_CONTEXTS; i++)
    {
	if ((cont & (1 << i)) == 0)
	  continue;

	if (func == F_MENU) {
	    pull->prev = NULL;
	    if (!AddFuncKey (key, i, mods, func, pull, Name, Action)) break;
	}
	else
	if (!AddFuncKey(key, i, mods, func, (MenuRoot*) 0, Name, Action))
	  break;
    }

    Action = "";
    pull = NULL;
    cont = 0;
    mods_used |= mods;
    mods = 0;
}


static void GotTitleButton (char *bitmapname, int func, Bool rightside)
{
    if (!CreateTitleButton (bitmapname, func, Action, pull, rightside, True)) {
	twmrc_error_prefix();
	fprintf (stderr,
		 "unable to create %s titlebutton \"%s\"\n",
		 rightside ? "right" : "left", bitmapname);
    }
    Action = "";
    pull = NULL;
}

static Bool CheckWarpScreenArg (register char *s)
{
    XmuCopyISOLatin1Lowered (s, s);

    if (strcmp (s,  WARPSCREEN_NEXT) == 0 ||
	strcmp (s,  WARPSCREEN_PREV) == 0 ||
	strcmp (s,  WARPSCREEN_BACK) == 0)
      return True;

    for (; *s && isascii(*s) && isdigit(*s); s++) ; /* SUPPRESS 530 */
    return (*s ? False : True);
}


static Bool CheckWarpRingArg (register char *s)
{
    XmuCopyISOLatin1Lowered (s, s);

    if (strcmp (s,  WARPSCREEN_NEXT) == 0 ||
	strcmp (s,  WARPSCREEN_PREV) == 0)
      return True;

    return False;
}


static Bool CheckColormapArg (register char *s)
{
    XmuCopyISOLatin1Lowered (s, s);

    if (strcmp (s, COLORMAP_NEXT) == 0 ||
	strcmp (s, COLORMAP_PREV) == 0 ||
	strcmp (s, COLORMAP_DEFAULT) == 0)
      return True;

    return False;
}


void twmrc_error_prefix (void)
{
    fprintf (stderr, "%s:  line %d:  ", ProgramName, twmrc_lineno);
}

