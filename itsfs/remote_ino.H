/*
 * This code is written by Phillip F Knaack. This code is in the
 * public domain. Do anything you want with this code -- compile it,
 * run it, print it out, pass it around, shit on it -- anything you want,
 * except, don't claim you wrote it.  If you modify it, add your name to
 * this comment in the COPYRIGHT file and to this comment in every file
 * you change.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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

class remote_inode_server {
    uchar replydata[ MAX_REQ ];
    bool verbose;
    bool symlinks;
    bool dirsymlinks;
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
    void dispatch_loop( void );
};
