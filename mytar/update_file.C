
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
            changed = true;
            inf->size = sb.st_size;
            inf->mtime = sb.st_mtime;
        }
        id = inf->id;
        db->update_info( inf );
    }

    if ( !changed )
        return;

    UINT32 piece, changed_pieces = 0;
    int fd;
    fd = open( fname, O_RDONLY );
    printf( "%s: ", fname );
    fflush(stdout);
    if ( fd < 0 )
    {
        fprintf( stderr, "open: %s\n", strerror( errno ));
        piece = 0;
    }
    else
    {
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
    }
    close( fd );

    db->truncate_pieces( id, piece );
}
