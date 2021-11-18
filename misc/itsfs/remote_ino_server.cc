
#include "remote_ino.H"
#include "lognew.H"
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>

struct opendir_path {
    DIR * dir;
    char * filepos;
    char path[0];
    void * operator new( size_t s, uchar * _p )
        {
            int len = strlen( (char*)_p );
            opendir_path * ret =
                (opendir_path *) MALLOC( sizeof( opendir_path ) + len + 256 );
            memcpy( ret->path, _p, len );
            ret->filepos = &ret->path[len];
            return (void*) ret;
        }
    void operator delete( void * p )
        {
            free( p );
        }
    opendir_path( DIR * _d )
        {
            dir = _d;
        };
    char * addfile( uchar * file )
        {
            sprintf( filepos, "/%s", file );
            return (char*)path;
        }
};

int
remote_inode_server :: readdirstat( opendir_path * op,
                                    struct stat * sb,
                                    remino_readdir_entry * ent )
{
    struct dirent * de = readdir( op->dir );
    if ( !de )
        return 0;

    ent->name = (uchar*) STRDUP( de->d_name );
    char * fullpath = op->addfile( ent->name );

    if ( symlinks )
    {
        int r = lstat( fullpath, sb );

        if ( r == 0 && !dirsymlinks &&
             ( sb->st_mode & S_IFMT ) == S_IFLNK )
        {
            struct stat sb2;
            r = stat( fullpath, &sb2 );
            if ( r == 0 && ( sb2.st_mode & S_IFMT ) == S_IFDIR )
                *sb = sb2;
        }
    }
    else
        if ( stat( fullpath, sb ) < 0 )
            return -1;

#if defined(CYGWIN)
    ent->fileid = 0;
#elif defined(SOLARIS)
    ent->fileid = de->d_ino;
#else
    ent->fileid = de->d_fileno;
#endif

    switch ( sb->st_mode & S_IFMT )
    {
    case S_IFLNK:
        if ( symlinks )
            ent->ftype = INODE_LINK;
        else
            ent->ftype = INODE_FILE;
        break;
    case S_IFDIR:
        ent->ftype = INODE_DIR;
        break;
    default:
        ent->ftype = INODE_FILE;
        break;
    }

    return 1;
}

void
remote_inode_server :: stat_to_xdrstat( remino_stat_reply * rst,
                                        struct stat * sb )
{
    rst->mode      = sb->st_mode;
    rst->nlink     = sb->st_nlink;
    rst->uid       = sb->st_uid;
    rst->gid       = sb->st_gid;
    rst->size      = sb->st_size;
    rst->blocksize = sb->st_blksize;
    rst->rdev      = sb->st_rdev;
    rst->blocks    = sb->st_blocks;
    rst->fsid      = sb->st_dev;
    rst->asec      = sb->st_atime;
    rst->ausec     = 0;
    rst->msec      = sb->st_mtime;
    rst->musec     = 0;
    rst->csec      = sb->st_ctime;
    rst->cusec     = 0;
    rst->fileid    = sb->st_ino;
}

#define printf( x ) if ( verbose ) printf x

uchar *
remote_inode_server :: dispatch( uchar * inpkt, int inlen, int &outlen )
{
    remino_call call;
    remino_reply reply;
    XDR xdrs;
    uchar * ret = replydata;
    struct stat sb;
    struct timeval tv[2];

    memset( &call, 0, sizeof( call ));
    memset( &reply, 0, sizeof( reply ));

    xdrs.encode_decode = XDR_DECODE;
    xdrs.data = inpkt;
    xdrs.position = 0;
    xdrs.bytes_left = inlen;

    if ( !myxdr_remino_call( &xdrs, &call ))
    {
        xdrs.encode_decode = XDR_FREE;
        myxdr_remino_call( &xdrs, &call );
        return NULL;
    }

    reply.reply = call.call;
    errno = 0;

#define  C  call.remino_call_u
#define  R  reply.remino_reply_u

    switch ( call.call )
    {
    case OPEN:
    {
        int flags = 0;
        if ( C.open.flags & remote_inode_client::CREATE_FLAG )
        {
            flags = O_CREAT;
        }
        if (( C.open.flags & ( remote_inode_client::READ_FLAG |
                               remote_inode_client::WRITE_FLAG )) ==
            ( remote_inode_client::READ_FLAG |
              remote_inode_client::WRITE_FLAG ))
        {
            flags |= O_RDWR;
        }
        else if (( C.open.flags & ( remote_inode_client::READ_FLAG |
                                    remote_inode_client::WRITE_FLAG ))
                 == remote_inode_client::READ_FLAG )
        {
            flags |= O_RDONLY;
        }
   
        R.open.fd = 
            open( (char*)C.open.path,
                  flags,
                  C.open.mode );
        R.open.err = errno;
        printf(( "OPEN %s -> fd %d errno %d\n", 
                 (char*)C.open.path, R.open.fd, R.open.err ));
        break;
    }
    case CLOSE:
        R.err.retval = 
            close( C.close.fd );
        R.err.err = errno;
        printf(( "CLOSE %d -> errno %d\n", C.close.fd, R.err.err ));
        break;

    case READ:
        R.read.data.data_val =
            (uchar*)MALLOC( MAX_REQ );

        if ( lseek( C.read.fd,
                    C.read.pos, SEEK_SET ) >= 0 )
        {
            R.read.data.data_len =
                read( C.read.fd,
                      (char*)R.read.data.data_val,
                      C.read.len );
        }
        R.read.err = errno;
        printf(( "READ %d pos %d sz %d -> %d errno %d\n",
                 C.read.fd, C.read.pos, C.read.len,
                 R.read.data.data_len, R.read.err ));
        break;

    case WRITE:
        if ( lseek( C.write.fd,
                    C.write.pos, SEEK_SET ) >= 0 )
        {
            R.write.size =
                write( C.write.fd,
                       C.write.data.data_val,
                       C.write.data.data_len );
        }
        R.write.err = errno;
        printf(( "WRITE %d pos %d sz %d -> %d errno %d\n",
                 C.write.fd, C.write.pos, C.write.data.data_len,
                 R.write.size, R.write.err ));
        break;

    case TRUNCATE:
        R.err.retval =
            truncate( (char*)C.truncate.path,
                      C.truncate.length );
        R.err.err = errno;
        printf(( "TRUNCATE %s to %d -> errno %d\n",
                 (char*)C.truncate.path, C.truncate.length, R.err.err ));
        break;

    case UNLINK:
        R.err.retval =
            unlink( (char*)C.path_only.path );
        R.err.err = errno;
        printf(( "UNLINK %s -> %d errno %d\n",
                 (char*)C.path_only.path, R.err.retval, R.err.err ));
        break;

    case OPENDIR:
    {
        uchar * p = C.path_only.path;
        DIR * dir = opendir( (char*)p );
        if ( dir )
        {
            R.opendir.dirptr = (unsigned) new( p ) opendir_path( dir );
        }
        else
            R.opendir.dirptr = 0;
        
        R.opendir.err = errno;
        printf(( "OPENDIR %s -> %d errno %d\n",
                 (char*)C.path_only.path, R.opendir.dirptr, R.opendir.err ));
        break;
    }

    case CLOSEDIR:
    {
        opendir_path * op = (opendir_path *)C.dirptr_only.dirptr;
        R.err.retval = closedir( op->dir );
        R.err.err = errno;
        delete op;
        printf(( "CLOSEDIR %d -> %d errno %d\n",
                 C.dirptr_only.dirptr, R.err.retval, R.err.err ));
        break;
    }

    case READDIR:
    {
        opendir_path * op = (opendir_path *)C.dirptr_only.dirptr;

        struct stat sb;
        R.readdir.retval = readdirstat( op, &sb, &R.readdir.entry );
        R.readdir.err = errno;
        if ( R.readdir.retval <= 0 )
        {
            R.readdir.entry.name = (uchar*) STRDUP( "" );
            R.readdir.entry.fileid = -1;
            // 0 is not ok to return to caller
            R.readdir.retval = -1;
        }
        printf(( "READDIR %d -> %s %d ret %d\n",
                 C.dirptr_only.dirptr, R.readdir.entry.name,
                 R.readdir.entry.fileid, R.readdir.retval ));
        break;
    }

    case READDIR2:
    {
        opendir_path * op = (opendir_path *)C.dirptr2.dirptr;
        int pos = C.dirptr2.pos;
        int entries = 0;
        int bytes = MAX_REQ - (MAX_PATH*2);
        remino_readdir_entry dummy;
        remino_readdir2_entry * ent, * nent, ** nentp;
        struct stat sb;

        R.readdir2.list = 0;
        R.readdir2.eof = FALSE;

        // perform a whole series of readdirs, walking forward
        // to 'pos' in the directory; then do more readdirs,
        // filling up 'R' until its either out of space to
        // hold this entry or we hit the end of the directory.

        rewinddir( op->dir );
        while ( pos-- > 0 )
        {
            if ( readdirstat( op, &sb, &dummy ) <= 0 )
                goto readdir2_error_done;
            free( dummy.name );
        }

        bytes -= sizeof( remino_readdir2_reply );

        nentp = &R.readdir2.list;

        for ( ;; )
        {
            if ( bytes < 0 )
                break;

            int r = readdirstat( op, &sb, &dummy );
            if ( r < 0 )
                goto readdir2_error_done;

            if ( r == 0 )
            {
                R.readdir2.eof = TRUE;
                break;
            }

            bytes -= sizeof( remino_readdir2_entry );
            bytes -= strlen( (char*)dummy.name ) + 4;

            if ( bytes < 0 )
                break;

            ent = (remino_readdir2_entry *)
                MALLOC( sizeof( remino_readdir2_entry ));

            ent->dirent = dummy;
            stat_to_xdrstat( &ent->stat, &sb );
            ent->next = 0;

            entries++;
            *nentp = ent;
            nentp = &ent->next;
        }

        R.readdir2.err = errno;
        R.readdir2.retval = 0;
        goto readdir2_done;

    readdir2_error_done:
        R.readdir2.err = errno;
        R.readdir2.retval = -1;
        R.readdir2.eof = TRUE;
        for ( ent = R.readdir2.list; ent; ent = nent )
        {
            nent = ent->next;
            free( ent->dirent.name );
            free( ent );
        }
        R.readdir2.list = 0;

    readdir2_done:
        printf(( "READDIR2 %d pos %d -> entries %d ret %d\n", 
                 C.dirptr2.dirptr, C.dirptr2.pos,
                 entries, R.readdir2.retval ));
        break;
    }

    case REWINDDIR:
    {
        opendir_path * op = (opendir_path *)C.dirptr_only.dirptr;
        R.err.retval = 0;
        rewinddir( op->dir );
        R.err.err = errno;
        printf(( "REWINDDIR %d\n", C.dirptr_only.dirptr ));
        break;
    }

    case MKDIR:
        R.err.retval = 
            mkdir( (char*)C.path_mode.path,
                   C.path_mode.mode );
        R.err.err = errno;
        printf(( "MKDIR %s mode %d -> %d errno %d\n",
                 (char*)C.path_mode.path, C.path_mode.mode,
                 R.err.retval, R.err.err ));
        break;

    case RMDIR:
        R.err.retval =
            rmdir( (char*)C.path_only.path );
        R.err.err = errno;
        printf(( "RMDIR %s -> %d errno %d\n",
                 (char*)C.path_only.path, R.err.retval, R.err.err ));
        break;

    case LSTAT:
        if ( symlinks )
        {
            R.stat.retval = 
                lstat( (char*)C.path_only.path, &sb );
            if ( R.stat.retval == 0 && !dirsymlinks &&
                 ( sb.st_mode & S_IFMT ) == S_IFLNK )
            {
                struct stat sb2;
                int r = stat( (char*)C.path_only.path, &sb2 );
                if ( r == 0 )
                {
                    if (( sb2.st_mode & S_IFMT ) == S_IFDIR )
                        sb = sb2;
                    else
                        R.stat.retval = r;
                }
            }
        }
        else
            R.stat.retval = 
                stat( (char*)C.path_only.path, &sb );

        R.stat.err = errno;
        if ( R.stat.retval == 0 )
            stat_to_xdrstat( &R.stat, &sb );

        printf(( "LSTAT %s -> %d mode %o errno %d\n",
                 (char*)C.path_only.path,
                 R.stat.retval,
                 R.stat.mode,
                 R.stat.err ));
        break;

    case READLINK:
        R.readlink.path = (uchar*)MALLOC( 750 );
        R.readlink.retval = readlink( (char*)C.path_only.path,
                                      (char*)R.readlink.path, 750 );
        // NOTE: readlink(2) does NOT append a NUL to the string!
        //       but it does return the count of chars stored.
        if ( R.readlink.retval > 0 )
            R.readlink.path[R.readlink.retval] = 0;
        R.readlink.err = errno;
        printf(( "READLINK %s -> %d (%s) errno %d\n", 
                 (char*)C.path_only.path,
                 R.readlink.retval,
                 (R.readlink.retval>0) ? (char*)R.readlink.path : "(none)",
                 R.readlink.err ));

        break;

    case LINK_S:
    case LINK_H:
        if ( call.call == LINK_S )
            R.err.retval = symlink( (char*)C.rename.from,
                                    (char*)C.rename.to );
        else
            R.err.retval = link( (char*)C.rename.from,
                                 (char*)C.rename.to );
        R.err.err = errno;
        printf(( "%s %s to %s -> %d errno %d\n", 
                 (call.call==LINK_S) ? "SYMLINK" : "LINK",
                 (char*)C.rename.to,
                 (char*)C.rename.from,
                 R.err.retval,
                 R.readlink.err ));

        break;

    case RENAME:
        R.err.retval = 
            rename( (char*)C.rename.from,
                    (char*)C.rename.to );
        R.err.err = errno;
        printf(( "RENAME %s to %s -> %d errno %d\n",
                 (char*)C.rename.from, (char*)C.rename.to,
                 R.err.retval, R.err.err ));
        break;

    case CHOWN:
        R.err.retval =
            chown( (char*)C.chown.path,
                   C.chown.owner,
                   C.chown.group );
        R.err.err = errno;
        printf(( "CHOWN %s (%d,%d) -> %d errno %d\n",
                 (char*)C.chown.path,
                 C.chown.owner, C.chown.group,
                 R.err.retval, R.err.err ));
        break;

    case CHMOD:
        R.err.retval =
            chmod( (char*)C.path_mode.path,
                   C.path_mode.mode );
        R.err.err = errno;
        printf(( "CHMOD %s (%d) -> %d errno %d\n",
                 (char*)C.path_mode.path,
                 C.path_mode.mode,
                 R.err.retval, R.err.err ));
        break;

    case UTIMES:
        tv[0].tv_sec  = C.utimes.asecs;
        tv[0].tv_usec = C.utimes.ausecs;
        tv[1].tv_sec  = C.utimes.msecs;
        tv[1].tv_usec = C.utimes.musecs;
        R.err.retval =
            utimes( (char*)C.utimes.path, tv );
        R.err.err = errno;
        printf(( "UTIMES %s -> %d errno %d\n",
                 (char*)C.utimes.path, 
                 R.err.retval, R.err.err ));
        break;

    default:
        ret = NULL;
        break;
    }

#undef printf

    if ( ret != NULL )
    {
        xdrs.encode_decode = XDR_ENCODE;
        xdrs.data = replydata;
        xdrs.position = 0;
        xdrs.bytes_left = MAX_REQ;

        if ( myxdr_remino_reply( &xdrs, &reply ))
        {
            outlen = xdrs.position;
        }
        else
        {
            ret = NULL;
        }

        xdrs.encode_decode = XDR_FREE;
        myxdr_remino_reply( &xdrs, &reply );
    }

    xdrs.encode_decode = XDR_FREE;
    myxdr_remino_call( &xdrs, &call );

    return ret;
}

// remote_inode_server_tcp implementation

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

remote_inode_server_tcp :: remote_inode_server_tcp( u_int addr,
                                                    int port,
                                                    uchar * fsname,
                                                    bool _verbose,
                                                    bool _symlinks,
                                                    bool _dirsymlinks )
    : svr( _verbose, _symlinks, _dirsymlinks )
{
    if ( fsname && ( strlen( (char*)fsname ) > 32 ))
    {
        printf( "bogus fsname %s\n", fsname );
        return;
    }

    struct sockaddr_in sa;
    fd = socket( AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0 );
    if ( fd < 0 )
    {
        printf( "socket: %s\n", strerror( errno ));
        return;
    }

    int flag = 1;
#if defined(SOLARIS)
    setsockopt( fd, SOL_SOCKET, SO_REUSEADDR, (char*)&flag, sizeof( flag ));
#else
    setsockopt( fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof( flag ));
#endif
    sa.sin_family = AF_INET;
    sa.sin_port = htons( port );

    if ( addr == 0 )
    {
        int ear;
        sa.sin_addr.s_addr = INADDR_ANY;

        if ( bind( fd, (struct sockaddr *)&sa, sizeof( sa )) < 0 )
        {
            int e = errno;
            close( fd );
            fd = -1;
            printf( "bind : %s\n", strerror( e ));
            return;
        }

        listen( fd, 1 );
#if defined(CYGWIN)
        int salen = sizeof( struct sockaddr_in );
#elif defined(SOLARIS)
        socklen_t salen = sizeof( struct sockaddr_in );
#else
        u_int salen = sizeof( struct sockaddr_in );
#endif

        ear = accept4( fd, (struct sockaddr *)&sa, &salen, SOCK_CLOEXEC );
        if ( ear < 0 )
        {
            int e = errno;
            close( fd );
            fd = -1;
            printf( "accept : %s\n", strerror( e ));
            return;
        }

        close( fd );
        fd = ear;
    }
    else
    {
        sa.sin_addr.s_addr = htonl( addr );
        if ( connect( fd, (struct sockaddr *)&sa, sizeof( sa )) < 0 )
        {
            int e = errno;
            close( fd );
            fd = -1;
            printf( "connect : %s\n", strerror( e ));
            return;
        }
    }

    if ( fsname )
    {
        uchar newfsname[ 64 ];
        char hostname[ 64 ];
        char * hp;

        memset( newfsname, 0, 64 );
        gethostname( hostname, 64 );
        for ( hp = hostname; *hp; hp++ )
            if ( *hp == '.' )
            {
                *hp = 0;
                break;
            }
#ifndef CYGWIN
        sprintf( (char*)newfsname, "%s_%s_%d",
                 fsname, hostname, getpid() );
#else
        sprintf( (char*)newfsname, "%s_%d",
                 fsname, getpid() );
#endif
        write( fd, newfsname, 64 );
    }
}

remote_inode_server_tcp :: ~remote_inode_server_tcp( void )
{
    close( fd );
}

#include <time.h>

void
remote_inode_server_tcp :: dispatch_loop( bool checkparent )
{
    uchar rcvbuf[ 9000 ];
    time_t now, last;
    pid_t parent;

    time( &now );
    parent = getppid();

    last = now;
    while ( 1 )
    {
        u_int l, l2;
        uchar * p;
        int cc;

        time( &now );
        if ( checkparent && now != last )
        {
            last = now;
            if ( kill( parent, 0 ) < 0 )
            {
                /* an ironic printf, since if the 'parent' dies,
                   the shell probably exited.  but maybe its a subshell,
                   or maybe itsfsriw's output is redirected to a file.. */
                printf( "parent process has died, bailing out\n" );
                return;
            }
        }

        cc = read( fd, &l, sizeof( l ));
        if ( cc != sizeof( l ))
        {
            printf( "tcp 1 read returns %d: %s\n", cc, strerror( errno ));
            return;
        }
        l = ntohl( l );
        if ( l > 9000 )
        {
            printf( "tcp 2 bogus length %d received\n", l );
            return;
        }

        l2 = l;
        p = rcvbuf;
        while ( l > 0 )
        {
            cc = read( fd, p, l );
            if ( cc <= 0 )
            {
                printf( "tcp 3 read returns %d: %s\n", cc, strerror( errno ));
                return;
            }
            p += cc;
            l -= cc;
        }
        p = svr.dispatch( rcvbuf, (int)l2, (int&)l );
        if ( p )
        {
            uchar buf [ MAX_REQ + 4 ];

            *(int*)(buf) = htonl( l );
            memcpy( buf + 4, p, l );

            cc = write( fd, buf, l + 4 );
            if ( cc != (int)( l + 4 ))
            {
                printf( "tcp 4 write returns %d: %s\n", cc, strerror( errno ));
                return;
            }
        }
    }
}
