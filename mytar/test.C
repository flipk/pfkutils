
#include "file_db.H"
#include "update_file.H"

#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>

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

#if 1
    update_file( db, "1" );
    update_file( db, "test.C" );
    update_file( db, "t" );
#else
    /**/
#endif

    delete db;

    return 0;
}
