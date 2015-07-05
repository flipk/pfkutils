
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

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

#include "xdr.h"
#include "mytypes.h"
#include "ino_types.h"
#include "remote_ino_prot.h"

#undef rewinddir

class remote_inode_client {
    uchar reqdata[ MAX_REQ ];
    remino_call call;
    remino_reply reply;
    XDR xdrs;

    int (*send_request)( int arg, uchar * p, int len );
    int (*get_reply)( int arg, uchar * p, int * len );
    int arg;

    int   _stat     ( uchar * path, struct stat * );

public:
    remote_inode_client( int (*send)( int arg, uchar *, int ),
                         int (*get)( int arg, uchar *, int * ),
                         int arg );

    static const int CREATE_FLAG = 1;
    static const int READ_FLAG   = 2;
    static const int WRITE_FLAG  = 4;

    void  xdrstat_to_stat( remino_stat_reply * rst, struct stat * sb );

    int   open      ( uchar * path, int flags, int mode );
    int   close     ( int fd );
    int   read      ( int fd, int pos, uchar * buf, int sz );
    int   write     ( int fd, int pos, uchar * buf, int sz );

    int   truncate  ( uchar * path, int length );
    int   unlink    ( uchar * path );

    DIR * opendir   ( uchar * path );
    int   closedir  ( DIR * );
    int   readdir   ( uchar * name, int &fileid,
                      inode_file_type &ftype, DIR * );
    int   readdir2  ( remino_readdir2_entry *** list, int * pos, DIR * );
    int   rewinddir ( DIR * );

    int   mkdir     ( uchar * path, int mode );
    int   rmdir     ( uchar * path );
    int   lstat     ( uchar * path, struct stat * );

    int   rename    ( uchar * from, uchar * to );
    int   chown     ( uchar * path, int owner, int group );
    int   chmod     ( uchar * path, int mode );

    int   utimes    ( uchar * path, struct timeval * );
    int   readlink  ( uchar * path, uchar * buf );
    int   symlink   ( uchar * target, uchar * path );
    int   link      ( uchar * target, uchar * path );
};

class opendir_path;

class remote_inode_server {
    uchar replydata[ MAX_REQ ];
    bool verbose;
    bool symlinks;
    bool dirsymlinks;
    void stat_to_xdrstat( remino_stat_reply * rst, struct stat * sb );
    int readdirstat( opendir_path *, struct stat *, remino_readdir_entry * );
public:
    remote_inode_server( bool _verbose, bool _symlinks, bool _dirsymlinks )
        {
            verbose = _verbose;
            symlinks = _symlinks;
            dirsymlinks = _dirsymlinks;
        }
    uchar * dispatch( uchar * packet, int length, int &reply_length );
};

class remote_inode_client_tcp {
    int fd;
    static int send( int arg, uchar * p, int len );
    static int get( int arg, uchar * p, int * len );
public:
    remote_inode_client_tcp( u_int addr, int port );
    remote_inode_client_tcp( int fd );
    ~remote_inode_client_tcp( void );
    remote_inode_client clnt;
    bool bad_fd;
};

class remote_inode_server_tcp {
    int fd;
    remote_inode_server svr;
public:
    remote_inode_server_tcp( u_int addr, int port, uchar * fsname,
                             bool _verbose, bool _symlinks,
                             bool _dirsymlinks );
    ~remote_inode_server_tcp( void );
    void dispatch_loop( bool check_parent );
};
