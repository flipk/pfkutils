/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

/*

NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE 
NOTE 
NOTE if you edit this file, you must run
NOTE    ./flexbison.sh
NOTE to regenerate the cc and h files.
NOTE 
NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE 

REFERENCE: https://www.json.org/json-en.html

*/

%option noyywrap
%option yylineno
%option reentrant
%option prefix="json_tokenizer_"
%option outfile="json_tokenizer.cc"
%option header-file="json_tokenizer.h"

%{

#define __STDC_FORMAT_MACROS 1
#include <inttypes.h>
#include <string>
#include "simple_json.h"
#include "tokenize_and_parse.h"
#include "json_parser.hh"

using namespace std;

string  collect_string;

/* TODO : \bfnrt \uABCD  and exponents in numbers? */
%}

ws              [\t \r\n]+

%s IN_STRING

%%

<IN_STRING>\\.   {
    // we un-escape double quotes here but
    // we leave all the other escapes alone.
    // (FYI this is un-done in operator<<(StringProperty)).
    if (yytext[1] != '"')
        collect_string += yytext[0];
    collect_string += yytext[1];
}

<IN_STRING>\"    {
    BEGIN(INITIAL);
    yylval->str_value = new string(collect_string);
    return TOK_STRING;
}

<IN_STRING>.     { collect_string += yytext[0]; }


<INITIAL>\"      { BEGIN(IN_STRING); collect_string =""; }

<INITIAL>[+-]?[0-9]*\.[0-9]* {
    std::string s(yytext, yyleng);
    char * endptr = NULL;
    double val = strtod(s.c_str(), &endptr);
    yylval->float_value = val;
    return TOK_FLOAT;
}

<INITIAL>[+-]?[0-9]+         {
    std::string s(yytext, yyleng);
    char * endptr = NULL;
    long val = strtol(s.c_str(), &endptr, /*base*/0);
    yylval->int_value = (int) val;
    return TOK_INT;
}

<INITIAL>:                { return TOK_COLON; }
<INITIAL>\[               { return TOK_OPENSQUARE; }
<INITIAL>\]               { return TOK_CLOSESQUARE; }
<INITIAL>\{               { return TOK_OPENBRACE; }
<INITIAL>\}               { return TOK_CLOSEBRACE; }
<INITIAL>,                { return TOK_COMMA; }
<INITIAL>true             { return TOK_TRUE; }
<INITIAL>false            { return TOK_FALSE; }
<INITIAL>null             { return TOK_NULL; }
<INITIAL>{ws}             {  }
<INITIAL>.                {  }

%%
