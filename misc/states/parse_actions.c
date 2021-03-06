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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wordentry.h"
#include "parse_actions.h"
#include "machine.h"

void
list_add( WENTLIST * lst, WENT * we )
{
    if ( !lst->tail )
    {
        lst->head = lst->tail = we;
    }
    else
    {
        lst->tail->next = we;
        lst->tail = we;
    }
}

void
count_line( void )
{
    machine.line_number++;
}

void
machine_addname( WENT * w )
{
    w->type = MACHINE_NAME;
    list_add( &machine.name, w );
}

void
inputlist_add_timeout( void )
{
    WENT * to;

    to = new_wordentry( "TIMEOUT_INPUT" );
    to->type = INPUT_TIMEOUT;
    list_add( &machine.inputs, to );

#if 0
    to = new_wordentry( "UNKNOWN_INPUT" );
    to->type = INPUT_UNKNOWN;
    list_add( &machine.inputs, to );
#endif
}

static WENT *
find_word( WENT * head, char * listname, char * w, int error )
{
    WENT * i = head;
    for ( ; i; i = i->next )
        if ( strcmp( i->word, w ) == 0 )
            break;
    if ( error && !i )
    {
        line_error( "word %s not found in %s definition list!",
                    w, listname );
        exit( 1 );
    }
    return i;
}

void
add_state( char * statename, WENT * pre, WENT * inputs )
{
    WENT * st;

    st = find_word( machine.states.head,
                    machine.states.listname, statename, 1 );

    if ( st->ex[0] || st->ex[1] )
    {
        line_error( "state %s is already defined!", statename );
        exit( 1 );
    }

    st->ex[0] = pre;
    st->ex[1] = inputs;
}

void
add_next( WENT * statename )
{
    WENT * st = find_word( machine.states.head, machine.states.listname,
                           statename->word, 1 );

    statename->type = NEXT_ACTION;
    statename->ex[0] = st;
}

WENT *
lookup_inputname( char * w )
{
    return find_word( machine.inputs.head, machine.inputs.listname, w, 1 );
}

WENT *
lookup_outputname( char * w )
{
    return find_word( machine.outputs.head, machine.outputs.listname, w, 1 );
}

void
call_name_res_add( WENT ** list, WENT * item )
{
    WENT * cur, * prev;

    for ( prev = NULL, cur = *list;
          cur && cur->type != CALL_DEFRESULT;
          prev = cur, cur = cur->next )
    {
        /* nothing */
    }

    if ( !prev )
        *list = item;
    else
        prev->next = item;

    item->next = cur;
}

void
add_call( WENT * call_act )
{
    WENT * call_name, * call_res, * crw;

    /* also add a duplicate of this call object to
       the calls-list, and if it already exists, walk the
       list of call-results looking for ones we have not
       seen before and update the list with duplicates if
       they are found. */

    call_name = find_word( machine.calls.head, machine.calls.listname,
                           call_act->word, 0 );
    if ( !call_name )
    {
        call_name = new_wordentry( call_act->word );
        call_name->type = CALL_NAME;
        list_add( &machine.calls, call_name );
    }

    call_act->ex[1] = call_name;

    for ( crw = call_act->ex[0]; crw; crw = crw->next )
    {
        call_res = find_word( call_name->ex[0], "call_name", crw->word, 0 );
        if ( !call_res )
        {
            call_res = new_wordentry( crw->word );
            call_res->type = crw->type;
            call_name_res_add( &call_name->ex[0], call_res );
        }
        crw->ex[1] = call_res;
    }
}

VERBATIM *
copy_verbatim( int (*func)(void) )
{
    int c[3] = { -1, -1, -1 };
    VERBATIM * v;

    v = (VERBATIM *)malloc( sizeof( VERBATIM ));

    v->usedlen = 0;
    v->alloclen = 100;
    v->data = (char*) malloc( v->alloclen );

#define ADDBYTE(byte) \
    do { \
            if ( v->usedlen == v->alloclen ) \
            { \
                v->alloclen *= 2; \
                v->data = (char*) realloc( v->data, v->alloclen ); \
            } \
            v->data[ v->usedlen++ ] = byte; \
    } while ( 0 )

    /*
     * copy data in using func() until we
     * see a "%" (ascii 37 decimal)
     * followed by "}" (ascii 125 decimal)
     * but don't copy these two to the output.
     * a three-byte shift register will do it.
     */

    while ( 1 )
    {
        c[0] = c[1];
        c[1] = c[2];
        c[2] = func();

        if ( c[0] != -1 )
            ADDBYTE(c[0]);

        if ( c[2] == '\n' || c[2] == '\r' )
            count_line();

        if ( c[1] == 37 && c[2] == 125 )
            break;
    }

    ADDBYTE( 0 );

    return v;
}
