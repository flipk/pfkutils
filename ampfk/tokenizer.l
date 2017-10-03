
%option noyywrap
%option yylineno
%option outfile="lex.yy.c"
 /* header-file="lex.yy.h" */

%{

#include <stdio.h>
#include "y.tab.h"
#include "tokenizer.h"
#include "parser.h"
#include "condition.h"

#define USE_STDIN

static char * strvec( char * w, int len );
#ifndef USE_STDIN
static int myinput( char *buf, int len );
#endif

#ifndef USE_STDIN
#define YY_NO_INPUT 1
#define YY_INPUT(buf,result,max_size) result = myinput(buf, max_size)
#endif

void handle_if(const char *text, int len);
void handle_else(void);
void handle_endif(void);
int maybe_ret(void);

%}

%%

^if[ \t]+.*(\r|\n)		{ handle_if(yytext,yyleng); }
^else.*(\r|\n)			{ handle_else(); }
^endif.*(\r|\n)			{ handle_endif(); }

\+=		{ if (maybe_ret()) return PLUSEQ; }
=		{ if (maybe_ret()) return EQ; }
:		{ if (maybe_ret()) return COLON; }
(\r|\n)		{ if (maybe_ret()) return NL; }
^\t		{ if (maybe_ret()) return TAB; }
\\(\r|\n)[ \t]*	{ if (maybe_ret()) return BSNL; }
#.*(\r|\n)	{ /* skip comments */ }
[ \t]+		{ /* skip */ }
[^ \t\r\n:=#]+	{
		    if (maybe_ret()) {
		        yylval.word = strvec( yytext, yyleng );
			return WORD;
		    }
		}

%%


void
tokenizer_dummy_function(void)
{
    // with this, you can build with -Wall silently
    yyunput(0,0);
}

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

#ifndef USE_STDIN
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
#endif

int if_stack_level = 0;
int if_stack_conditions[10] = { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

void
tokenizer_init(FILE *in)
{
    yyin = in;
    if_stack_level = 0;
    memset(if_stack_conditions, 0, sizeof(if_stack_conditions));
    if_stack_conditions[0] = 1;
}

void
handle_if(const char *text, int len)
{
    if (if_stack_level == 9)
    {
        printf("ERROR : can't handle if-statements more than 10 levels deep\n");
        exit(1);
    }
    if_stack_level ++;
    if_stack_conditions[if_stack_level] = check_condition_name(text,len);
}

void
handle_else(void)
{
    if (if_stack_level == 0)
    {
        printf("ERROR 'else' found without matching 'if'\n");
        exit(1);
    }
    // simply invert the current condition
    if_stack_conditions[if_stack_level] ^= 1;
}

void
handle_endif(void)
{
    if (if_stack_level == 0)
    {
        printf("ERROR 'endif' found without matching 'if'\n");
        exit(1);
    }
    // pop one stack level
    if_stack_level--;
}

int
maybe_ret(void)
{
    return if_stack_conditions[if_stack_level];
}
