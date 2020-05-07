/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

/*

NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE 
NOTE 
NOTE if you edit this file, you must run
NOTE    ./flexbison.sh
NOTE to regenerate the cc and hh files.
NOTE 
NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE 


REFERENCE: https://www.json.org/json-en.html

*/

%parse-param {yyscan_t yyscanner}
%define api.pure
%name-prefix "json_parser_"

%{

#define YYDEBUG 1
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS 1
#endif

#include <string>
#include <iostream>
#include <pthread.h>
#include <inttypes.h>
#ifndef DEPENDING
#include "json_tokenizer.h"
#endif
#include "simple_json.h"
#define __JSON_PARSER_INTERNAL__ 1
#include "json_tokenize_and_parse.h"
#include PARSER_YY_HDR

static void yyerror( yyscan_t yyscanner, const std::string e );

extern YY_DECL;

using namespace std;
using namespace SimpleJson;

#undef  yylex
#define yylex(yylval) json_tokenizer_lex(yylval, yyscanner)

static Property *returnProperty;

%}

%union
{
    std::string  * str_value;
    int64_t int64_value;
    double float_value;
    // must include namespace here because it gets autogenerated
    // into a header file outside the 'using' above.
    SimpleJson::Property *property;
    SimpleJson::ObjectProperty *objectProperty;
    SimpleJson::ArrayProperty *arrayProperty;
}

%token TOK_OPENSQUARE TOK_CLOSESQUARE TOK_OPENBRACE TOK_CLOSEBRACE
%token TOK_COMMA TOK_COLON TOK_STRING TOK_INT TOK_FLOAT
%token TOK_TRUE TOK_FALSE TOK_NULL

%type <property>   PROPERTY VALUE 
%type <objectProperty>  OBJECT PROPERTYLIST
%type <arrayProperty> ARRAY VALUELIST
%type <str_value> TOK_STRING
%type <int64_value> TOK_INT
%type <float_value> TOK_FLOAT

%start TOPLEVEL_OBJECT

%%

TOPLEVEL_OBJECT
	: OBJECT
	{
            returnProperty = $1;
	}
	| ARRAY
	{
            returnProperty = $1;
	}
	;

OBJECT
	: TOK_OPENBRACE PROPERTYLIST TOK_CLOSEBRACE
	{
            $$ = $2;
        }
	;

// FLEX/BISON RULE : ALWAYS USE LEFT-RECURSION, not RIGHT
PROPERTYLIST
	: PROPERTY
        {
            $$ = new ObjectProperty;
            $$->push_back($1);
        }
	| PROPERTYLIST TOK_COMMA PROPERTY
        {
            $$ = $1;
            $$->push_back($3);
        }
        ;

PROPERTY
	: TOK_STRING TOK_COLON VALUE
        {
            $$ = $3;
            $$->name = *$1;
            delete $1;
        }
        ;

VALUE
	: TOK_STRING
        {
            $$ = new StringProperty(*$1);
            delete $1;
        }
        | TOK_INT
        {
            $$ = new IntProperty($1);
        }
        | TOK_FLOAT
        {
            $$ = new FloatProperty($1);
        }
        | OBJECT
        {
            $$ = $1;
        }
        | ARRAY
        {
            $$ = $1;
        }
        | TOK_TRUE
        {
            $$ = new TrinaryProperty(TrinaryProperty::TP_TRUE);
        }
        | TOK_FALSE
        {
            $$ = new TrinaryProperty(TrinaryProperty::TP_FALSE);
        }
        | TOK_NULL
        {
            $$ = new TrinaryProperty(TrinaryProperty::TP_NULL);
        }
        ;

ARRAY
	: TOK_OPENSQUARE VALUELIST TOK_CLOSESQUARE
        {
            $$ = $2;
        }
        ;

VALUELIST
	: VALUE
        {
            $$ = new ArrayProperty;
            $$->push_back($1);
        }
        | VALUELIST TOK_COMMA VALUE
        {
            $$ = $1;
            $$->push_back($3);
        }
        ;

%%

static void
yyerror( yyscan_t yyscanner, const string e )
{
    fprintf(stderr, "error: %d: %s\n",
            json_tokenizer_get_lineno(yyscanner), e.c_str());
    exit( 1 );
}

// the flex scanner might be reentrant,
// but the bison parser definitely is not.
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// this has to be in this file to get access to yytname[]
void
json_parser_debug_tokenize(FILE *f)
{
    yyscan_t scanner;
    json_tokenizer_lex_init ( &scanner );
    json_tokenizer_restart(f, scanner);

    pthread_mutex_lock(&mutex);

    int c;
    do {
        YYSTYPE  yylval;
        c = json_tokenizer_lex(&yylval, scanner);
        if (c < 256)
            printf("%d ", c);
        else
            printf("%c[31;1m %s %c[m", 27,
                   yytname[c-255],
                   27);
        switch (c)
        {
        case TOK_STRING:
            printf("(\"%s\") ", yylval.str_value->c_str());
            delete yylval.str_value;
            break;

        case TOK_INT:
            printf("(%" PRId64 ") ", yylval.int64_value);
            break;

        case TOK_FLOAT:
            printf("(%lf) ", yylval.float_value);
            break;

        default:;
        }
    } while (c > 0);
    printf("\n");

    json_tokenizer_lex_destroy ( scanner );
    pthread_mutex_unlock(&mutex);
}

Property *
json_parser(FILE *f)
{
    yyscan_t scanner;
    pthread_mutex_lock(&mutex);
//    yydebug = 1;
    returnProperty = NULL;
    json_tokenizer_lex_init ( &scanner );
    json_tokenizer_restart(f, scanner);
    json_parser_parse(scanner);
    json_tokenizer_lex_destroy ( scanner );
    pthread_mutex_unlock(&mutex);
    return returnProperty;
}
