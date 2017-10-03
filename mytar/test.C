
#include "file_db.H"
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>

int
main( int argc, char ** argv )
{
    file_db * db;
    file_info * fi;
    struct stat sb;

    stat( "ZZZZ", &sb );

    db = new file_db( "db", true );

    fi = new file_info( "ZZZZ" );

    fi->size = sb.st_size;
    fi->mtime = sb.st_mtime;
    fi->uid = sb.st_uid;
    fi->gid = sb.st_gid;
    fi->mode = sb.st_mode & 0777;

    db->add_info( fi );

    UINT32 pos;
    UINT32 block_num;
    int fd = open( "ZZZZ", O_RDONLY );

    for ( pos = 0, block_num = 0;
          pos < fi->size; 
          pos += file_db::BLOCK_SIZE, block_num++ )
    {
        char buf[ file_db::BLOCK_SIZE ];
        int cc;
        cc = read( fd, buf, file_db::BLOCK_SIZE );
        if ( cc <= 0 )
            break;
        db->update_block( fi, block_num, buf, cc );
    }

    delete fi;
    delete db;

    return 0;
}
