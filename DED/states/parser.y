/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
*/

/*
 * note that this doesn't work :
 * for some reason, it defines an enum (prefix_error) and
 * a function with the same name! that's a bug in bison IMHO.
 *  %define api.prefix {SOMEPREFIX_}
 */

%{

#include <stdio.h>
#include <stdlib.h>
#include "wordentry.h"
#include "parse_actions.h"
#include "machine.h"

extern int yylex( void );
extern void yyerror( char * e );

%}

%union
{
	int value;
	char * word;
	WENT * we;
	VERBATIM * vtext;
}

%token LB RB MACHINEKW INPUTS OUTPUTS SWITCH DEFAULT
%token STATES STATE PRE POST CALL OUTPUT INLINE
%token INPUT TIMEOUT EXIT NEXT CASE EMPTY

%token VTEXT_CARGS
%token VTEXT_CCODE
%token VTEXT_DCODE
%token VTEXT_STARTHDR
%token VTEXT_DATA
%token VTEXT_ENDHDR
%token VTEXT_STARTIMPL
%token VTEXT_ENDIMPL

%token <value> NUMBER
%token <word>  IDENT   /* NOTE, this returns memory that must be free'd! */
%token <vtext> VTEXT

%type  <we>    ident
%type  <we>    maybe_pre
%type  <we>    maybe_post
%type  <we>    inputdefs
%type  <we>    inputdef
%type  <we>    actiondefs
%type  <we>    actiondef
%type  <we>    casedefs
%type  <we>    casedef

%start statefile

%%

statefile
	: stmts
	;

stmts
	: stmt
	| stmts stmt
	;

stmt
	: MACHINEKW ident
	  { machine_addname( $2 ); }
	| VTEXT_DATA     VTEXT
	  { machine.datav = $2; }
	| VTEXT_CARGS VTEXT
	  { machine.constructor_args = $2; }
	| VTEXT_CCODE VTEXT
	  { machine.constructor_code = $2; }
	| VTEXT_DCODE VTEXT
	  { machine.destructor_code = $2; }
	| VTEXT_STARTHDR VTEXT
	  { machine.startv = $2; }
	| VTEXT_STARTIMPL VTEXT
	  { machine.startimplv = $2; }
	| INPUTS LB  inputlist	RB
	  { inputlist_add_timeout(); }
	| OUTPUTS LB  outputlist  RB
	| STATES LB statelist RB
	| statedef
	| VTEXT_ENDHDR   VTEXT
	  { machine.endhdrv = $2; }
	| VTEXT_ENDIMPL  VTEXT
	  { machine.endimplv = $2; }
	;

inputlist
	: inputitem
	| inputlist inputitem
	;

inputitem
	: ident
	  {
	    $1->type = INPUT_NAME;
	    list_add( &machine.inputs, $1 );
	  }
	;

outputlist
	: outputitem
	| outputlist outputitem
	;

outputitem
	: ident
	  {
	    $1->type = OUTPUT_NAME;
	    list_add( &machine.outputs, $1 );
	  }
	;

statelist
	: stateitem
	| statelist stateitem
	;

stateitem
	: ident
	  {
	    $1->type = STATE_NAME;
	    list_add( &machine.states, $1 );
	  }
	;

statedef
	: STATE IDENT LB maybe_pre maybe_post inputdefs RB
	  {
	    WENT * pre = new_wordentry( "prepost" );
	    pre->type = PREPOST_ACTION;
	    pre->ex[0] = $4;
	    pre->ex[1] = $5;
	    add_state( $2, pre, $6 );
	    free( $2 );
	  }
	| STATE IDENT LB maybe_pre maybe_post RB
	  {
	    WENT * pre = new_wordentry( "prepost" );
	    pre->type = PREPOST_ACTION;
	    pre->ex[0] = $4;
	    pre->ex[1] = $5;
	    add_state( $2, pre, NULL );
	    free( $2 );
	  }
	;

maybe_pre
	: /* empty */
	  { $$ = NULL; }
	| PRE LB actiondefs RB
	  { $$ = $3; }
	;

maybe_post
	: /* empty */
	  { $$ = NULL; }
	| POST LB actiondefs RB
	  { $$ = $3; }
	;

inputdefs
	: inputdef
	  { $$ = $1; }
	| inputdef inputdefs
	  { $1->next = $2; $$ = $1; }
	;

inputdef
	: INPUT ident LB actiondefs RB
	  {
	    $$ = $2;
	    $2->type = STATEINPUT_DEF;
	    $2->ex[0] = lookup_inputname( $2->word );
	    $2->ex[1] = $4;
	  }
	;

actiondefs
	: actiondef
	  { $$ = $1; }
	| actiondef actiondefs
	  {
	    if ( $1 == NULL )
	      $$ = $2;
	    else {
	      $1->next = $2;
	      $$ = $1;
	    }
	  }
	;

actiondef
	: OUTPUT ident
	  {
	    $2->type = OUTPUT_ACTION;
	    $2->ex[0] = lookup_outputname( $2->word );
	    $$ = $2;
	  }
	| CALL ident
	  {
	    $2->type = CALL_ACTION;
	    $2->ex[0] = NULL;
	    add_call( $2 );
	    $$ = $2;
	  }
	| SWITCH ident LB casedefs RB
	  {
	    $2->type = SWITCH_ACTION;
	    $2->ex[0] = $4;
	    $$ = $2;
	  }
	| CALL ident LB casedefs RB
	  {
	    $2->type = CALL_ACTION;
	    $2->ex[0] = $4;
	    add_call( $2 );
	    $$ = $2;
	  }
	| TIMEOUT ident
	  {
	    $2->type = TIMEOUTS_ACTION;
	    $$ = $2;
	  }
	| TIMEOUT NUMBER
	  {
	    $$ = new_wordentry( "timeout" );
	    $$->type = TIMEOUTV_ACTION;
	    $$->u.timeoutval = $2;
	  }
	| NEXT ident
	  { $$ = $2; add_next( $2 ); }
	| INLINE VTEXT
	  {
	    $$ = new_wordentry( "inline" );
	    $$->type = VERBATIM_ACTION;
	    $$->u.v = $2;
	  }
	| EXIT
	  {
	    $$ = new_wordentry( "exit" );
	    $$->type = EXIT_ACTION;
	  }
	;

casedefs
	: casedef
	  { $$ = $1; }
	| casedef casedefs
	  { $1->next = $2; $$ = $1; }
	;

casedef
	: CASE ident LB actiondefs RB
	  {
	    $2->type = CALL_RESULT;
	    $2->ex[0] = $4;
	    $$ = $2;
	  }
	| DEFAULT LB actiondefs RB
	  {
	    $$ = new_wordentry( "default" );
	    $$->type = CALL_DEFRESULT;
	    $$->ex[0] = $3;
	  }
	;

ident
	: IDENT
	  { $$ = new_wordentry( $1 ); free( $1 ); }
	;

%%

void
yyerror( char * e )
{
    line_error( "error: %s", e );
	exit( 1 );
}

int
yywrap( void )
{
	return 1;
}
