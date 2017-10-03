
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define MAX_PK_TCP_MSG_LENGTH 2048

#include "pk_tcp_msg.H"
#include "treesync.h"
#include "trade.H"
#include "regen.H"

#define MAX_PK_PATH    1024
#define MAX_PIECE_LEN  1024

MESSAGE_LIST;

bool            tcp_closed = false;
pk_tcp_msgr   * tcp_channel;
bool            trade_first;
FILE          * conflict_list;

static int
trade_read( void * arg, int fd, void * buf, int buflen )
{
    int cc = read( fd, buf, buflen );
    if ( cc <= 0 )
        tcp_closed = true;
//    printf( "read %d of %d bytes from fd %d\n", cc, buflen, fd );
    return cc;
}

static int
trade_write( void * arg, int fd, void * buf, int buflen )
{
    int cc = write( fd, buf, buflen );
//    printf( "wrote %d of %d bytes on fd %d\n", cc, buflen, fd );
    return cc;
}

void
trade_init( int fd, bool _first )
{
    tcp_channel = new pk_tcp_msgr( trade_read, trade_write, NULL, fd );

    trade_first = _first;

    RegenStarted  start;
    tcp_channel->send( &start );

    if ( tcp_channel->recv( &start, sizeof(start) ) == false )
    {
        fprintf( stderr, "error negotiating link\n" );
        exit(1);
    }

    if ( start.get_type() != RegenStarted::TYPE )
    {
        fprintf( stderr, "invalid msg type %#x received!\n",
                 start.get_type() );
        exit(1);
    }

    printf( "tcp connection negotiated\n" );

    conflict_list = fopen( TREESYNC_CONFLICT_FILE, "w" );
}

void
trade_close( void )
{
    delete tcp_channel;
    tcp_channel = NULL;
    fclose( conflict_list );
}


static void
check_parent_dir( char * path )
{
    char newpath[ MAX_PK_PATH ];
    char * slash;

    strcpy( newpath, path );

    for ( slash = newpath; *slash; slash++ )
    {
        if ( *slash == '/' )
        {
            *slash = 0;
            (void) mkdir( newpath, 0755 );
            *slash = '/';
        }
    }
}

static void
get_affected_file(void)
{
    MaxPkMsgType          mpmt;
    AffectedFileStatus    afs;
    AffectedFile        * af;
    AffectedDone        * ad;

    FileEntry * fe;
    int file_count = 0;

    time_t now, last_print;

    printf( "getting file list\n" );

    time( &last_print );
    while ( 1 )
    {
        if ( tcp_channel->recv( &mpmt, sizeof(mpmt) ) == false )
            kill(0,6);
        if ( mpmt.convert( &af ))
        {
            file_count++;
            afs.conflicted = 0;
            for ( fe = file_list.get_head();
                  fe; fe = file_list.get_next(fe) )
            {
                if ( strcmp( fe->filename, af->filename ) == 0 )
                    afs.conflicted = 1;
            }
            tcp_channel->send( &afs );
        }
        else if ( mpmt.convert( &ad ))
        {
            break;
        }
        else
            kill(0,6);

        if ( time( &now ) != last_print )
        {
            printf( "\r  files: %d   ", file_count );
            fflush( stdout );
            last_print = now;
        }
    }
    printf( "\r  files: %d   \n", file_count );
    fflush( stdout );
}

void
put_affected_file(void)
{
    AffectedFile        af;
    AffectedFileStatus  afs;
    AffectedDone        ad;

    FileEntry * fe, * nfe;

    printf( "sending file list\n" );

    for ( fe = file_list.get_head(); fe; fe = nfe )
    {
        nfe = file_list.get_next(fe);

        strcpy( af.filename, fe->filename );
        tcp_channel->send( &af );

        if ( tcp_channel->recv( &afs, sizeof(afs) ) == false )
            kill(0,6);

        if ( afs.get_type() != AffectedFileStatus::TYPE )
            kill(0,6);

        if ( afs.conflicted )
        {
            fprintf( stderr, "file conflict: %s\n", fe->filename );
            fprintf( conflict_list, "%s\n", fe->filename );
            fe->type = FileEntry::CONFLICT;
        }
    }

    tcp_channel->send( &ad );
}

void
print_file_summary(void)
{
    FileEntry * fe, * nfe;
    int count_new, count_changed, count_removed, count_conflict;
    UINT32 bytes_new, bytes_changed;

    count_new = count_changed = count_removed = count_conflict = 0;
    bytes_new = bytes_changed = 0;
    for ( fe = file_list.get_head(); fe; fe = nfe )
    {
        nfe = file_list.get_next(fe);

        switch ( fe->type )
        {
        case FileEntry::NEW:
            count_new++;
            bytes_new += fe->file_size;
            break;
        case FileEntry::CHANGED:
            count_changed++;
            bytes_changed += fe->file_size;
            break;
        case FileEntry::REMOVED:
            count_removed++;
            break;
        case FileEntry::CONFLICT:
            count_conflict++;
            break;
        }
    }

    printf( "new     files = %d   (%d bytes)\n"
            "changed files = %d   (%d bytes)\n"
            "removed files = %d\n"
            "conflict files = %d\n",
            count_new, bytes_new,
            count_changed, bytes_changed,
            count_removed,
            count_conflict );
}

void
get_created_changed(void)
{
    MaxPkMsgType           mpmt;
    ChangedFileName      * cfn;
    ChangedFileContents  * cfc;
    ChangedFileDone      * cfd;

    int files_done = 0;
    UINT32 bytes_done = 0;
    time_t now, last_print;

    int fd;
    UINT32 remaining;

    printf( "getting file contents\n" );
    time( &last_print );

    while ( 1 )
    {
        if ( tcp_channel->recv( &mpmt, sizeof(mpmt) ) == false )
            kill(0,6);
        if ( mpmt.convert( &cfn ))
        {
            files_done++;
            check_parent_dir( cfn->filename );
            fd = open( cfn->filename, O_WRONLY | O_CREAT, 0644 );
            if ( fd < 0 )
            {
                fprintf( stderr, "open failed: %s: %s\n",
                         cfn->filename, strerror( errno ));
            }
            remaining = cfn->file_length.get();
//            printf( "writing '%s'\n", cfn->filename );
        }
        else if ( mpmt.convert( &cfc ))
        {
            UINT16  piece_len = cfc->piece_length.get();
            if ( fd > 0 )
                write( fd, cfc->buffer, piece_len );
            remaining -= piece_len;
            bytes_done += piece_len;
            if ( remaining == 0  &&  fd > 0 )
                close( fd );
        }
        else if ( mpmt.convert( &cfd ))
        {
            break;
        }
        else
            kill(0,6);        

        if ( time( &now ) != last_print )
        {
            printf( "\r  transferred %d files (%d bytes)   ",
                    files_done, bytes_done );
            fflush( stdout );
            last_print = now;
        }
    }
    printf( "\r  transferred %d files (%d bytes)   \n",
            files_done, bytes_done );
}

void
put_created_changed(void)
{
    ChangedFileName      cfn;
    ChangedFileContents  cfc;
    ChangedFileDone      cfd;

    struct stat sb;
    UINT32 remaining, pos, blk_size;
    int cc, fd;
    FileEntry * fe, * nfe;

    printf( "sending file contents\n" );

    for ( fe = file_list.get_head(); fe; fe = nfe )
    {
        nfe = file_list.get_next(fe);
        if (( fe->type != FileEntry::CHANGED ) && 
            ( fe->type != FileEntry::NEW ))
            // skip this file
            continue;

        if ( stat( fe->filename, &sb ) < 0 )
        {
            fprintf( stderr, "error : cannot open '%s'\n", fe->filename );
            continue;
        }

        fd = open( fe->filename, O_RDONLY );
        if ( fd < 0 )
        {
            fprintf( stderr, "error : cannot open2 '%s'\n", fe->filename );
            continue;
        }

        strcpy( cfn.filename, fe->filename );
        remaining = sb.st_size;
        cfn.file_length.set( remaining );
        pos = 0;

        tcp_channel->send( &cfn );

//        printf( "sending '%s' of size %d\n", cfn.filename, remaining );
        while ( remaining > 0 )
        {
            blk_size = remaining;
            if ( remaining > MAX_PIECE_LEN )
                blk_size = MAX_PIECE_LEN;
            cfc.piece_length.set( blk_size );
            cc = read( fd, cfc.buffer, blk_size );
            if ( cc != blk_size )
            {
                fprintf( stderr, "cannot read proper size (%d != %d)\n",
                         blk_size, cc );
            }

            tcp_channel->send( &cfc );
            remaining -= blk_size;
        }

        close( fd );
    }

    tcp_channel->send( &cfd );
}

void
get_removed(void)
{
    MaxPkMsgType        mpmt;
    RemovedFileName   * rfn;
    SyncDone          * sd;

    printf( "getting remove list\n" );

    while ( 1 )
    {
        if ( tcp_channel->recv( &mpmt, sizeof(mpmt) ) == false )
            kill(0,6);
        if ( mpmt.convert( &rfn ))
        {
//            fprintf( stderr, "removing '%s'\n", rfn->filename );
            unlink( rfn->filename );
        }
        else if ( mpmt.convert( &sd ))
        {
            break;
        }
        else
            kill(0,6);
    }
}

void
put_removed(void)
{
    RemovedFileName     rfn;
    SyncDone            sd;
    FileEntry * fe, * nfe;

    printf( "sending remove list\n" );

    for ( fe = file_list.get_head(); fe; fe = nfe )
    {
        nfe = file_list.get_next(fe);
        if ( fe->type != FileEntry::REMOVED )
            // skip this file
            continue;

        strcpy( rfn.filename, fe->filename );

        tcp_channel->send( &rfn );
    }

    tcp_channel->send( &sd );
}

void
trade_files( void )
{
    {
        RegenDone done;
        tcp_channel->send( &done );

        if ( tcp_channel->recv( &done, sizeof(done) ) == false )
        {
            fprintf( stderr, "error waiting for remote host done msg!\n" );
            exit(1);
        }
        if ( done.get_type() != RegenDone::TYPE )
        {
            fprintf( stderr, "illegal msg type %#x received!\n",
                     done.get_type() );
            exit(1);
        }
    }

    if ( trade_first )
    {
        put_affected_file();
        get_affected_file();
        print_file_summary();
        put_created_changed();
        get_created_changed();
        put_removed();
        get_removed();
    }
    else
    {
        get_affected_file();
        put_affected_file();
        print_file_summary();
        get_created_changed();
        put_created_changed();
        get_removed();
        put_removed();
    }

}
