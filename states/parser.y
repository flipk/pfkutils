
%{

#include <stdio.h>
#include "wordentry.h"
#include "parse_actions.h"
#include "machine.h"

extern int yylex( void );

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

%token MESSAGE_TYPE
%token VTEXT_CARGS
%token VTEXT_CCODE
%token VTEXT_DCODE
%token VTEXT_STARTHDR
%token VTEXT_DATA
%token VTEXT_ENDHDR
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
%type  <we>    calldefs
%type  <we>    calldef

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
	| MESSAGE_TYPE VTEXT
	  { machine.message_type = $2; }
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
	  { $1->next = $2; $$ = $1; }
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
	| SWITCH ident LB calldefs RB
	  {
	    $2->type = SWITCH_ACTION;
	    $2->ex[0] = $4;
	    $$ = $2;
	  }
	| CALL ident LB calldefs RB
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

calldefs
	: calldef
	  { $$ = $1; }
	| calldef calldefs
	  { $1->next = $2; $$ = $1; }
	;

calldef
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