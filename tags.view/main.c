/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

#include <stdio.h>
#include <string.h>

#include "main.h"

int
viewtags_main( int argc, char ** argv )
{
    char * tagsfilename = "tags";
    char * tagname;
    TAGS_FILE * tf;

    switch ( argc )
    {
    case 3:
        /* -f tagsfile plus tagname */
        if ( strcmp( argv[1], "-f" ) != 0 )
            goto out;
        tagsfilename = argv[2];
        break;

    case 1:
        /* only tagname */
        break;

    default:
        out:
        printf( "usage: viewtags [-f tagsfile] tagname\n" );
        return 1;
    }

    tf = viewtags_tags_open( tagsfilename );
    printf( "%d tags in %d files (%d bytes used)\n",
            tf->numtags, tf->numfiles, xmalloc_allocated );

    while (( tagname = viewtags_input_tag()) != NULL )
        viewtags_display_tags( tf, tagname );

    viewtags_tags_close( tf );
    return 0;
}
