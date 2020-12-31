/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
*/

%{

#define DEBUG 0

#include "wordentry.h"
#include "parse_actions.h"
#include "states-misc_states_parser.h"

#define myinput input

char * strvec( char * w, int len );

#if DEBUG
#define RET(x) { \
	if ( x > 255 ) \
		fprintf( stderr, "token: %s\n", #x ); \
        else if ( x < 33 ) \
                fprintf( stderr, "token: %d\n", x ); \
	else \
		fprintf( stderr, "token: '%c'\n", x ); \
	return x; \
 }
#else
#define RET(x) return x
#endif

#define YY_NO_UNPUT

%}

%%

"\%\{"		{ yylval.vtext = copy_verbatim( myinput ); RET( VTEXT ); }
(\r|\n)		{ count_line(); }
(\t|\ )		{ }
#.*$		{ }
"{"		{ RET( LB ); }
"}"		{ RET( RB ); }
machine		{ RET( MACHINEKW ); }
machine_cargs	{ RET( VTEXT_CARGS ); }
machine_ccode	{ RET( VTEXT_CCODE ); }
machine_dcode	{ RET( VTEXT_DCODE ); }
starthdr	{ RET( VTEXT_STARTHDR ); }
machine_data	{ RET( VTEXT_DATA     ); }
endhdr		{ RET( VTEXT_ENDHDR   ); }
startimpl	{ RET( VTEXT_STARTIMPL  ); }
endimpl		{ RET( VTEXT_ENDIMPL  ); }
inputs		{ RET( INPUTS ); }
outputs		{ RET( OUTPUTS ); }
states		{ RET( STATES ); }
state		{ RET( STATE ); }
pre		{ RET( PRE ); }
post		{ RET( POST ); }
switch		{ RET( SWITCH ); }
default		{ RET( DEFAULT ); }
call		{ RET( CALL ); }
case		{ RET( CASE ); }
empty		{ RET( EMPTY ); }
output		{ RET( OUTPUT ); }
input		{ RET( INPUT ); }
timeout		{ RET( TIMEOUT ); }
inline		{ RET( INLINE ); }
exit		{ RET( EXIT ); }
next		{ RET( NEXT ); }
[0-9]+		{
			yylval.value = atoi( yytext );
#if DEBUG
			fprintf( stderr, "token: NUMBER (%d)\n",
				 yylval.value );
#endif
			return NUMBER;
		}
[a-zA-Z][a-zA-Z0-9_]* {
			yylval.word = strvec( yytext, yyleng );
#if DEBUG
			fprintf( stderr, "token: IDENT (%s)\n",
				 yylval.word );
#endif
			return IDENT;
		}

%%

/* the return from this function needs to be free'd
   when it is no longer needed. */

char *
strvec( char * w, int len )
{
    char * ret;

    ret = (char*) malloc( len + 1 );
    if ( !ret )
    {
        fprintf( stderr, "out of memory\n" );
        exit( 1 );
    }
    memcpy( ret, w, len );
    ret[ len ] = 0;

    return ret;
}
