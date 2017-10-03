/*
 * This code is originally written by Phillip F Knaack.
 * There are absolutely no restrictions on the use of this code.  It is
 * NOT GPL!  If you use this code in a product, you do NOT have to release
 * your alterations, nor do you have to acknowledge in any way that you
 * are using this software.
 * The only thing I ask is this: do not claim you wrote it.  My name
 * should appear in this file forever as the original author, and every
 * person who modifies this file after me should also place their names
 * in this file (so those that follow know who broke what).
 * This restriction is not enforced in any way by any license terms, it
 * is merely a personal request from me to you.  If you wanted, you could
 * even completely remove this entire comment and place a new one with your
 * company's confidential proprietary license on it-- but if you are a good
 * internet citizen, you will comply with my request out of the goodness
 * of your heart.
 * If you do use this code and you make a buttload of money off of it,
 * I would appreciate a little kickback-- but again this is a personal
 * request and is not required by any licensing of this code.  (Of course
 * in the offchance that anyone every actually DOES make a lot of money 
 * using this code, I know I'm going to regret that statement, but at
 * this point in time it feels like the right thing to say.)
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
