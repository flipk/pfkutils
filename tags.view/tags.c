/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "main.h"

static void
bail( void )
{
    printf( "error while reading tags file: inappropriate format\n" );
    exit( 1 );
}

TAGS_FILE *
viewtags_tags_open( char * file )
{
    int i, numargs;
    TAGS_FILE * tf;

    tf = xmalloc( sizeof( TAGS_FILE ));

    tf->f = fopen( file, "r" );
    if ( tf->f == NULL )
    {
        printf( "tags_open: error opening %s\n", file );
        xfree( tf );
        exit( 1 );
    }

    if ( viewtags_get_line( tf->f ) < 0 )
        bail();

    if ( strcmp( viewtags_lineargs[0], "FILES" ) != 0 )
        bail();

    tf->numfiles = atoi( viewtags_lineargs[1] );

    tf->files = xmalloc( sizeof( struct fileinfo ) * tf->numfiles );

    for ( i = 0; i < tf->numfiles; i++ )
    {
        int len;

        numargs = viewtags_get_line( tf->f );
        if ( numargs != 2 )
            bail();

        if ( !isdigit( viewtags_lineargs[0][0] ))
            bail();

        if ( atoi( viewtags_lineargs[0] ) != i )
            bail();

        len = strlen( viewtags_lineargs[1] ) + 1;
        tf->files[i].filename = xmalloc( len );
        memcpy( tf->files[i].filename, viewtags_lineargs[1], len );
    }

    numargs = viewtags_get_line( tf->f );

    if ( numargs != 1 || strcmp( viewtags_lineargs[0], "ENDFILES" ) != 0 )
        bail();

    numargs = viewtags_get_line( tf->f );

    if ( numargs != 2 || strcmp( viewtags_lineargs[0], "TAGS" ) != 0 )
        bail();

    tf->numtags = atoi( viewtags_lineargs[1] );
    tf->tags = xmalloc( sizeof( struct taginfo * ) * tf->numtags );

    for ( i = 0; i < tf->numtags; )
    {
        int len;
        numargs = viewtags_get_line( tf->f );
        if ( numargs == -1 )
            bail();
        if ( numargs != 2 || strcmp( viewtags_lineargs[0], "TAG" ) != 0 )
            continue;
        len = strlen( viewtags_lineargs[1] ) + 1;
        tf->tags[i] = xmalloc( sizeof( struct taginfo * ) + len );
        memcpy( tf->tags[i]->tagname, viewtags_lineargs[1], len );
        tf->tags[i]->tagfileoffset = (int)ftell( tf->f );
        i++;
    }

    return tf;
}

void
viewtags_tags_close( TAGS_FILE * tf )
{
    int i;
    fclose( tf->f );
    xfree( tf->files );
    for ( i = 0; i < tf->numtags; i++ )
        xfree( tf->tags[i] );
    xfree( tf->tags );
    xfree( tf );
}
