
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

#include "inode_remote.H"
#include "lognew.H"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>


struct Inode_remote_tree :: Inode_remote_tree_hash_1 {
public:
    static int hash_key( Inode_remote * item ) {
        return item->file_id;
    }
    static int hash_key( int key ) {
        return key;
    }
    static bool hash_key_compare( Inode_remote * item, int key ) {
        return (item->file_id == key);
    }
};


Inode_remote_tree :: Inode_remote_tree( int tree_id,
                                        uchar * _path,
                                        int fd )
    : Inode_tree( INODE_REMOTE, tree_id ),
      rict( fd )
{
    uchar * p;
    bool dummy;

    bad_fd = false;

    rootpath = (uchar*) STRDUP( (char*)_path );
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
Inode_remote_tree :: _get( bool hash_only, int file_id )
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

    if ( hash_only )
    {
        errno = ENOENT;
        return NULL;
    }

    inode_file_type nftype;
    uchar * p = inode_name_db->fetch( file_id, nftype );

    if ( p == NULL )
    {
        errno = ENOENT;
        return NULL;
    }

    struct stat sb;

    if ( !get_stats_somehow( (char*)p, file_id, &sb ))
    {
        errno = ENOENT;
        delete p;
        return NULL;
    }

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
    ino = LOGNEW Inode_remote( this, p, file_id, nftype );
    ino->sb = sb;
    ino->last_stat = time( NULL );
    hashlru.add( ino );
    errno = 0;
    delete p;
    return ino;
}

Inode *
Inode_remote_tree :: _get_parent( bool hash_only, int file_id )
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
    delete p;

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

    return _get( hash_only, parentfileid );
}

bool
Inode_remote_tree :: get_stats_somehow( char * path,
                                        int file_id, struct stat * sb )
{
    bool parentfound = false;
    Inode_remote * parent = (Inode_remote *) _get_parent( true, file_id );

    if ( parent != NULL )
    {
        char * p2 = &path[ strlen( path ) - 1 ];
        while ( *p2 != '/' && p2 != path )
            p2--;
        p2++;

        remino_readdir2_entry * e;
        for ( e = parent->dirlist; e; e = e->next )
        {
            if ( strcmp( p2, (char*)e->dirent.name ) == 0 )
                break;
        }

        if ( e )
        {
            parentfound = true;
            rict.clnt.xdrstat_to_stat( &e->stat, sb );
        }

        deref( parent );
        if ( parentfound )
            return true;
    }

    if ( !parentfound )
        if ( rict.clnt.lstat( (uchar*)path, sb ) < 0 )
            return false;

    return true;
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
    if ( dirpath == NULL )
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
    delete dirpath;

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

        if ( get_stats_somehow( (char*)newpath, newfileid, &sb ))
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
            i = LOGNEW Inode_remote( this, newpath, newfileid, newfilenftype );
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

    switch ( sb.st_mode & S_IFMT )
    {
    case S_IFLNK:
        newfilenftype = INODE_LINK;
        break;
    case S_IFDIR:
        newfilenftype = INODE_DIR;
        break;
    case S_IFREG:
        newfilenftype = INODE_FILE;
        break;
    }

    newfileid = inode_name_db->alloc_id();
    inode_name_db->add( mount_id, newpath, newfilenftype, newfileid );

    Inode_remote * i = LOGNEW Inode_remote( this, newpath,
                                            newfileid, newfilenftype );
    i->sb = sb;
    i->last_stat = time( NULL );

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
    int ret = 0;

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
    // deleting p is gross
    if ( nftype == INODE_DIR )
    {
        if ( rict.clnt.rmdir( p ) < 0 )
        {
            if ( errno != ENOENT )
                ret = -1;
            err_ret = errno;
        }
    }
    else
    {
        if ( rict.clnt.unlink( p ) < 0 )
        {
            if ( errno != ENOENT )
                ret = -1;
            err_ret = errno;
        }
    }
    delete p;
    if ( ret == 0 )
    {
        if ( i )
        {
            hashlru.remove( i );
            delete i;
            // we know that i's refcount was zero
            zerorefs--;
        }
        inode_name_db->del( fileid );
    }

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

    // deleting dirpath is dirty

    if ( rict.clnt.lstat( dirpath, &sb ) < 0 )
    {
        delete dirpath;
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
        delete dirpath;
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
    delete dirpath;

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
    ino = LOGNEW Inode_remote( this, newpath, fileid, nftype );
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
        delete olddirpath;
        delete newdirpath;
        errno = ENOENT;
        return -1;
    }

    struct stat sb1;
    struct stat sb2;

    if (( rict.clnt.lstat( olddirpath, &sb1 ) < 0 ) ||
        ( rict.clnt.lstat( newdirpath, &sb2 ) < 0 ))
    {
        delete olddirpath;
        delete newdirpath;
        // should update name_db
        errno = ENOENT;
        return -1;
    }

    if ((( sb1.st_mode & S_IFMT ) != S_IFDIR ) ||
        (( sb2.st_mode & S_IFMT ) != S_IFDIR ))
    {
        delete olddirpath;
        delete newdirpath;
        // should update name_db
        errno = ENOTDIR;
        return -1;
    }

    sprintf( (char*)oldpath, "%s/%s", olddirpath, oldfile );
    sprintf( (char*)newpath, "%s/%s", newdirpath, newfile );
    delete olddirpath;
    delete newdirpath;

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
    i->path = (uchar*) STRDUP( (char*)newpath );

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
