/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

#include "inode_virtual.H"
#include "lognew.H"
#include "control_pipe.H"

#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>

Inode_virtual :: Inode_virtual( Inode_virtual_tree * _it,
                                int _file_id, ITVT _itype )
    : Inode( INODE_VIRTUAL, _file_id, _it->tree_id, INODE_DIR )
{
    it = _it;
    itype = _itype;
    dirposval = 0;
    name = NULL;
    bad = false;
    status_info = NULL;
    cpipe = NULL;

    switch ( itype )
    {
    case INO_TREE_VIRT_ROOT_DIR:
        name = (uchar*)STRDUP( "." );
        break;

    case INO_TREE_VIRT_ROOT_PARENT_DIR:
        name = (uchar*)STRDUP( ".." );
        break;

    case INO_TREE_VIRT_STATUS_FILE:
        ftype = INODE_FILE;
        name = (uchar*)STRDUP( "status" );
        break;

    case INO_TREE_VIRT_CMD_FILE:
        ftype = INODE_FILE;
        name = (uchar*)STRDUP( "command" );
        break;

    case INO_TREE_VIRT_HELP_FILE:
        ftype = INODE_FILE;
        name = (uchar*)STRDUP( "help" );
        break;

    case INO_TREE_VIRT_TREE:
    default:
        bad = true;
        break;
    }

    owned_tree = NULL;
}

Inode_virtual :: Inode_virtual( Inode_virtual_tree * _it,
                                int _file_id, uchar * path,
                                Inode_tree * _owned_tree )
    : Inode( INODE_VIRTUAL, _file_id, _it->tree_id, INODE_DIR )
{
    it = _it;
    itype = INO_TREE_VIRT_TREE;
    dirposval = 0;
    bad = false;
    status_info = NULL;
    cpipe = NULL;
    owned_tree = _owned_tree;
    name = (uchar*)STRDUP( (char*)path );
}

Inode_virtual :: Inode_virtual ( Inode_virtual_tree * _it,
                                 int _file_id,
                                 uchar * path )
    : Inode( INODE_VIRTUAL, _file_id, _it->tree_id, INODE_FILE )
{
    it = _it;
    itype = INO_TREE_VIRT_CONTROL_COMM;
    dirposval = 0;
    bad = false;
    status_info = NULL;
    owned_tree = NULL;
    name = (uchar*)STRDUP( (char*) path );
    cpipe = new Control_Pipe;
}

Inode_virtual :: ~Inode_virtual( void )
{
    if ( name )
        free( name );
    if ( status_info )
        free( status_info );
    if ( cpipe )
        delete cpipe;

    // don't delete owned tree,
    // cuz it should already be registered in the nfssrv.
    // nfssrv will delete it when it bails out.
}

int      
Inode_virtual :: isattr( sattr &s )
{
    // ignore (but succeed) if its the command file or
    // a control pipe; we allow writes to them.

    if ( itype == INO_TREE_VIRT_CMD_FILE    ||
         itype == INO_TREE_VIRT_CONTROL_COMM )
        return 0;

    if ( itype == INO_TREE_VIRT_TREE )
    {
        Inode * i;
        int ret;

        i = owned_tree->get( owned_tree->ROOT_INODE_ID );
        if ( !i )
        {
            errno = ENOENT;
            return -1;
        }
        ret = i->isattr( s );
        i->deref();
        return ret;
    }

    errno = EPERM;
    return -1;
}

int      
Inode_virtual :: ifattr( fattr & f )
{
    if ( bad )
    {
        errno = EIO;
        return -1;
    }

    switch ( itype )
    {
    case INO_TREE_VIRT_ROOT_DIR:
    case INO_TREE_VIRT_ROOT_PARENT_DIR:
        f.type = NFDIR;
        f.mode = 0777;
        f.size = 512;
        break;

    case INO_TREE_VIRT_TREE:
    {
        Inode * i;
        int ret;

        i = owned_tree->get( owned_tree->ROOT_INODE_ID );
        if ( !i )
        {
            errno = ENOENT;
            return -1;
        }
        ret = i->ifattr( f );
        i->deref();
        return ret;
    }

    case INO_TREE_VIRT_CMD_FILE:
        f.type = NFREG;
        f.mode = 0222;
        f.size = 0;
        break;

    case INO_TREE_VIRT_HELP_FILE:
        f.type = NFREG;
        f.mode = 0444;
        f.size = strlen( it->help_string );
        break;

    case INO_TREE_VIRT_STATUS_FILE:
        f.type = NFREG;
        f.mode = 0444;
        update_status_info();
        f.size = strlen( (char*)status_info );
        break;

    case INO_TREE_VIRT_CONTROL_COMM:
        f.type = NFREG;
        f.mode = cpipe ? 0666 : 0000;
        f.size = cpipe ? cpipe->len() : 0;
        break;
    }

    f.rdev = 1;
    f.fsid = it->mount_id;
    f.fileid = file_id;

    f.nlink = 1;
    f.uid = getuid();
    f.gid = getgid();
    f.blocksize = 8192;
    f.blocks = 1;

    struct timeval tv;
    gettimeofday( &tv, NULL );

    f.atime. seconds = tv.tv_sec;
    f.atime.useconds = tv.tv_usec;
    f.mtime. seconds = tv.tv_sec;
    f.mtime.useconds = tv.tv_usec;
    f.ctime. seconds = tv.tv_sec;
    f.ctime.useconds = tv.tv_usec;

    return 0;
}

void
Inode_virtual :: update_status_info( void )
{
    char fullstatus[ 8192 ];
    memset( fullstatus, 0, 8192 );

    struct rusage ru;
    getrusage( RUSAGE_SELF, &ru );
    sprintf( fullstatus,
             "%s\n"
             "status : ok   time : %d.%06d   "
             "ctxsw(v) : %d   ctxsw(iv) : %d\n"
             "\n",
             it->update_status_info(),
             ru.ru_stime.tv_sec, ru.ru_stime.tv_usec,
             ru.ru_nvcsw, ru.ru_nivcsw );

    Inode_virtual * inov;
    for ( inov = it->inos.get_head();
          inov; inov = it->inos.get_next( inov ))
    {
        sprintf( fullstatus + strlen( fullstatus ),
                 "mounted virtual inode : type %d name %s\n",
                 inov->itype, inov->name );
    }

    sprintf( fullstatus + strlen( fullstatus ), "\n" );

    if ( status_info )
        free( status_info );

    status_info = (uchar*) MALLOC( strlen( fullstatus ) + 1 );
    strcpy( (char*)status_info, fullstatus );
}

int      
Inode_virtual :: iread( int offset, uchar * buf, int &size )
{
    int l, s;

    if ( bad )
    {
        errno = EIO;
        return -1;
    }

    switch ( itype )
    {
    case INO_TREE_VIRT_STATUS_FILE:
        update_status_info();
        size = strlen( (char*)status_info );
        memcpy( buf, status_info, size );
        return 0;

    case INO_TREE_VIRT_HELP_FILE:
        l = strlen( it->help_string );
        if ( offset >= l )
        {
            size = 0;
            return 0;
        }
        s = l - offset;
        if ( s > size )
            s = size;
        memcpy( buf, it->help_string + offset, s );
        return 0;

    case INO_TREE_VIRT_CONTROL_COMM:
        errno = cpipe ? cpipe->read( buf, size ) : EIO;
        return (errno == 0) ? 0 : -1;

    default:
        errno = EPERM;
        return -1;
    }
}

int      
Inode_virtual :: iwrite( int offset, uchar * buf, int size )
{
    switch ( itype )
    {
    case INO_TREE_VIRT_CMD_FILE:
        it->command_handler( (char*)buf, size );
        return 0;

    case INO_TREE_VIRT_CONTROL_COMM:
        errno = cpipe ? cpipe->write( buf, size ) : EIO;
        return (errno == 0) ? 0 : -1;
    }
    // default

    errno = EPERM;
    return -1;
}

int
Inode_virtual :: setdirpos( int cookie )
{
    if ( file_id != it->ROOT_INODE_ID )
    {
        errno = ENOTDIR;
        return -1;
    }

    dirposval = cookie;
    return 0;
}

int      
Inode_virtual :: readdir( int &fileno, uchar * filename )
{
    Inode_virtual * j;
    int i;

    if ( file_id != it->ROOT_INODE_ID )
    {
        errno = ENOTDIR;
        return -1;
    }

    i = 0;
    for ( j = it->inos.get_head();  j;  i++, j = it->inos.get_next( j ))
        if ( i == dirposval )
            break;

    if ( !j )
        return -1;

    dirposval++;
    fileno = j->file_id;
    strcpy( (char*)filename, (char*)j->name );
    return 0;
}

int
Inode_virtual :: readlink( uchar * buf )
{
    // virtual inode fs does not support symlinks.

    errno = EEXIST;
    return -1;
}
