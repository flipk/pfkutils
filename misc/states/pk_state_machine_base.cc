
/*
    This file is part of the "pfkutils" tools written by Phil Knaack
    (pfk@pfk.org).
    Copyright (C) 2008  Phillip F Knaack

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "pk_state_machine_base.H"

extern "C" {
    void sprintf( char *, char *, ... );
    void printf( char *, ... );
    int tickGet( void );
    void memset( void *, int, int );
    void memcpy( void *, void *, int );
    void * malloc( int );
    void free( void * );
    int strlen( char * );
};

PK_STATE_MACHINE_BASE :: PK_STATE_MACHINE_BASE( char * _name )
{
    current_state = INVALID_STATE;
    memset( &logentries, 0, sizeof( logentries ));
    logpos = 0;
    int namelen = strlen( _name ) + 1;
    name = (char*)malloc( namelen );
    memcpy( name, _name, namelen );
}

PK_STATE_MACHINE_BASE :: ~PK_STATE_MACHINE_BASE( void )
{
    free( name );
}

void
PK_STATE_MACHINE_BASE :: first_call( void )
{
    _first_call();
    call_pre_hooks();
}

PK_STATE_MACHINE_BASE :: transition_return
PK_STATE_MACHINE_BASE :: transition( void * m )
{
    int it;

    it = input_discriminator( m );
    if ( it == UNKNOWN_INPUT )
    {
        unknown_message( m );
        return TRANSITION_OK;
    }

    if ( !valid_input_this_state( it ))
    {
        unhandled_message( it );
        return TRANSITION_OK;
    }

    cancel_timer();
    next_state = INVALID_STATE;

    if ( _transition( it ) == TRANSITION_EXIT )
        return TRANSITION_EXIT;

    if ( next_state != INVALID_STATE )
    {
        logentry * le;
        debug_transition_hook( it, current_state, next_state );

        le = &logentries[ logpos ];
        le->prev = current_state;
        le->input = it;
        le->next = next_state;
        le->time = tickGet();

        logpos = (logpos + 1) % numlogentries;

        current_state = next_state;
        call_pre_hooks();
    }

    return TRANSITION_OK;
}

void
PK_STATE_MACHINE_BASE :: printhist( void (*printfunc)(char *) )
{
    int i;
    logentry * le;
    char outline[ 100 ];

#define FORMAT1 "%-8s %-31s %-31s %-31s\n"
#define FORMAT2 "%8d %-31s %-31s %-31s\n"

    sprintf( outline, "\n\n" FORMAT1, 
             "time", "prev", "input", "next" );
    printfunc( outline );

    i = logpos;
    do { 
        le = &logentries[i];
        if ( le->time != 0 )
        {
            sprintf( outline, FORMAT2,
                     le->time,
                     dbg_state_name( le->prev ),
                     dbg_input_name( le->input ),
                     dbg_state_name( le->next ) );
            printfunc( outline );
        }
        i = (i+1)%numlogentries;
    } while ( i != logpos );
}

char *
PK_STATE_MACHINE_BASE :: current_state_name( void )
{
    return dbg_state_name( current_state );
}
