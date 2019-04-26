/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

%option noyywrap
%option yylineno
%option outfile="lex.yy.cc"
%option header-file="obj/test-tokenizer.h"
%option reentrant
%option prefix="xml_tokenizer_"

%{

#define __STDC_FORMAT_MACROS 1
#include <inttypes.h>
#include <string>
#include "tokenizer.h"

// #include "../parser.h"
// #include PARSER_YY_HDR

using namespace std;

static string * strvec(const char * w, int len);

%}

ws      [\t \r\n]+
ident   [a-zA-Z][a-zA-Z0-9_-]*

%s IN_TAG
%s IN_TAG_QUOTE

%%

<IN_TAG_QUOTE>\"      { BEGIN(IN_TAG); return TOK_ATT_STRING; }
<IN_TAG_QUOTE>[^\"]*  { }

<IN_TAG>{ident}=      { return TOK_IDENTEQ; }
<IN_TAG>{ident}       { return TOK_IDENT; }
<IN_TAG>\"            { BEGIN(IN_TAG_QUOTE); }
<IN_TAG>\/\>          { BEGIN(INITIAL); return TOK_CLOSE_EMPTYTAG; }
<IN_TAG>\>            { BEGIN(INITIAL); return TOK_CLOSE_TAG; }
<IN_TAG>{ws}          { }

<INITIAL>\<\/{ident}\>  { return TOK_ENDTAG; }
<INITIAL>\<             { BEGIN(IN_TAG); return TOK_OPEN_TAG; }

. { return yytext[0]; }

%%

static string *
strvec( const char * w, int len )
{
    string * ret = new string;
    ret->append(w, len);
    return ret;
}
