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

#include "inode_remote.H"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

Inode_remote_tree :: Inode_remote_tree( int tree_id,
                                        uchar * _path,
                                        u_int remote_addr,
                                        int remote_port )
    : Inode_tree( INODE_REMOTE, tree_id ),
      rict( remote_addr, remote_port ),
      hashlru( HASH_SIZE )
{
    init_common( _path );
}

Inode_remote_tree :: Inode_remote_tree( int tree_id,
                                        uchar * _path,
                                        int fd )
    : Inode_tree( INODE_REMOTE, tree_id ),
      rict( fd ),
      hashlru( HASH_SIZE )
{
    init_common( _path );
}

void
Inode_remote_tree :: init_common( uchar * _path )
{
    uchar * p;
    bool dummy;

    bad_fd = false;

    rootpath = (uchar*) strdup( (char*)_path );
    zerorefs = 0;

    inode_name_db->add( mount_id, rootpath, INODE_DIR, ROOT_INODE_ID );
}

Inode_remote_tree :: ~Inode_remote_tree( void )
{
    Inode_remote *ino, *nino;
    for ( ino = hashlru.get_lru_head(); ino; ino = nino )
    {
        nino = hashlru.get_lru_next( ino );
        if ( ino->refget() > 0 )
            printf( "inode for %s has nonzero refcount\n", ino->path );
        hashlru.remove( ino );
        delete ino;
    }
    free( rootpath );
}

Inode *
Inode_remote_tree :: get( int file_id )
{
    if ( rict.bad_fd )
        bad_fd = true;

    Inode_remote * ino;

    ino = hashlru.find( file_id );
    if ( ino )
    {
        if ( ino->ref() == 1 )
            zerorefs--;
        errno = 0;
        return ino;
    }

    inode_file_type nftype;
    uchar * p = inode_name_db->fetch( file_id, nftype );

    if ( p == NULL )
    {
        errno = ENOENT;
        return NULL;
    }

    struct stat sb;
    if ( rict.clnt.lstat( p, &sb ) == 0 )
    {
        bool changedb = false;
        if ((( sb.st_mode & S_IFMT) == S_IFLNK ) &&
            ( nftype != INODE_LINK ))
        {
            changedb = true;
            nftype = INODE_LINK;
        }
        else if ((( sb.st_mode & S_IFMT) == S_IFDIR ) &&
                 ( nftype != INODE_DIR ))
        {
            changedb = true;
            nftype = INODE_DIR;
        }
        else if (!(( sb.st_mode & S_IFMT) == S_IFREG ) &&
                 ( nftype == INODE_FILE ))
        {
            changedb = true;
            nftype = INODE_FILE;
        }
        if ( changedb )
        {
            inode_name_db->del( file_id );
            inode_name_db->add( mount_id, p, nftype, file_id );
        }
        ino = new Inode_remote( this, p, file_id, nftype );
        ino->sb = sb;
        ino->last_stat = time( NULL );
        hashlru.add( ino );
        errno = 0;
        return ino;
    }
    // else

    inode_name_db->del( file_id );
    errno = ENOENT;
    return NULL;
}

Inode *
Inode_remote_tree :: get_parent( int file_id )
{
    if ( rict.bad_fd )
        bad_fd = true;

    inode_file_type nftype;
    uchar * p = inode_name_db->fetch( file_id, nftype );
    if ( p == NULL )
    {
        errno = ENOENT;
        return NULL;
    }

    uchar np[ MAX_PATH_LENGTH ];
    strcpy( (char*)np, (char*)p );
    uchar * pp = &np[ strlen( (char*)np ) ];
    while ( *pp != '/' && pp != np )
        pp--;
    *pp = 0;

    int parentfileid = inode_name_db->fetch( mount_id, np, nftype );
    if ( parentfileid == -1 )
    {
        errno = ENOENT;
        return NULL;
    }

    return get( parentfileid );
}

Inode *
Inode_remote_tree :: get( int dir_id, uchar * filename )
{
    if ( rict.bad_fd )
        bad_fd = true;

    uchar * dirpath;
    inode_file_type dirnftype;
    Inode_remote * ret = NULL;

    dirpath = inode_name_db->fetch( dir_id, dirnftype );
    if ( !dirpath )
    {
        errno = EEXIST;
        return NULL;
    }

    uchar * x = filename;
    while ( *x )
        if ( *x++ == '/' )
        {
            errno = EINVAL;
            return NULL;
        }

    uchar newpath[ Inode_tree::MAX_PATH_LENGTH ];
    sprintf( (char*)newpath, "%s/%s", dirpath, filename );

    inode_file_type newfilenftype;
    int newfileid = inode_name_db->fetch( mount_id, newpath, newfilenftype );

    struct stat sb;
    if ( newfileid != -1 )
    {
        ret = hashlru.find( newfileid );
        if ( ret != NULL )
        {
            if ( ret->ref() == 1 )
                zerorefs--;
            errno = 0;
            return ret;
        }
        // else
        if ( rict.clnt.lstat( newpath, &sb ) == 0 )
        {
            bool changedb = false;
            if ((( sb.st_mode & S_IFMT ) == S_IFLNK ) &&
                ( newfilenftype != INODE_LINK ))
            {
                changedb = true;
                newfilenftype = INODE_LINK;
            }
            else if ((( sb.st_mode & S_IFMT ) == S_IFDIR ) &&
                     ( newfilenftype != INODE_DIR ))
            {
                changedb = true;
                newfilenftype = INODE_DIR;
            }
            else if ((( sb.st_mode & S_IFMT ) == S_IFREG ) &&
                     ( newfilenftype != INODE_FILE ))
            {
                changedb = true;
                newfilenftype = INODE_FILE;
            }

            if ( changedb )
            {
                inode_name_db->del( newfileid );
                inode_name_db->add( mount_id, newpath,
                                    newfilenftype, newfileid );
            }

            Inode_remote * i;
            i = new Inode_remote( this, newpath, newfileid, newfilenftype );
            i->sb = sb;
            i->last_stat = time( NULL );
            hashlru.add( i );
            errno = 0;
            return i;
        }
        // else

        inode_name_db->del( newfileid );
        errno = ENOENT;
        return NULL;
    }
    // else

    if ( rict.clnt.lstat( newpath, &sb ) != 0 )
    {
        errno = ENOENT;
        return NULL;
    }
    // else

    if (( sb.st_mode & S_IFMT ) == S_IFLNK )
        newfilenftype = INODE_LINK;
    else if (( sb.st_mode & S_IFMT ) == S_IFDIR )
        newfilenftype = INODE_DIR;
    else if (( sb.st_mode & S_IFMT ) == S_IFREG )
        newfilenftype = INODE_FILE;

    newfileid = inode_name_db->alloc_id();
    inode_name_db->add( mount_id, newpath, newfilenftype, newfileid );

    Inode_remote * i = new Inode_remote( this, newpath,
                                         newfileid, newfilenftype );

    hashlru.add( i );
    errno = 0;
    return i;
}

int
Inode_remote_tree :: deref( Inode * i )
{
    int errsave = errno;

    if ( i->refget() == 1 )
        zerorefs++;

    int r = i->deref();

    if ( zerorefs > MAX_HASH )
        clean();

    errno = errsave;
    return r;
}

int
Inode_remote_tree :: destroy( int fileid, inode_file_type nftype )
{
    Inode_remote * i;
    int err_ret = 0;

    i = hashlru.find( fileid );
    if ( i && i->refget() != 0 )
    {
        errno = EACCES;
        return -1;
    }

    inode_file_type dummynftype;
    uchar * p = inode_name_db->fetch( fileid, dummynftype );
    if ( p == NULL )
    {
        errno = EEXIST;
        return -1;
    }
    if ( nftype == INODE_DIR )
    {
        if ( rict.clnt.rmdir( p ) < 0 )
        {
            if ( errno != ENOENT )
                return -1;
            err_ret = errno;
        }
    }
    else
    {
        if ( rict.clnt.unlink( p ) < 0 )
        {
            if ( errno != ENOENT )
                return -1;
            err_ret = errno;
        }
    }

    if ( i )
    {
        hashlru.remove( i );
        delete i;
        // we know that i's refcount was zero
        zerorefs--;
    }
    inode_name_db->del( fileid );

    errno = err_ret;
    return (err_ret == 0) ? 0 : -1;
}

Inode *
Inode_remote_tree :: createslink( int dirid,
                                  uchar * name, uchar * target )
{
    return _create( dirid, name, target, INODE_LINK, false );
}

Inode *
Inode_remote_tree :: createhlink( int dirid,
                                  uchar * name, int fileid )
{
    Inode_remote * f;
    Inode * ret;

    f = (Inode_remote *)get( fileid );
    if ( f == NULL )
        return NULL;

    ret = _create( dirid, name, f->path, INODE_LINK, true );
    deref( f );
    return ret; 
}

Inode *
Inode_remote_tree :: create( int dirid,
                             uchar * name,
                             inode_file_type nftype )
{
    return _create( dirid, name, NULL, nftype, false );
}

Inode *
Inode_remote_tree :: _create( int dirid,
                              uchar * name, uchar * target,
                              inode_file_type nftype, bool hardlink )
{
    uchar * dirpath;
    inode_file_type dirnftype;
    struct stat sb;

    dirpath = inode_name_db->fetch( dirid, dirnftype );
    if ( !dirpath )
    {
        errno = ENOENT;
        return NULL;
    }

    if ( rict.clnt.lstat( dirpath, &sb ) < 0 )
    {
        Inode_remote * diri = hashlru.find( dirid );
        if ( diri )
        {
            if ( diri->refget() > 0 )
            {
                // can't delete it!
                printf( "inode %s has refcount of %d but doesn't exist!\n",
                        diri->path, diri->refget() );
                errno = ENOENT;
                return NULL;
            }
            // else
            hashlru.remove( diri );
            delete diri;
            // we know refcount is zero
            zerorefs--;
        }
        inode_name_db->del( dirid );
        errno = ENOENT;
        return NULL;
    }

    if (( sb.st_mode & S_IFMT ) != S_IFDIR )
    {
        if ( dirnftype == INODE_DIR )
        {
            if (( sb.st_mode & S_IFMT ) == S_IFLNK )
                dirnftype = INODE_LINK;
            else 
                dirnftype = INODE_FILE;
            inode_name_db->del( dirid );
            inode_name_db->add( mount_id, dirpath, dirnftype, dirid );
        }
        errno = ENOTDIR;
        return NULL;
    }
    
    if ( dirnftype != INODE_DIR )
    {
        dirnftype = INODE_DIR;
        inode_name_db->del( dirid );
        inode_name_db->add( mount_id, dirpath, dirnftype, dirid );
    }

    uchar newpath[ Inode_tree::MAX_PATH_LENGTH ];
    sprintf( (char*)newpath, "%s/%s", dirpath, name );

    int fileid;
    inode_file_type filenftype;

    if ( rict.clnt.lstat( newpath, &sb ) == 0 )
    {
        errno = EEXIST;
        return NULL;
    }

    fileid = inode_name_db->fetch( mount_id, newpath, filenftype );
    if ( fileid != -1 )
        inode_name_db->del( fileid );
    int fd;

    switch ( nftype )
    {
    case INODE_DIR:
        if ( rict.clnt.mkdir( newpath, 0755 ) < 0 )
            return NULL;
        break;

    case INODE_FILE:
        fd = rict.clnt.open( newpath, O_CREAT | O_WRONLY, 0644 );
        if ( fd < 0 )
            return NULL;
        rict.clnt.close( fd );
        break;

    case INODE_LINK:
        if ( hardlink )
        {
            if ( rict.clnt.link( target, newpath ) < 0 )
                return NULL;
            nftype = INODE_FILE;
        }
        else
        {
            if ( rict.clnt.symlink( target, newpath ) < 0 )
                return NULL;
        }
        break;
    }

    fileid = inode_name_db->alloc_id();
    inode_name_db->add( mount_id, newpath, nftype, fileid );

    Inode_remote * ino;
    ino = new Inode_remote( this, newpath, fileid, nftype );
    hashlru.add( ino );
    errno = 0;

    return ino;
}

int
Inode_remote_tree :: rename( int olddir, uchar *oldfile,
                             int newdir, uchar *newfile )
{
    uchar oldpath[ Inode_tree::MAX_PATH_LENGTH ];
    uchar newpath[ Inode_tree::MAX_PATH_LENGTH ];

    uchar * olddirpath;
    uchar * newdirpath;
    inode_file_type olddirnftype;
    inode_file_type newdirnftype;

    olddirpath = inode_name_db->fetch( olddir, olddirnftype );
    newdirpath = inode_name_db->fetch( newdir, newdirnftype );

    if ( olddirpath == NULL || newdirpath == NULL )
    {
        errno = ENOENT;
        return -1;
    }

    struct stat sb1;
    struct stat sb2;

    if (( rict.clnt.lstat( olddirpath, &sb1 ) < 0 ) ||
        ( rict.clnt.lstat( newdirpath, &sb2 ) < 0 ))
    {
        errno = ENOENT;
        return -1;
    }

    if ((( sb1.st_mode & S_IFMT ) != S_IFDIR ) ||
        (( sb2.st_mode & S_IFMT ) != S_IFDIR ))
    {
        errno = ENOTDIR;
        return -1;
    }

    sprintf( (char*)oldpath, "%s/%s", olddirpath, oldfile );
    sprintf( (char*)newpath, "%s/%s", newdirpath, newfile );

    if ( rict.clnt.rename( oldpath, newpath ) < 0 )
        return -1;

    int fileid;
    inode_file_type filenftype;

    fileid = inode_name_db->fetch( mount_id, oldpath, filenftype );
    if ( fileid != -1 )
    {
        inode_name_db->del( fileid  );
        inode_name_db->add( mount_id, newpath, filenftype, fileid );
    }

    Inode_remote * i = hashlru.find( fileid );
    if ( i == NULL )
    {
        errno = 0;
        return 0;
    }

    free( i->path );
    i->path = (uchar*) strdup( (char*)newpath );

    if ( rict.clnt.lstat( newpath, &sb1 ) < 0 )
        // wtf ?
        return -1;

    if (( sb1.st_mode & S_IFMT ) == S_IFDIR )
    {
        printf( "\n"
                "*** NOTE ***\n"
                "*** %s\n"
                "*** renamed to\n"
                "*** %s\n"
                "*** RENAME OF DIRECTORIES IS NOT SAFE\n"
                "*** DUE TO MISSING IMPLEMENTATION OF HANDLER CODE\n"
                "*** NOTE ***\n"
                "\n",
                oldpath, newpath );

// - update path of any currently open inode_remote
//   which is child of the moved directory
// - update path of any indb member which is child
//   of moved directory

    }

    errno = 0;
    return 0;
}

void
Inode_remote_tree :: clean( void )
{
    Inode_remote * ino, * pino;

    if ( rict.bad_fd )
        bad_fd = true;

    // clean LRU bottom up.
    // go thru once, (j==1) and clean anything over a certain age.
    // if that's not enough to get below the zerorefs threshold,
    // go thru again (j==2) and clean anything that has zero refs.

    for ( int j = 1; j <= 2; j++ )
    {
        for ( ino = hashlru.get_lru_tail(); ino; ino = pino )
        {
            pino = hashlru.get_lru_prev( ino );
            if ( ino->refget() == 0 )
            {
                if (( ino->lastref() > 4 ) || ( j == 2 ))
                {
                    hashlru.remove( ino );
                    delete ino;
                    --zerorefs;
                    if (( j == 2 ) && ( zerorefs <= MAX_HASH ))
                        // return early in this case.
                        return;
                }
            }
        }

        if ( zerorefs <= MAX_HASH )
            // no need for a second iteration.
            return;
    }

    // if we get here, there simply isn't enough zeroref'd
    // items left to get below threshold. oh well. leave them
    // there.
}

void
Inode_remote_tree :: print_cache
( int arg, 
  void (*printfunc)( int arg, char * format, ... ) )
{
    int i;
    Inode_remote * ino;
    for ( ino = hashlru.get_lru_head();
          ino;
          ino = hashlru.get_lru_next( ino ))
    {
        printfunc( arg, "%d %s\n", 
                   ino->refget(), ino->path );
    }
}
