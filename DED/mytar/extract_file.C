
/*
    This file is part of the "pfkutils" tools written by Phil Knaack
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

#include "file_db.H"
#include "extract_file.H"

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

void
extract_file( file_db * db, char * fname )
{
    file_info * inf = db->get_info_by_fname( fname );

    if ( !inf )
    {
        fprintf( stderr, "file '%s' not found in database!\n", fname );
        return;
    }

    /* ensure parent directory of file exists */
    int l = strlen( fname ) + 1;
    char * dirname = new char[ l ];
    char * dirname_p = dirname;
    memcpy( dirname, fname, l );

    while (( dirname_p = strchr( dirname_p, '/' )))
    {
        *dirname_p = 0;

        mkdir( dirname, 0700 );

        *dirname_p++ = '/';
    }

    delete[] dirname;

    int fd = open( fname, O_WRONLY | O_CREAT, 0600 );
    if ( fd < 0 )
    {
        fprintf( stderr, "opening output file '%s': %s\n",
                 fname, strerror( errno ));
        delete inf;
        return;
    }

    UINT32 piece;

    for ( piece = 0; ; piece++ )
    {
        // build in some extra space in the buffer,
        // because if the file was originally compressed,
        // the data stored in the db might actually be larger
        // than the original file (compressing a file which is
        // already compressed may make it bigger rather than smaller)

        char buf[ file_db::PIECE_SIZE + 100 ];
        int buflen = sizeof(buf);
        db->extract_piece( inf->id, piece, buf, &buflen );
        if ( buflen == 0 )
            break;
        if ( write( fd, buf, buflen ) != buflen )
        {
            fprintf( stderr, "writing output file '%s': %s\n",
                     fname, strerror( errno ));
            close( fd );
            delete inf;
            return;
        }
    }
    close( fd );
    delete inf;
}

