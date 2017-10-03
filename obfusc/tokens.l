/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
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
