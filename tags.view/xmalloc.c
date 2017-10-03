
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"

struct minfo {
    int size;
    char data[0];
};

int xmalloc_allocated = 0;

void *
xmalloc( int size )
{
    struct minfo * mi;

    mi = (struct minfo *) malloc( sizeof( struct minfo ) + size );
    if ( mi == NULL )
    {
        printf( "xmalloc of %d bytes failed!\n", size );
        exit( 1 );
    }

    mi->size = size;
    xmalloc_allocated += size;

    memset( mi->data, 0, size );
    return (void *)mi->data;
}

void
xfree( void * d )
{
    struct minfo * mi;

    mi = (struct minfo*)((char*)d-4);
    xmalloc_allocated -= mi->size;

    free( mi );
}
