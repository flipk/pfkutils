
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton interface for Bison's Yacc-like parsers in C
   
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

/* Line 1676 of yacc.c  */
#line 121 "gram.y"

    int num;
    char *ptr;



/* Line 1676 of yacc.c  */
#line 275 "y.tab.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;


