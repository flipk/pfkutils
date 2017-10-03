
/*
    This file is part of the "pkutils" tools written by Phil Knaack
    (pknaack1@netscape.net).
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
