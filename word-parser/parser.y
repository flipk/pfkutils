
%{

#include <stdio.h>
#include <stdlib.h>
#include "tokenizer.h"
#include "tuple.h"

extern int yylex( void );
extern void yyerror( char * e );

struct tuple * parser_output;

%}

%union
{
    int value;
    char * word;
    struct tuple * tuple;
}

%token LP RP COLON

%token <word> WORD HEXVALUE STRING

%type <tuple> TUPLE
%type <tuple> TUPLE_LIST

%start LINE

%%

LINE
        : LP TUPLE_LIST RP
        {
            parser_output = $2;
        }
        ;

TUPLE_LIST
        : TUPLE
        {
            $$ = $1;
        }
        | TUPLE TUPLE_LIST 
        {
            $1->next = $2;
            $$ = $1;
        }
        ;

TUPLE
        : WORD
        {
            struct tuple * t = (struct tuple *) malloc( sizeof(struct tuple));
            t->next = NULL;
            t->word = $1;
            t->type = TUPLE_TYPE_NONE;
            $$ = t;
        }
        | WORD COLON WORD
        {
            struct tuple * t = (struct tuple *) malloc( sizeof(struct tuple));
            t->next = NULL;
            t->word = $1;
            t->type = TUPLE_TYPE_WORD;
            t->u.word = $3;
            $$ = t;
        }
        | WORD COLON HEXVALUE
        {
            struct tuple * t = (struct tuple *) malloc( sizeof(struct tuple));
            t->next = NULL;
            t->word = $1;
            t->type = TUPLE_TYPE_HEX;
            t->u.word = $3;
            $$ = t;
        }
        | WORD COLON STRING
        {
            struct tuple * t = (struct tuple *) malloc( sizeof(struct tuple));
            t->next = NULL;
            t->word = $1;
            t->type = TUPLE_TYPE_STRING;
            t->u.word = $3;
            $$ = t;
        }
        | WORD COLON LP TUPLE_LIST RP
        {
            struct tuple * t = (struct tuple *) malloc( sizeof(struct tuple));
            t->next = NULL;
            t->word = $1;
            t->type = TUPLE_TYPE_TUPLE;
            t->u.tuples = $4;
            $$ = t;
        }
        | LP TUPLE_LIST RP
        {
            struct tuple * t = (struct tuple *) malloc( sizeof(struct tuple));
            t->next = NULL;
            t->word = "empty";
            t->type = TUPLE_TYPE_TUPLE;
            t->u.tuples = $2;
            $$ = t;
        }
        ;

%%

void
yyerror( char * e )
{
    fprintf(stderr, "error: %d: %s\n", parser_line_number, e);
    exit( 1 );
}

int
yywrap( void )
{
    return 1;
}

void
dump_tokens(void)
{
    while(1)
    {
        int c = yylex();
        if (c == 0)
            break;
        printf("%d : %s\n", c, yytname[c-255]);
    }
}
