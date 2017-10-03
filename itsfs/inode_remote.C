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

#include "inode_remote.H"
#include "lognew.H"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>

Inode_remote :: Inode_remote( Inode_remote_tree * _it, uchar * _path,
                              int _fileid, inode_file_type _ft )
    : Inode( INODE_REMOTE, _fileid, _it->tree_id, _ft )
{
    path = (uchar*)STRDUP( (char*)_path );
    dir = NULL;
    fd = -1;
    it = _it;
    refresh_directory = true;
    dirlist = 0;
    last_stat = 0;
}

Inode_remote :: ~Inode_remote( void )
{
    free( path );
    if ( ftype == INODE_DIR )
    {
        if ( dir )
            it->rict.clnt.closedir( dir );
    }
    else
    {
        if ( fd != -1 )
            it->rict.clnt.close( fd );
    }
    free_dirlist();
}

int
Inode_remote :: isattr( sattr & s )
{
    int now = time( NULL );
    if (( now - last_stat ) > 2 )
    {
        if ( it->rict.clnt.lstat( path, &sb ) < 0 )
            return -1;
        last_stat = now;
    }

    if ( s.mode != (unsigned)-1 )
    {
        s.mode |= 0200;
        if ( it->rict.clnt.chmod( path, s.mode & 0777 ) < 0 )
            return -1;
    }

    bool own_modify = false;
    uid_t uid;
    gid_t gid;

    if ( s.uid != (unsigned)-1 )
    {
        own_modify = true;
        uid = s.uid;
    }
    else
        uid = sb.st_uid;

    if ( s.gid != (unsigned)-1 )
    {
        own_modify = true;
        gid = s.gid;
    }
    else
        gid = sb.st_gid;

    if ( own_modify )
    {
        if ( it->rict.clnt.chown( path, uid, gid ) < 0 )
            return -1;
    }

    if ( s.size != (unsigned)-1 )
    {
        if ( it->rict.clnt.truncate( path, s.size ) < 0 )
            return -1;
    }

    if ( s.atime.seconds != (unsigned)-1 ||
         s.mtime.seconds != (unsigned)-1 )
    {
        struct timeval times[2] = {
            { sb.st_atimespec.tv_sec,
              sb.st_atimespec.tv_nsec / 1000 },
            { sb.st_mtimespec.tv_sec,
              sb.st_mtimespec.tv_nsec / 1000 }
        };
        if ( s.atime.seconds != (unsigned)-1 )
        {
            times[0].tv_sec  = s.atime.seconds;
            times[0].tv_usec = s.atime.useconds;
        }
        if ( s.mtime.seconds != (unsigned)-1 )
        {
            times[1].tv_sec  = s.mtime.seconds;
            times[1].tv_usec = s.mtime.useconds;
        }
        if ( it->rict.clnt.utimes( path, times ) < 0 )
            return -1;
    }

    errno = 0;
    return 0;
}

int
Inode_remote :: ifattr( fattr & f )
{
    inode_file_type dummy;
    int now = time( NULL );

    if (( now - last_stat ) > 2 )
    {
        if ( it->rict.clnt.lstat( path, &sb ) < 0 )
            return -1;
        last_stat = now;
    }

    if (( sb.st_mode & S_IFMT ) == S_IFDIR )
    {
        f.type = NFDIR;
        f.mode = NFSMODE_DIR;
    }
    else if (( sb.st_mode & S_IFMT ) == S_IFLNK )
    {
        f.type = NFLNK;
        f.mode = NFSMODE_LNK;
    }
    else
    {
        f.type = NFREG;
        f.mode = NFSMODE_REG;
    }

    f.mode          |= sb.st_mode & 0777;
    f.nlink          = sb.st_nlink;
    f.uid            = sb.st_uid;
    f.gid            = sb.st_gid;
    f.size           = sb.st_size;
    f.blocksize      = sb.st_blksize;
    f.rdev           = 1;
    f.blocks         = sb.st_blocks;
    f.fsid           = it->mount_id;
    f.atime.seconds  = sb.st_atimespec.tv_sec;
    f.atime.useconds = sb.st_atimespec.tv_nsec / 1000;
    f.mtime.seconds  = sb.st_mtimespec.tv_sec;
    f.mtime.useconds = sb.st_mtimespec.tv_nsec / 1000;
    f.ctime.seconds  = sb.st_ctimespec.tv_sec;
    f.ctime.useconds = sb.st_ctimespec.tv_nsec / 1000;
    f.fileid         = inode_name_db->fetch( it->mount_id, path, dummy );

    errno = 0;
    return 0;
}

int
Inode_remote :: iread( int offset, uchar * buf, int &size )
{
    if ( ftype == INODE_DIR )
    {
        errno = EISDIR;
        return -1;
    }

    if ( fd == -1 )
    {
        fd = it->rict.clnt.open( path, O_RDWR, 0644 );
        if ( fd < 0 )
            fd = it->rict.clnt.open( path, O_RDONLY, 0644 );
        if ( fd < 0 )
            return -1;
    }

    int retsize = it->rict.clnt.read( fd, offset, buf, size );
    if ( retsize < 0 )
        return -1;

    size = retsize;
    errno = 0;
    return 0;
}

int
Inode_remote :: iwrite( int offset, uchar * buf, int size )
{
    if ( ftype == INODE_DIR )
    {
        errno = EISDIR;
        return -1;
    }

    if ( fd == -1 )
    {
        fd = it->rict.clnt.open( path, O_RDWR, 0644 );
        if ( fd < 0 )
            return -1;
    }

    int retsize = it->rict.clnt.write( fd, offset, buf, size );
    if ( retsize < 0 || retsize != size )
        return -1;

    errno = 0;
    return 0;
}

int
Inode_remote :: setdirpos( int cookie )
{
    if ( ftype != INODE_DIR )
    {
        errno = ENOTDIR;
        return -1;
    }

    if ( cookie == 0 )
    {
        refresh_directory = true;
        errno = 0;
        return 0;
    }

    curdir = dirlist;
    while ( cookie-- > 0 )
        if ( curdir )
            curdir = curdir->next;

    errno = 0;
    return 0;
}

void
Inode_remote :: free_dirlist( void )
{
    remino_readdir2_entry * e, * ne;

    // free dirlist;

    for ( e = dirlist; e; e = ne )
    {
        ne = e->next;
        free( e->dirent.name );
        free( e );
    }
    dirlist = 0;
}

int
Inode_remote :: readdir( int &fileno, uchar * filename )
{
    inode_file_type read_nftype;

    if ( dir == NULL )
    {
        dir = it->rict.clnt.opendir( path );
        if ( dir == NULL )
            return -1;
        refresh_directory = true;
    }

    if ( refresh_directory )
    {
        free_dirlist();

        int pos = 0;
        remino_readdir2_entry ** listptr = &dirlist;

        do {
            // repeat the following line until pos is -1 after this call

            if ( it->rict.clnt.readdir2( &listptr, &pos, dir ) < 0 )
            {
                int e = errno;
                free_dirlist();
                errno = e;
                return -1;
            }

        } while ( pos != -1 );

        curdir = dirlist;
        refresh_directory = false;
    }

    if ( !curdir )
        return -1;

    strcpy( (char*)filename, (char*)curdir->dirent.name );
    fileno = curdir->dirent.fileid;
    read_nftype = (inode_file_type)curdir->dirent.ftype;

    curdir = curdir->next;

    uchar fullpath[ Inode_tree::MAX_PATH_LENGTH ];

    if ( strcmp( (char*)filename, "." ) == 0 )
    {
        fileno = file_id;
        errno = 0;
        return 0;
    }
    else if ( strcmp( (char*)filename, ".." ) == 0 )
    {
        uchar * p;
        strcpy( (char*)fullpath, (char*)path );
        if ( fullpath[0] != '.' )
        {
            p = &fullpath[ strlen( (char*)fullpath ) ];
            while ( *p != '/' && p != fullpath )
                p--;
            *p = 0;
        }
    }
    else
    {
        sprintf( (char*)fullpath, "%s/%s", path, filename );
    }

    inode_file_type dummy;
    fileno = inode_name_db->fetch( it->mount_id, fullpath, dummy );

    if ( fileno == -1 )
    {
        fileno = inode_name_db->alloc_id();
        inode_name_db->add( it->mount_id, fullpath, read_nftype, fileno );
    }

    errno = 0;
    return 0;
}

int
Inode_remote :: readlink( uchar * p )
{
    if ( it->rict.clnt.readlink( path, p ) < 0 )
        return -1;
    return 0;
}
