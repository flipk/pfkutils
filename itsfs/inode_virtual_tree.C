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

Inode_virtual_tree :: Inode_virtual_tree
( int _tree_id,
  uchar * (*_update_status_info)( void ),
  void (*_command_handler)( char *, int ),
  char *_help_string )
    : Inode_tree( INODE_VIRTUAL, _tree_id )
{
    inos = NULL;
    Inode_virtual * i;
    update_status_info = _update_status_info;
    command_handler = _command_handler;
    help_string = _help_string;

#define ADD(i)   i->n = inos; inos = i; deref( i )

#define ADD2( fileno, id )                                              \
    do {                                                                \
        int newid = id;                                                 \
        i = new Inode_virtual( this, newid, Inode_virtual::fileno );    \
        inode_name_db->add( mount_id, i->name, i->ftype, newid );       \
        ADD(i); newid = inode_name_db->alloc_id();                      \
    } while ( 0 )

    ADD2( INO_TREE_VIRT_ROOT_DIR       , ROOT_INODE_ID             );
    ADD2( INO_TREE_VIRT_ROOT_PARENT_DIR, inode_name_db->alloc_id() );
    ADD2( INO_TREE_VIRT_STATUS_FILE    , inode_name_db->alloc_id() );
    ADD2( INO_TREE_VIRT_CMD_FILE       , inode_name_db->alloc_id() );
    ADD2( INO_TREE_VIRT_HELP_FILE      , inode_name_db->alloc_id() );

#undef  ADD2

}

// the following two are not in the inode_tree interface.

int
Inode_virtual_tree :: register_tree( uchar * name, Inode_tree * newtree )
{
    Inode_virtual * i;
    i = new Inode_virtual( this, newtree->ROOT_INODE_ID, name, newtree );
    ADD( i );
    return newtree->ROOT_INODE_ID;
}

Inode_tree *
Inode_virtual_tree :: kill_tree( int id )
{
    Inode_virtual *i, *pi;

    for ( pi = NULL, i = inos; i; pi = i, i = i->n )
        if ( i->file_id == id )
            break;

    if ( !i )
        return NULL;

    if ( pi )
        pi->n = i->n;
    else
        inos = i->n;

    i->n = NULL;

    Inode_tree * ret = i->owned_tree;
    delete i;
    return ret;
}

Inode_virtual_tree :: ~Inode_virtual_tree( void )
{
    Inode_virtual * i, * ni;
    for ( i = inos; i; i = ni )
    {
        ni = i->n;
        delete i;
    }
}

Inode *
Inode_virtual_tree :: get( int file_id )
{
    Inode_virtual * i;
    for ( i = inos; i; i = i->n )
        if ( i->file_id == file_id )
            break;
    if ( i )
        if ( i->owned_tree )
            i = (Inode_virtual*) i->owned_tree->get( file_id );

    if ( i )
        i->ref();
    else
        errno = ENOENT;

    return i;
}

Inode *
Inode_virtual_tree :: get( int dir_id, uchar * filename )
{
    if ( dir_id != ROOT_INODE_ID )
        return NULL;

    Inode_virtual * i;
    for ( i = inos; i; i = i->n )
        if ( strcmp( (char*)i->name, (char*)filename ) == 0 )
            break;

    if ( i )
    {
        if ( i->owned_tree )
        {
            int owned_root = i->owned_tree->ROOT_INODE_ID;
            i = (Inode_virtual*) i->owned_tree->get( owned_root );
        }
        else
            i->ref();
    }
    else
        errno = ENOENT;

    return i;
}

Inode *
Inode_virtual_tree :: get_parent( int file_id )
{
    if ( file_id == ROOT_INODE_ID )
    {
        errno = EPERM;
        return NULL;
    }
    Inode_virtual * i;
    for ( i = inos; i; i = i->n )
        if ( i->file_id == file_id )
            break;
    if ( !i )
    {
        errno = ENOENT;
        return NULL;
    }
    return get( ROOT_INODE_ID );
}

int
Inode_virtual_tree :: deref( Inode * i )
{
    i->deref();
}

Inode *
Inode_virtual_tree :: create( int dirid, uchar * name, inode_file_type ftype )
{
    errno = EPERM;
    return NULL;
}

Inode *
Inode_virtual_tree :: createslink( int dirid, uchar * name,
                                   uchar * target )
{
    errno = EPERM;
    return NULL;
}

Inode *
Inode_virtual_tree :: createhlink( int dirid, uchar * name, int fileid )
{
    errno = EPERM;
    return NULL;
}

int
Inode_virtual_tree :: destroy( int fileid, inode_file_type )
{
    errno = EPERM;
    return NULL;
}

int
Inode_virtual_tree :: rename( int olddir, uchar * oldfile,
                              int newdir, uchar * newfile )
{
    errno = EPERM;
    return NULL;
}

void
Inode_virtual_tree :: clean( void )
{
    // nothing to clean
}

void
Inode_virtual_tree :: print_cache( int arg, void (*printfunc)
                                   (int arg, char *format,...) )
{
    // nothing to print
}
