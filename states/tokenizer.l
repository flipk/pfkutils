
%{

#define DEBUG 0

#include "wordentry.h"
#include "parse_actions.h"
#include "tokens.h"

#ifdef FREEBSD
#define myinput input
#endif

#ifdef SOLARIS
#define myinput yyinput
#endif

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
message_type	{ RET( MESSAGE_TYPE ); }
starthdr	{ RET( VTEXT_STARTHDR ); }
machine_data	{ RET( VTEXT_DATA     ); }
endhdr		{ RET( VTEXT_ENDHDR   ); }
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