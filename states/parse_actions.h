/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
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
