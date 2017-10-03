
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
#include "update_file.H"

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

void
update_file( file_db * db, char * fname )
{
    struct stat sb;

    if ( stat( fname, &sb ) < 0 )
    {
        fprintf( stderr, "unable to stat '%s': %s\n",
                 fname, strerror( errno ));
        return;
    }

    file_info * inf = db->get_info_by_fname( fname );
    bool changed = false;
    UINT32 id;
    int fd = -1;
    if ( !inf )
    {
        // file not found in archive, add it new!
        inf = new file_info( fname );
        inf->uid = sb.st_uid;
        inf->gid = sb.st_gid;
        inf->mode = sb.st_mode & 0777;
        inf->size = sb.st_size;
        inf->mtime = sb.st_mtime;
        id = db->add_info( inf );
        changed = true;
        fd = open( fname, O_RDONLY );
    }
    else
    {
        // file found in archive, compare info.
        if ( (unsigned int)inf->uid != (unsigned int)sb.st_uid  ||
             (unsigned int)inf->gid != (unsigned int)sb.st_gid  ||
             inf->mode != (sb.st_mode & 0777) )
        {
            inf->uid = sb.st_uid;
            inf->gid = sb.st_gid;
            inf->mode = sb.st_mode & 0777;
        }

        if ( (UINT64)sb.st_size != inf->size   ||
             sb.st_mtime != inf->mtime )
        {
            inf->size = sb.st_size;
            inf->mtime = sb.st_mtime;
            fd = open( fname, O_RDONLY );
            if ( fd < 0 )
                fprintf( stderr, "open: %s\n", strerror( errno ));
            else
            {
                char buf;
                if ( read( fd, &buf, 1 ) < 0 )
                {
                    fprintf( stderr, "read: %s: %s\n",
                             fname, strerror( errno ));
                    close(fd);
                }
                else
                    changed = true;
            }
        }
        id = inf->id;
        db->update_info( inf );
    }

    if ( !changed || fd < 0 )
        return;

    UINT32 piece, changed_pieces = 0;
    printf( "%s: ", fname );
    fflush(stdout);

    for ( piece = 0; ; piece++ )
    {
        char buf[ file_db::PIECE_SIZE ];
        int cc = read( fd, buf, sizeof(buf) );
        if ( cc <= 0 )
            break;
        if ( db->update_piece( id, piece, buf, cc )) 
            changed_pieces++;
    }
    printf( "%d/%d\n", changed_pieces, piece );
    close( fd );

    db->truncate_pieces( id, piece );
}
