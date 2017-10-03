/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

void myputs( char * s );
void myputwhitespace( void );
void myputpreproc( char * s );
void consume_preproc( int (*func)(void) );
void consume_define( int (*func)(void) );
