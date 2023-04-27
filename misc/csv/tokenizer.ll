/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

%option noyywrap
%option yylineno

%{

#define __STDC_FORMAT_MACROS 1
#include <inttypes.h>
#include <string>
#include "../parser.h"
#include "../tokenizer.h"
#include PARSER_YY_HDR
using namespace std;

static string * strvec(const char * w, int len);

%}

%s QUOTED

%%

<QUOTED>[^"]*  { /*"*/ yylval.word = strvec(yytext, yyleng); return WORD; }
<QUOTED>\"\" { return QUOTE; }
<QUOTED>\" { BEGIN(INITIAL); }

<INITIAL>\"   { BEGIN(QUOTED); }
<INITIAL>,    { return COMMA; }

<INITIAL>\r\n { return NEWLINE; }
<INITIAL>\n { return NEWLINE; }
<INITIAL>\r { return NEWLINE; }

<INITIAL>[^,\r\n"]*  { /*"*/ yylval.word = strvec(yytext, yyleng); return WORD; }

%%

void
tokenizer_init(FILE *in)
{
    yyin = in;
}

// quiet a warning about yyunput defined but not used.
void ___tokenizer__unused__crap(void)
{
  char c = 0;
  yyunput(0, &c);
}

static string *
strvec( const char * w, int len )
{
    string * ret = new string;
    ret->append(w, len);
    return ret;
}
