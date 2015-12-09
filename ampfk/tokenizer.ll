
%option noyywrap
%option yylineno
%option outfile="lex.yy.c"
 /* header-file="lex.yy.h" */

%{

#include <string>
#include "automake_parser.h"
#include "parser.hh"
#include "condition.h"

using namespace std;

static string * strvec(const char * w, int len);
static void handle_if(const char *text, int len);
static void handle_else(void);
static void handle_endif(void);
static int maybe_ret(void);

%}

 // allow '=' signs in assignments.
%s ASSIGNMENT

%%

^if[ \t]+.*(\r|\n)		{ handle_if(yytext,yyleng); }
^else.*(\r|\n)			{ handle_else(); }
^endif.*(\r|\n)			{ handle_endif(); }

<INITIAL>\+=	{ if (maybe_ret()) { BEGIN(ASSIGNMENT); return PLUSEQ; }}
<INITIAL>=	{ if (maybe_ret()) { BEGIN(ASSIGNMENT); return EQ; }}
:		{ if (maybe_ret()) return COLON; }
(\r|\n)		{ if (maybe_ret()) { BEGIN(INITIAL); return NL; }}
^\t		{ if (maybe_ret()) return TAB; }
\\(\r|\n)[ \t]*	{ if (maybe_ret()) return BSNL; }
#.*(\r|\n)	{ /* skip comments */ }
[ \t]+		{ /* skip */ }
<INITIAL>[^ \t\r\n:=#]+	{
		    if (maybe_ret()) {
		        yylval.word = strvec( yytext, yyleng );
			return WORD;
		    }
		}
<ASSIGNMENT>[^ \t\r\n:#]+	{
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
    yyinput();
}

/* the return from this function needs to be free'd
   when it is no longer needed. */

static string *
strvec( const char * w, int len )
{
    string * ret = new string;

    ret->append(w, len);
    return ret;
}

static int if_stack_level = 0;
static int if_stack_conditions[10] = { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

void
tokenizer_init(FILE *in)
{
    yyin = in;
    if_stack_level = 0;
    memset(if_stack_conditions, 0, sizeof(if_stack_conditions));
    if_stack_conditions[0] = 1;
}

static void
handle_if(const char *text, int len)
{
    if (if_stack_level == 9)
    {
        printf("ERROR : can't handle if-statements more than 10 levels deep\n");
        exit(1);
    }
    if_stack_level ++;
    if_stack_conditions[if_stack_level] = conditions.check(text,len);
}

static void
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

static void
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

static int
maybe_ret(void)
{
    return if_stack_conditions[if_stack_level];
}
