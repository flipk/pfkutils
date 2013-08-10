
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

#include "remote_ino.H"
#include "remote_ino_prot.h"
#include <string.h>
#include <errno.h>
#include <sys/uio.h>

#include "config.h"

remote_inode_client :: remote_inode_client( 
    int (*send)( int arg, uchar *, int ),
    int (*get)( int arg, uchar *, int * ), 
    int _arg )
{
    send_request = send;
    get_reply = get;
    arg = _arg;
}

#define Z(n)   memset( &call, 0, sizeof( call )); \
               memset( &reply, 0, sizeof( reply )); \
               call.call = reply.reply = n;

#define C   call.remino_call_u
#define R   reply.remino_reply_u

#define EN(errcode) \
    xdrs.encode_decode = XDR_ENCODE; \
    xdrs.data = reqdata; \
    xdrs.position = 0; \
    xdrs.bytes_left = MAX_REQ; \
    if ( !myxdr_remino_call( &xdrs, &call )) \
    { errcode }

#define DEC(errcode) \
    xdrs.encode_decode = XDR_DECODE; \
    xdrs.data = reqdata; \
    xdrs.position = 0; \
    xdrs.bytes_left = replylen; \
    if ( !myxdr_remino_reply( &xdrs, &reply )) \
    { errcode } \
    if ( call.call != reply.reply ) \
    { errcode }

#define SR( errcode ) \
    if ( send_request( arg, reqdata, xdrs.position ) < 0 ) \
    { errcode } \
    int replylen = MAX_REQ; \
    if ( get_reply( arg, reqdata, &replylen ) < 0 ) \
    { errcode }

#define FR \
    xdrs.encode_decode = XDR_FREE; \
    myxdr_remino_reply( &xdrs, &reply );

int
remote_inode_client ::  open( uchar * path, int flags, int mode )
{
    Z( OPEN );
    C.open.path = path;
    C.open.flags = 0;
    if ( flags & O_CREAT )
        C.open.flags = CREATE_FLAG;
    if ( flags & O_WRONLY )
        C.open.flags |= WRITE_FLAG;
    if ( flags & O_RDWR )
        C.open.flags |= READ_FLAG | WRITE_FLAG;

    C.open.mode = mode;
    EN( errno = EINVAL ; return -1; );
    SR( errno = EINVAL; return -1; );
    DEC( errno = EINVAL ; return -1; );
    FR;
    errno = R.open.err;
    int ret = R.open.fd;
    return ret;
}

int
remote_inode_client ::  close( int fd )
{
    Z( CLOSE );
    C.close.fd = fd;
    EN( errno = EINVAL ; return -1; );
    SR( errno = EINVAL ; return -1; );
    DEC( errno = EINVAL ; return -1; );
    FR;
    errno = R.err.err;
    return R.err.retval;
}

int
remote_inode_client ::  read( int fd, int pos, uchar * buf, int sz )
{
    Z( READ );
    C.read.fd = fd;
    C.read.pos = pos;
    C.read.len = sz;
    EN( errno = EINVAL ; return -1; );
    SR( errno = EINVAL ; return -1; );
    DEC( FR ; errno = EINVAL ; return -1; );
    if ( R.read.data.data_len > 0 )
        memcpy( buf, R.read.data.data_val, R.read.data.data_len );
    FR;
    errno = R.read.err;
    int ret = R.read.data.data_len;
    return ret;
}

int
remote_inode_client ::  write( int fd, int pos, uchar * buf, int sz )
{
    Z( WRITE );
    C.write.fd = fd;
    C.write.pos = pos;
    C.write.data.data_val = buf;
    C.write.data.data_len = sz;
    EN( errno = EINVAL ; return -1; );
    SR( errno = EINVAL ; return -1; );
    DEC( errno = EINVAL ; return -1; );
    FR;
    errno = R.write.err;
    int ret = R.write.size;
    return ret;
}

int
remote_inode_client ::  truncate( uchar * path, int length )
{
    Z( TRUNCATE );
    C.truncate.path = path;
    C.truncate.length = length;
    EN( errno = EINVAL ; return -1; );
    SR( errno = EINVAL ; return -1; );
    DEC( errno = EINVAL ; return -1; );
    FR;
    errno = R.err.err;
    return R.err.retval;
}

int
remote_inode_client ::  unlink( uchar * path )
{
    Z( UNLINK );
    C.path_only.path = path;
    EN( errno = EINVAL ; return -1; );
    SR( errno = EINVAL ; return -1; );
    DEC( errno = EINVAL ; return -1; );
    FR;
    errno = R.err.err;
    return R.err.retval;
}

DIR *
remote_inode_client ::  opendir( uchar * path )
{
    Z( OPENDIR );
    C.path_only.path = path;
    EN( errno = EINVAL ; return NULL; );
    SR( errno = EINVAL ; return NULL; );
    DEC( errno = EINVAL ; return NULL; );
    errno = R.opendir.err;
    DIR * ret = (DIR*) R.opendir.dirptr;
    return ret;
}

int
remote_inode_client :: closedir( DIR * d )
{
    Z( CLOSEDIR );
    C.dirptr_only.dirptr = (unsigned) d;
    EN( errno = EINVAL ; return -1 ; );
    SR( errno = EINVAL ; return -1 ; );
    DEC( errno = EINVAL ; return -1 ; );
    errno = R.err.err;
    return R.err.retval;
}

int
remote_inode_client :: readdir( uchar * name, int &fileid,
                                inode_file_type &ftype, DIR * d )
{
    Z( READDIR );
    C.dirptr_only.dirptr = (unsigned) d;
    EN( errno = EINVAL ; return -1 ; );
    SR( errno = EINVAL ; return -1 ; );
    DEC( errno = EINVAL ; return -1 ; );
    strcpy( (char*)name, (char*)R.readdir.entry.name );
    fileid = R.readdir.entry.fileid;
    ftype = (inode_file_type) R.readdir.entry.ftype;
    FR;
    errno = R.readdir.err;
    return R.readdir.retval;
}

// pos initialized to zero by caller;
// list initialized to point to the head pointer of the caller's list;
// pos is moved forward by this call for each entry;
// list is moved forward to the empty 'next' pointer of the last entry;
// pos is returned as -1 if the eof of the directory is reached.

int
remote_inode_client :: readdir2( remino_readdir2_entry *** list,
                                 int * pos, DIR * d )
{
    Z( READDIR2 );
    C.dirptr2.dirptr = (unsigned) d;
    C.dirptr2.pos = *pos;

    EN( errno = EINVAL ; return -1 ; );
    SR( errno = EINVAL ; return -1 ; );
    DEC( errno = EINVAL ; return -1 ; );

    // NOTE that we don't call FR unless there was an error!
    //      we get a long linked-list back, but we actually
    //      pass that along to the caller,
    //      so they'll free it when they're done.

    if ( R.readdir2.retval == 0 )
    {
        **list = R.readdir2.list;

        // nuke pointer from R so that it can't be mistaken 
        // for anything important.

        R.readdir2.list = 0;

        if ( R.readdir2.eof == FALSE )
        {
            remino_readdir2_entry * ent;

            // must walk *list forward to point to the &next of the
            // last entry returned; must walk pos forward too.

            for ( ent = **list; ent; ent = ent->next )
            {
                *list = &ent->next;
                (*pos)++;
            }
        }
        else
            *pos = -1;
    }
    else
    {
        FR;
    }

    errno = R.readdir2.err;
    return R.readdir2.retval;
}

int
remote_inode_client ::  rewinddir( DIR * d )
{
    Z( REWINDDIR );
    C.dirptr_only.dirptr = (unsigned) d;
    EN( errno = EINVAL ; return -1 ; );
    SR( errno = EINVAL ; return -1 ; );
    DEC( errno = EINVAL ; return -1 ; );
    errno = R.err.err;
    return R.err.retval;
}

int
remote_inode_client ::  mkdir( uchar * path, int mode )
{
    Z( MKDIR );
    C.path_mode.path = path;
    C.path_mode.mode = mode;
    EN( errno = EINVAL ; return -1 ; );
    SR( errno = EINVAL ; return -1 ; );
    DEC( errno = EINVAL ; return -1 ; );
    errno = R.err.err;
    return R.err.retval;
}

int
remote_inode_client ::  rmdir( uchar * path )
{
    Z( RMDIR );
    C.path_only.path = path;
    EN( errno = EINVAL ; return -1 ; );
    SR( errno = EINVAL ; return -1 ; );
    DEC( errno = EINVAL ; return -1 ; );
    errno = R.err.err;
    return R.err.retval;
}

int
remote_inode_client ::  lstat( uchar * path, struct stat * sb )
{
    Z( LSTAT );
    return _stat( path, sb );
}

#include "../sudo.h"

void
remote_inode_client :: xdrstat_to_stat( remino_stat_reply * rst,
                                        struct stat * sb )
{
    sb->st_mode               = rst->mode;
    sb->st_nlink              = rst->nlink;
    sb->st_uid                = MY_UID;
    sb->st_gid                = MY_GID;
    sb->st_size               = rst->size;
    sb->st_blksize            = rst->blocksize;
    sb->st_rdev               = rst->rdev;
    sb->st_blocks             = rst->blocks;
    sb->st_dev                = rst->fsid;
#if defined(CYGWIN) || defined(SOLARIS)
    sb->st_atime              = rst->asec;
    sb->st_mtime              = rst->msec;
    sb->st_ctime              = rst->csec;
#else
    sb->st_atimespec.tv_sec   = rst->asec;
    sb->st_atimespec.tv_nsec  = rst->ausec * 1000;
    sb->st_mtimespec.tv_sec   = rst->msec;
    sb->st_mtimespec.tv_nsec  = rst->musec * 1000;
    sb->st_ctimespec.tv_sec   = rst->csec;
    sb->st_ctimespec.tv_nsec  = rst->cusec * 1000;
#endif
    sb->st_ino                = rst->fileid;
}

int
remote_inode_client :: _stat( uchar * path, struct stat * sb )
{
    C.path_only.path = path;
    EN( errno = EINVAL ; return -1 ; );
    SR( errno = EINVAL ; return -1 ; );
    DEC( errno = EINVAL ; return -1 ; );
    xdrstat_to_stat( &R.stat, sb );
    errno = R.stat.err;
    return R.stat.retval;
}

int
remote_inode_client :: readlink( uchar * path, uchar * buf )
{
    Z( READLINK );
    C.path_only.path = path;
    EN( errno = EINVAL ; return -1 ; );
    SR( errno = EINVAL ; return -1 ; );
    DEC( errno = EINVAL ; return -1 ; );
    strcpy( (char*)buf, (char*)R.readlink.path );
    FR;
    errno = R.readlink.err;
    return R.readlink.retval;
}

int
remote_inode_client :: symlink( uchar * target, uchar * path )
{
    Z( LINK_S );
    C.rename.from = target;
    C.rename.to   = path;
    EN( errno = EINVAL ; return -1 ; );
    SR( errno = EINVAL ; return -1 ; );
    DEC( errno = EINVAL ; return -1 ; );
    errno = R.err.err;
    return R.err.retval;
}

int
remote_inode_client :: link( uchar * target, uchar * path )
{
    Z( LINK_H );
    C.rename.from = target;
    C.rename.to   = path;
    EN( errno = EINVAL ; return -1 ; );
    SR( errno = EINVAL ; return -1 ; );
    DEC( errno = EINVAL ; return -1 ; );
    errno = R.err.err;
    return R.err.retval;
}

int
remote_inode_client ::  rename( uchar * from, uchar * to )
{
    Z( RENAME );
    C.rename.from = from;
    C.rename.to   = to;
    EN( errno = EINVAL ; return -1 ; );
    SR( errno = EINVAL ; return -1 ; );
    DEC( errno = EINVAL ; return -1 ; );
    errno = R.err.err;
    return R.err.retval;
}

int
remote_inode_client ::  chown( uchar * path, int owner, int group )
{
    Z( CHOWN );
    C.chown.path = path;
    C.chown.owner = owner;
    C.chown.group = group;
    EN( errno = EINVAL ; return -1 ; );
    SR( errno = EINVAL ; return -1 ; );
    DEC( errno = EINVAL ; return -1 ; );
    errno = R.err.err;
    return R.err.retval;
}

int
remote_inode_client ::  chmod( uchar * path, int mode )
{
    Z( CHMOD );
    C.path_mode.path = path;
    C.path_mode.mode = mode;
    EN( errno = EINVAL ; return -1 ; );
    SR( errno = EINVAL ; return -1 ; );
    DEC( errno = EINVAL ; return -1 ; );
    errno = R.err.err;
    return R.err.retval;
}

int
remote_inode_client ::  utimes( uchar * path, struct timeval * tv )
{
    Z( UTIMES );
    C.utimes.path   = path;
    C.utimes.asecs  = tv[0].tv_sec;
    C.utimes.ausecs = tv[0].tv_usec;
    C.utimes.msecs  = tv[1].tv_sec;
    C.utimes.musecs = tv[1].tv_usec;
    EN( errno = EINVAL ; return -1 ; );
    SR( errno = EINVAL ; return -1 ; );
    DEC( errno = EINVAL ; return -1 ; );
    errno = R.err.err;
    return R.err.retval;
}

// remote_inode_client_tcp implementation!

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

remote_inode_client_tcp :: remote_inode_client_tcp( int _fd )
    : clnt( send, get, (int)this )
{
    fd = _fd;
    bad_fd = false;
}

remote_inode_client_tcp :: remote_inode_client_tcp( u_int addr, int port )
    : clnt( send, get, (int)this )
{
    int flag;
    struct sockaddr_in sa;
    fd = socket( AF_INET, SOCK_STREAM, 0 );
    if ( fd < 0 )
        return;

    bad_fd = false;

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
            close( fd );
            fd = -1;
            return;
        }

        listen( fd, 1 );
#if defined(CYGWIN) || defined(SOLARIS)
        int salen = sizeof( struct sockaddr_in );
#else
        u_int salen = sizeof( struct sockaddr_in );
#endif

        ear = accept( fd, (struct sockaddr *)&sa, &salen );
        if ( ear < 0 )
        {
            close( fd );
            fd = -1;
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
            close( fd );
            fd = -1;
            return;
        }
    }
}

remote_inode_client_tcp :: ~remote_inode_client_tcp( void )
{
    if ( fd != -1 )
        close( fd );
}

// static
int
remote_inode_client_tcp :: send( int arg, uchar * p, int len )
{
    remote_inode_client_tcp * ict;
    int cc;

    ict = (remote_inode_client_tcp *)arg;

    uchar buf [ MAX_REQ + 4 ];

    *(int*)(buf) = htonl( len );
    memcpy( buf+4, p, len );

    cc = write( ict->fd, (char*)buf, len+4 );
    if ( cc != (len + 4 ))
    {
        printf( "tcp 2 write returns %d errno %d\n", cc, errno );
        ict->bad_fd = true;
        return -1;
    }
    
    return 0;
}

// static
int
remote_inode_client_tcp :: get( int arg, uchar * p, int * len )
{
    remote_inode_client_tcp * ict;
    u_int l;
    int cc;

    ict = (remote_inode_client_tcp *)arg;
    cc = read( ict->fd, &l, sizeof( l ));
    if ( cc != sizeof( l ))
    {
        printf( "tcp 3 read returns %d errno %d\n", cc, errno );
        ict->bad_fd = true;
        return -1;
    }
    l = ntohl( l );

    if ( l > (u_int)*len )
        return -1;

    *len = l;

    while ( l > 0 )
    {
        cc = read( ict->fd, p, l );
        if ( cc <= 0 )
        {
            printf( "tcp 5 read returns %d errno %d\n", cc, errno );
            ict->bad_fd = true;
            return -1;
        }
        p += cc;
        l -= cc;
    }

    return 0;
}
