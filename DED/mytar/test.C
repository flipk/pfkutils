
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
#include "update_dir.H"
#include "extract_file.H"

#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>

class list_iterator : public file_db_iterator {
public:
    file_info_list  list;
    virtual void file( file_info * f ) {
        list.add( f );
    }
};


int
main( int argc, char ** argv )
{
    file_db * db;

#if 0
    unlink( "0.db" );
    db = new file_db( "0.db", true );
#else
    db = new file_db( "0.db", false );
#endif

#if 0
    /* update file */
    update_file( db, "1" );
    update_file( db, "file_db.o" );
    update_file( db, "test.C" );
    update_file( db, "update_file.o" );
#elif 0
    /* update dir */
    update_dir( db, "TEST" );
#elif 0
    /* list contents */
    list_iterator   li;
    db->iterate( &li );
    file_info * fi;
    while ( fi = li.list.dequeue_head() )
    {
        printf( "file: %s\n", fi->fname );
        delete fi;
    }
#elif 0
    /* extract */

    extract_file( db, "TEST/pfkutils/old.threads/h_internal/Attic/threads_messages_internal.H,v" );

#elif 1
    /* extract all */
    list_iterator   li;
    db->iterate( &li );
    file_info * fi;
    while ( fi = li.list.dequeue_head() )
    {
        extract_file( db, fi->fname );
        delete fi;
    }
#endif

    delete db;

    return 0;
}
