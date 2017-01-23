
#include "inode_virtual.H"
#include "lognew.H"
#include <errno.h>

Inode_virtual_tree :: Inode_virtual_tree
( int _tree_id,
  uchar * (*_update_status_info)( void ),
  void (*_command_handler)( char *, int ),
  char *_help_string )
    : Inode_tree( INODE_VIRTUAL, _tree_id )
{
    Inode_virtual * i;
    update_status_info = _update_status_info;
    command_handler = _command_handler;
    help_string = _help_string;

#define ADD2( fileno, id )                                              \
    do {                                                                \
        int newid = id;                                                 \
        i = LOGNEW Inode_virtual( this, newid, Inode_virtual::fileno ); \
        inode_name_db->add( mount_id, i->name, i->ftype, newid );       \
        inos.add( i ); deref( i ); newid = inode_name_db->alloc_id();   \
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
    i = LOGNEW Inode_virtual( this, newtree->ROOT_INODE_ID, name, newtree );
    inos.add( i );
    deref( i );
    return newtree->ROOT_INODE_ID;
}

Inode_tree *
Inode_virtual_tree :: kill_tree( int id )
{
    Inode_virtual * i;

    for ( i = inos.get_head();  i;  i = inos.get_next(i) )
        if ( i->file_id == id )
            break;

    if ( !i )
        return NULL;

    inos.remove( i );
    Inode_tree * ret = i->owned_tree;
    delete i;
    return ret;
}

Inode_virtual_tree :: ~Inode_virtual_tree( void )
{
    Inode_virtual * i;
    while ( i = inos.dequeue_head() )
        delete i;
}

Inode *
Inode_virtual_tree :: get( int file_id )
{
    Inode_virtual * i;

    for ( i = inos.get_head();  i;  i = inos.get_next(i) )
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
    for ( i = inos.get_head();  i;  i = inos.get_next(i) )
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
    for ( i = inos.get_head();  i;  i = inos.get_next(i) )
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
    if ( strncmp( (char*) name, "control-pipe-", 13 ) != 0 )
    {
        errno = EPERM;
        return NULL;
    }
    // else

    int newid = inode_name_db->alloc_id();
    Inode_virtual * i = LOGNEW Inode_virtual( this, newid, name );
    inode_name_db->add( mount_id, i->name, i->ftype, newid );
    inos.add(i);

    return i;
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
Inode_virtual_tree :: destroy( int fileid, inode_file_type ft )
{
    Inode_virtual * i;

    for ( i = inos.get_head(); i; i = inos.get_next(i) )
        if ( i->file_id == fileid )
            break;

    if ( i == NULL )
    {
        errno = ENOENT;
        return -1;
    }

    if ( i->refget() != 0 )
    {
        errno = EACCES;
        return -1;
    }

    // only type we support deleting is a control comm

    if ( i->itype != Inode_virtual::INO_TREE_VIRT_CONTROL_COMM )
    {
        errno = EPERM;
        return -1;
    }

    if ( ft != INODE_FILE )
    {
        errno = ENOTDIR;
        return -1;
    }

    inos.remove( i );
    delete i;
    errno = 0;
    return 0;
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
