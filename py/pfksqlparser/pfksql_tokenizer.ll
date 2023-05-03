/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

%option noyywrap
%option yylineno
%option reentrant
%option prefix="pfksql_tokenizer_"

%{

#define __STDC_FORMAT_MACROS 1
#include <inttypes.h>
#include <string>

#define __PFKSQL_PARSE_INTERNAL__ 1
#include "pfksql_tokenize_and_parse.h"

#include PARSER_YY_HDR

using namespace std;

static string collect_string;


%}

ws   [\t \r\n]+

%%

\"[^\"]*\"           {
    yylval->word_value = new std::string;
    yylval->word_value->assign(yytext+1, yyleng-2);
    return TOK_STRING;
}

,                { return TOK_COMMA; }
\(               { return TOK_LP; }
\)               { return TOK_RP; }
from             { return KW_FROM; }
select           { return KW_SELECT; }
where            { return KW_WHERE; }
and              {
    yylval->conj = CONJ_AND;
    return TOK_CONJUNCTION;
}
or              {
    yylval->conj = CONJ_OR;
    return TOK_CONJUNCTION;
}
true            {
    yylval->bool_value = true;
    return TOK_BOOL;
}
false           {
    yylval->bool_value = false;
    return TOK_BOOL;
}

[0-9]+           {
    std::string s(yytext, yyleng);
    char * endptr = NULL;
    long val = strtol(s.c_str(), &endptr, /*base*/0);
    yylval->int_value = (int) val;
    return TOK_NUMBER;
}
0x[0-9a-fA-F]+   {
    std::string s(yytext, yyleng);
    char * endptr = NULL;
    long val = strtol(s.c_str(), &endptr, /*base*/0);
    yylval->int_value = (int) val;
    return TOK_NUMBER;
}

like  {
    yylval->op = OP_LIKE;
    return TOK_OPERATOR;
}

!exists {
    return KW_NOT_EXIST;
}
exists {
    return KW_EXISTS;
}

   /* this one must be after all the others that
      have digits and letters because this accepts
      too many things; it must be the raingutter. */
[a-zA-Z0-9_]+ {
    yylval->word_value = new std::string(yytext, yyleng);
    return TOK_WORD;
}

\* {
    return TOK_STAR;
}

\<  {
    yylval->op = OP_LT;
    return TOK_OPERATOR;
}

\>  {
    yylval->op = OP_GT;
    return TOK_OPERATOR;
}

\<=  {
    yylval->op = OP_LTEQ;
    return TOK_OPERATOR;
}

\>=  {
    yylval->op = OP_GTEQ;
    return TOK_OPERATOR;
}

=  {
    yylval->op = OP_EQ;
    return TOK_OPERATOR;
}

\!=  {
    yylval->op = OP_NEQ;
    return TOK_OPERATOR;
}

\~  {
    yylval->op = OP_REGEX;
    return TOK_OPERATOR;
}

{ws}             { }

%%
