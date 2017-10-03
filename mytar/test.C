
#include "file_db.H"
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>

int
main( int argc, char ** argv )
{
    file_db * db;
    file_info * fi;
    UINT32 file_id;
    struct stat sb;

    stat( "1", &sb );


    // xxx BUGFIX why this doesn't work?

#if 1
    unlink( "0.db" );
    db = new file_db( "0.db", true );
#else
    db = new file_db( "0.db", false );
#endif


#if 0
    fi = db->get_info_by_fname( "1" );
    if ( !fi )
        printf( "not found\n" );

    file_id = fi->id;
    delete fi;
#else
    fi = new file_info( "1" );

    fi->size = sb.st_size;
    fi->mtime = sb.st_mtime;
    fi->uid = sb.st_uid;
    fi->gid = sb.st_gid;
    fi->mode = sb.st_mode & 0777;

    file_id = db->add_info( fi );
#endif

    UINT64 pos;
    UINT32 piece_num;
    int fd = open( "1", O_RDONLY );

    for ( pos = 0, piece_num = 0;
          pos < sb.st_size;
          pos += file_db::PIECE_SIZE, piece_num++ )
    {
        char buf[ file_db::PIECE_SIZE ];
        int cc;
        cc = read( fd, buf, file_db::PIECE_SIZE );
        if ( cc <= 0 )
            break;
        db->update_piece( file_id, piece_num, buf, cc );
    }

    delete db;

    return 0;
}
