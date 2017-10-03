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

#include "inode_virtual.H"
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

    switch ( itype )
    {
    case INO_TREE_VIRT_ROOT_DIR:
        name = (uchar*)strdup( "." );
        break;

    case INO_TREE_VIRT_ROOT_PARENT_DIR:
        name = (uchar*)strdup( ".." );
        break;

    case INO_TREE_VIRT_STATUS_FILE:
        ftype = INODE_FILE;
        name = (uchar*)strdup( "status" );
        break;

    case INO_TREE_VIRT_CMD_FILE:
        ftype = INODE_FILE;
        name = (uchar*)strdup( "command" );
        break;

    case INO_TREE_VIRT_HELP_FILE:
        ftype = INODE_FILE;
        name = (uchar*)strdup( "help" );
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
    owned_tree = _owned_tree;
    name = (uchar*)strdup( (char*)path );
}

Inode_virtual :: ~Inode_virtual( void )
{
    if ( name )
        free( name );
    if ( status_info )
        free( status_info );

    // don't delete owned tree,
    // cuz it should already be registered in the nfssrv.
    // nfssrv will delete it when it bails out.
}

int      
Inode_virtual :: isattr( sattr &s )
{
    // ignore (but succeed) if its the command file; 
    // we allow writes to that file.

    if ( itype == INO_TREE_VIRT_CMD_FILE )
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
             "\n"
             "%s\n"
             "status : ok   time : %d.%06d   "
             "ctxsw(v) : %d   ctxsw(iv) : %d\n"
             "\n",
             it->update_status_info(),
             ru.ru_stime.tv_sec, ru.ru_stime.tv_usec,
             ru.ru_nvcsw, ru.ru_nivcsw );

    Inode_virtual * inov;
    for ( inov = it->inos; inov; inov = inov->n )
        sprintf( fullstatus + strlen( fullstatus ),
                 "mounted virtual inode : type %d name %s\n",
                 inov->itype, inov->name );

    sprintf( fullstatus + strlen( fullstatus ), "\n" );

    if ( status_info )
        free( status_info );

    status_info = (uchar*) malloc( strlen( fullstatus ) + 1 );
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

    default:
        errno = EPERM;
        return -1;
    }
}

int      
Inode_virtual :: iwrite( int offset, uchar * buf, int size )
{
    if ( itype != INO_TREE_VIRT_CMD_FILE )
    {
        errno = EPERM;
        return -1;
    }

    it->command_handler( (char*)buf, size );

    return 0;
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
    int i;
    if ( file_id != it->ROOT_INODE_ID )
    {
        errno = ENOTDIR;
        return -1;
    }
    Inode_virtual * j;

    i = 0;
    for ( j = it->inos; j; i++, j = j->n )
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
