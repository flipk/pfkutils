
%{

#include <stdio.h>
#include <string.h>
#include "tuple.h"
#include "y.tab.h"
#include "tokenizer.h"

static char * strvec( char * w, int len );
static int myinput( char *buf, int len );

int parser_line_number;

#define YY_NO_INPUT 1
#define YY_INPUT(buf,result,max_size) result = myinput(buf, max_size)

%}

%%

[a-zA-Z0-9_\.\!\?\*]+ {
                 yylval.word = strvec( yytext, yyleng );
                 return WORD;
              }

:             { return COLON; }

\(            { return LP; }

\)            { return RP; }

(\r|\n)       { parser_line_number++; }

[ \t]+        { /* skip */ }

%%

/* the return from this function needs to be free'd
   when it is no longer needed. */

static char *
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

static char * input_buf;
static int buf_pos;
static int buf_remaining;

static int
myinput( char *buf, int maxlen )
{
    int toread = maxlen;
    if (toread > buf_remaining)
        toread = buf_remaining;
    if (toread > 0)
    {
        memcpy(buf, input_buf+buf_pos, toread);
        buf_pos += toread;
        buf_remaining -= toread;
    }
    return toread;
}

void
tokenizer_set_input_buf(char * buf, int len)
{
    input_buf = buf;
    buf_pos = 0;
    buf_remaining = len;
}

void
tokenizer_dummy_function(void)
{
    // with this, you can build with -Wall silently
    yyunput(0,0);
}
