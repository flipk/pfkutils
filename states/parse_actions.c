
#include <stdio.h>
#include <stdlib.h>
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

    to = new_wordentry( "UNKNOWN_INPUT" );
    to->type = INPUT_UNKNOWN;
    list_add( &machine.inputs, to );
}

static WENT *
find_word( WENTLIST * lst, char * w )
{
    WENT * i = lst->head;
    for ( ; i; i = i->next )
        if ( strcmp( i->word, w ) == 0 )
            break;
    if ( !i )
    {
        line_error( "word %s not found in %s definition list!",
                    w, lst->listname );
        exit( 1 );
    }
    return i;
}

void
add_state( char * statename, WENT * pre, WENT * inputs )
{
    WENT * st;

    st = find_word( &machine.states, statename );

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
    WENT * st = find_word( &machine.states, statename->word );

    statename->type = NEXT_ACTION;
    statename->ex[0] = st;
}

WENT *
lookup_inputname( char * w )
{
    return find_word( &machine.inputs, w );
}

WENT *
lookup_outputname( char * w )
{
    return find_word( &machine.outputs, w );
}

void
add_call( WENT * call )
{
    *machine.next_call = call;
    machine.next_call = &call->ex[1];
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
