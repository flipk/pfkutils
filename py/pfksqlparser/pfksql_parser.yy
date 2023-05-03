/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

%parse-param {yyscan_t yyscanner}
%define api.pure
%define api.prefix {pfk_sql_parser_}

%{

#include <string>
#include <pthread.h>

#define YYDEBUG 1
#ifndef DEPENDING
#include TOKENIZER_LL_HDR
#endif

#define __PFKSQL_PARSE_INTERNAL__ 1
#include "pfksql_tokenize_and_parse.h"
#include PARSER_YY_HDR

using namespace std;

static void yyerror( yyscan_t yyscanner, const std::string e );

extern YY_DECL;
#undef  yylex
#define yylex(yylval) pfksql_tokenizer_lex(yylval, yyscanner)

static SelectCommand * sc = NULL;

%}

%union
{
    std::string * word_value;
    int64_t       int_value;
    bool          bool_value;
    FieldList   * field_list;
    Conjunction   conj;
    Operator      op;
    Expr        * expr;
}

%token KW_FROM KW_SELECT KW_WHERE KW_EXISTS KW_NOT_EXIST
%token TOK_WORD TOK_VALUE TOK_OPERATOR TOK_COMMA TOK_LP TOK_RP
%token TOK_NUMBER TOK_STRING TOK_CONJUNCTION TOK_BOOL TOK_STAR

%type <word_value> TOK_WORD TOK_VALUE
%type <op> TOK_OPERATOR
%type <int_value> TOK_NUMBER
%type <bool_value> TOK_BOOL
%type <word_value> TOK_STRING
%type <conj> TOK_CONJUNCTION

%type <field_list> fieldlist
%type <expr> compare
%type <expr> expr

%start select_command

%%

select_command
	: KW_FROM TOK_WORD KW_SELECT fieldlist KW_WHERE expr
        {
            sc->table = $2;
            sc->field_list = $4;
            sc->expr = $6;
        }
	;

fieldlist
	: TOK_WORD
        {
            FieldList * fl = new FieldList;
            fl->field_list.push_back($1);
            $$ = fl;
        }
	| fieldlist TOK_COMMA TOK_WORD
        {
            $1->field_list.push_back($3);
            $$ = $1;
        }
	| TOK_STAR
        {
            FieldList * fl = new FieldList;
            fl->field_list.push_back(new std::string("*"));
            $$ = fl;
        }
	;

expr
	: expr TOK_CONJUNCTION expr
        {
            ExprConj * ec = new ExprConj;
            ec->left = $1;
            ec->conj = $2;
            ec->right = $3;
            $$ = ec;
        }
	| TOK_LP expr TOK_RP
        {
            $$ = $2;
        }
	| compare
        {
            $$ = $1;
        }
	;

compare
	: TOK_WORD TOK_OPERATOR TOK_NUMBER
        {
            ExprCompareNum * ecn = new ExprCompareNum;
            ecn->field = $1;
            ecn->op = $2;
            ecn->val = $3;
            $$ = ecn;
        }
	| TOK_WORD TOK_OPERATOR TOK_STRING
        {
            ExprCompareStr * ecs = new ExprCompareStr;
            ecs->field = $1;
            ecs->op = $2;
            ecs->str = $3;
            $$ = ecs;
        }
	| TOK_WORD TOK_OPERATOR TOK_BOOL
        {
            ExprCompareBool * ecb = new ExprCompareBool;
            ecb->field = $1;
            ecb->op = $2;
            ecb->val = $3;
            $$ = ecb;
        }
	| TOK_WORD KW_EXISTS
        {
            ExprExists * ee = new ExprExists;
            ee->field = $1;
            ee->exist = true;
            $$ = ee;
        }
	| TOK_WORD KW_NOT_EXIST
        {
            ExprExists * ee = new ExprExists;
            ee->field = $1;
            ee->exist = false;
            $$ = ee;
        }
	;

%%

static void
yyerror( yyscan_t yyscanner, const string e )
{
    fprintf(stderr, "error: %d: %s\n",
            pfksql_tokenizer_get_lineno(yyscanner), e.c_str());
    if (sc)
        sc->error = true;
}

// the flex scanner might be reentrant,
// but the bison parser definitely is not.
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// this has to be in this file to get access to yytname[]
void
pfksql_parser_debug_tokenize(const char * cmd)
{
    FILE *f = fmemopen((void*)cmd, strlen(cmd), "r");
    if (f == NULL)
        return;

    yyscan_t scanner;
    pfksql_tokenizer_lex_init ( &scanner );
    pfksql_tokenizer_restart(f, scanner);

    pthread_mutex_lock(&mutex);

    int c;
    do {
        PFK_SQL_PARSER_STYPE  yylval;
        c = pfksql_tokenizer_lex(&yylval, scanner);
        if (c == 0)
            break;
        if (c < 256)
            printf("%d ", c);
        else
            printf("%c[31;1m %s %c[m", 27,
                   yytname[c-255],
                   27);
        switch (c)
        {
        case TOK_STRING:
        case TOK_WORD:
            printf("(\"%s\") ", yylval.word_value->c_str());
            delete yylval.word_value;
            break;

        case TOK_NUMBER:
            printf("(%d) ", yylval.int_value);
            break;

        case TOK_OPERATOR:
            printf("(%s) ", op_to_str(yylval.op));
            break;

        case TOK_BOOL:
            printf("(%s) ", yylval.bool_value ? "true" : "false");
            break;

        case TOK_CONJUNCTION:
            printf("(%s) ", conj_to_str(yylval.conj));
            break;

        default:;
        }
        printf("\n");
    } while (c > 0);
    printf("\n");

    pfksql_tokenizer_lex_destroy ( scanner );
    pthread_mutex_unlock(&mutex);

    fclose(f);
}

SelectCommand *
pfksql_parser(const char * cmd)
{
    FILE *f = fmemopen((void*)cmd, strlen(cmd), "r");
    if (f == NULL)
        return NULL;

    SelectCommand * ret_sc = new SelectCommand;

    yyscan_t scanner;
    pfksql_tokenizer_lex_init ( &scanner );
    pfksql_tokenizer_restart(f, scanner);

    pthread_mutex_lock(&mutex);
    sc = ret_sc;
    pfk_sql_parser_parse(scanner);
    pfksql_tokenizer_lex_destroy ( scanner );
    pthread_mutex_unlock(&mutex);
    fclose(f);

    sc = NULL;
    if (ret_sc->error)
    {
        delete ret_sc;
        ret_sc = NULL;
    }
    return ret_sc;
}
