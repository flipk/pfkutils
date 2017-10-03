/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

// these are methods that redmalloc provides.
extern "C" void * _redmalloc( int size, int pc );
extern "C" void * _redcalloc( int num, int size, int pc );
extern "C" void * _redrealloc( void * p, int new_sz, int pc );
extern "C" void   _redfree( void * ptr );
extern "C" void   _redmallocaudit( void );

extern "C" void redmallocspawntask( void );
extern "C" void red_malloc_display( void );
