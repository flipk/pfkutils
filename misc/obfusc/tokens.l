
/*
    This file is part of the "pfkutils" tools written by Phil Knaack
    (pfk@pfk.org).
    Copyright (C) 2008  Phillip F Knaack

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

%{

#include "tokens.h"
#include "myputs.h"

extern int col;
extern int last_was_newline;
extern int define_line;
extern int define_symbol;

#define myinput input

void consume_multiline_comment( int (*func)(void) );

%}

%%

\/\/.*$			{ return TOKEN_COMMENT; }
\/\*.*\*\/		{ return TOKEN_COMMENT; }
\/\*			{ consume_multiline_comment(myinput); }
\".*\"			{ return TOKEN_LITERAL; }
\'.*\'			{ return TOKEN_LITERAL; }
^#define		{ consume_define(myinput); define_symbol=1; }
^#			{ consume_preproc(myinput); }
[a-zA-Z0-9_]+		{ return TOKEN_IDENTIFIER; }
\\\n			{ /*swallow*/ }
\n			{ if (define_line) {
			      putchar('\n');
			      define_line=0;
			      col=0;last_was_newline=1; }
			  return TOKEN_WHITESPACE; }
(\ |\	)+		{ return TOKEN_WHITESPACE; }
.			{ return TOKEN_CHAR; }

%%

void
consume_multiline_comment( int (*func)(void) )
{
	int state = 0;

	while ( 1 )
	{
		int c = func();

		switch(state)
		{
		case 0:
			if ( c == '*' )
				state = 1;
			else if ( c == 0 || c == EOF )
				return;
			break;
		case 1:
			if ( c == '/' )
				return;
			if ( c != '*' )
				state = 0;
			break;
		}
	}
}
