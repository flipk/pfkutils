
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

    extract_file( db, "TEST/pkutils/old.threads/h_internal/Attic/threads_messages_internal.H,v" );

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
