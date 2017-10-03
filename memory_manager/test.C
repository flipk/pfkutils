/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

#define ULONG unsigned int
#define NULL 0

#include <stdio.h>
#include <time.h>

#include "dll2.H"
#include "memory_region_manager.H"

struct item {
    char * addr;
    ULONG len;
    char value;
    char used;

    item( void ) {
        addr  = 0; len  = 0;
        value = 0; used = 0;
    }
};

#define ITEMS       10000
#define ITERS     1000000 
#define MAXLEN       2000
#define POOLSIZE 10000000
#define HASHSIZE     1000

struct item items[ ITEMS ];

#define mymalloc 1

int
main( int argc, char ** argv )
{
    int iter, ind, key;
    struct item * i;
    
    memory_region_manager  p( POOLSIZE, HASHSIZE );

    key = time(0) * getpid();
//    key = 2061874888;
    srandom( key );
//    printf( "srandom %d\n", key );

    for ( iter = 0; iter < ITERS;  )
    {
        ind = random() % ITEMS;
        i = &items[ind];

        if ( i->used )
        {
            // free it
            if ( i->addr[ i->len - 1 ] != i->value )
                printf( "error 1!\n" );

#if mymalloc
            p.free( i->addr );
#else
            free( i->addr );
#endif
            i->used = 0;
// printf( "index %d ptr %#x size %d freed\n", ind, i->addr, i->len );
        }
        else
        {
            // alloc it
            i->len = random() % MAXLEN;

            if ( i->len == 0 )
                i->len = 1;
#if mymalloc
            i->addr = (char*) p.malloc( i->len );
#else
            i->addr = (char*) malloc( i->len );
#endif

            iter++;

            if ( i->addr == 0 )
                printf( "malloc failed!\n" );
            else
            {
                i->value = random() % 256;
                i->addr[ i->len - 1 ] = i->value;
                i->used = 1;
// printf( "index %d ptr %#x size %d allocated\n", ind, i->addr, i->len );
            }
        }
    }

    for ( ind = 0; ind < ITEMS; ind++ )
    {
        i = &items[ind];
        if ( i->used )
        {
#if mymalloc
            p.free( i->addr );
#else
            free( i->addr );
#endif
// printf( "index %d ptr %#x size %d freed\n", ind, i->addr, i->len );
        }
    }
}
