
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

void count_line( void );
void list_add( WENTLIST *, WENT * );
void machine_addname( WENT * w );
void inputlist_add_timeout( void );
void statelist_add_unknown( void );
void add_state( char * statename, WENT * pre, WENT * inputs );
void add_next( WENT * state );
WENT * lookup_inputname( char * );
WENT * lookup_outputname( char * );
void add_call( WENT * call );
VERBATIM * copy_verbatim( int (*)(void) );
