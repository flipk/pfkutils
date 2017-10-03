/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include "nfssrv.H"
#include "xdr.h"
#include "rpc.h"

static char * nfs_procs[] = {
    "NULL",     "getattr", "setattr", "root",    "lookup",
    "readlink", "read",    "wrcache", "write",   "create",
    "remove",   "rename",  "link",    "symlink", "mkdir",
    "rmdir",    "readdir", "statfs"
};

// this is the nfs dispatcher.  when svc_run has pulled a packet
// off of a UDP socket, it decodes the authenticator and the RPC header.
// it also verifies the protocol and version numbers.  when that is complete
// it passes off the packet here.  this function is then responsible for
// fetching the remainder of the data for the procedure and then implementing
// the procedure.

uchar *
nfssrv :: dispatch( uchar * buf, int bufsize, int &replysize )
{
    static struct myxdr_functions {
        xdrproc_t   myxdr_arg;
        xdrproc_t   myxdr_res;
        void (nfssrv::*nfs_func)( void );
    } myxdr_functions[] = {   

// this array is indexed by the NFS procedure number.
// each member contains the xdr function which decodes the
// arguments to the procedure, the xdr function which encodes the 
// reply for the procedure, and a pointer to the actual implementation
// of the procedure.  note that null, root, and writecache are not 
// actually used in the nfsv2 protocol.

#define X xdrproc_t
        { (X)myxdr_void,        (X)myxdr_void,        &nfssrv::null_2     },
        { (X)myxdr_nfs_fh,      (X)myxdr_attrstat,    &nfssrv::getattr_2  },
        { (X)myxdr_sattrargs,   (X)myxdr_attrstat,    &nfssrv::setattr_2  },
        { (X)myxdr_void,        (X)myxdr_void,        &nfssrv::root_2     },
        { (X)myxdr_diropargs,   (X)myxdr_diropres,    &nfssrv::lookup_2   },
        { (X)myxdr_nfs_fh,      (X)myxdr_readlinkres, &nfssrv::readlink_2 },
        { (X)myxdr_readargs,    (X)myxdr_readres,     &nfssrv::read_2     },
        { (X)myxdr_void,        (X)myxdr_void,        &nfssrv::wrcache_2  },
        { (X)myxdr_writeargs,   (X)myxdr_attrstat,    &nfssrv::write_2    },
        { (X)myxdr_createargs,  (X)myxdr_diropres,    &nfssrv::create_2   },
        { (X)myxdr_diropargs,   (X)myxdr_nfsstat,     &nfssrv::remove_2   },
        { (X)myxdr_renameargs,  (X)myxdr_nfsstat,     &nfssrv::rename_2   },
        { (X)myxdr_linkargs,    (X)myxdr_nfsstat,     &nfssrv::link_2     },
        { (X)myxdr_symlinkargs, (X)myxdr_nfsstat,     &nfssrv::symlink_2  },
        { (X)myxdr_createargs,  (X)myxdr_diropres,    &nfssrv::mkdir_2    },
        { (X)myxdr_diropargs,   (X)myxdr_nfsstat,     &nfssrv::rmdir_2    },
        { (X)myxdr_readdirargs, (X)myxdr_readdirres,  &nfssrv::readdir_2  },
        { (X)myxdr_nfs_fh,      (X)myxdr_statfsres,   &nfssrv::statfs_2   }
#undef  X

    };

#define DIM(x) (sizeof(x)/sizeof(x[0]))

    transaction_count++;

    errno = 0;

    // initialize argument space so that the xdr decode
    // operation dynamically allocates any space the structure requires.
    memset( &argument, 0, sizeof( argument ));

    XDR xdr;
    rpccallmsg call;
    rpcreplymsg reply;
    uchar * ret = NULL;
    struct myxdr_functions *xf = NULL;

    xdr.encode_decode = XDR_DECODE;
    xdr.data = buf;
    xdr.position = 0;
    xdr.bytes_left = bufsize;

    memset( &call, 0, sizeof( call ));
    memset( &reply, 0, sizeof( reply ));

    if ( !myxdr_rpccallmsg( &xdr, &call ))
        return NULL;

    // if its an invalid procedure number, tell 'em off.
    if ( call.procedure > DIM( myxdr_functions ))
    {
        xdr.encode_decode = XDR_ENCODE;
        xdr.data = replybuffer;
        xdr.position = 0;
        xdr.bytes_left = sizeof( replybuffer );

        reply.xid = call.xid;
        reply.dir = RPC_REPLY_MSG;
        reply.stat = RPC_MSG_DENIED;
        reply.verf_flavor = 0;
        reply.verf_length = 0;
        reply.verf_base = NULL;
        reply.verf_stat = RPC_AUTH_FAILED;

        if ( !myxdr_rpcreplymsg( &xdr, &reply ))
            return NULL;

        replysize = xdr.position;
        ret = replybuffer;
        goto out;
    }

    xf = &myxdr_functions[ call.procedure ];

    if ( !(xf->myxdr_arg)( &xdr, &argument ))
        goto out;

    (this->*(xf->nfs_func))();

    xdr.encode_decode = XDR_ENCODE;
    xdr.data = replybuffer;
    xdr.position = 0;
    xdr.bytes_left = sizeof( replybuffer );

    reply.xid = call.xid;
    reply.dir = RPC_REPLY_MSG;
    reply.stat = RPC_MSG_ACCEPTED;
    reply.verf_flavor = 0;
    reply.verf_length = 0;
    reply.verf_base = NULL;
    reply.verf_stat = RPC_AUTH_OK;

    if ( !myxdr_rpcreplymsg( &xdr, &reply ))
        goto out;

    if ( !(xf->myxdr_res)( &xdr, &replies ))
        goto out;

    replysize = xdr.position;
    ret = replybuffer;

 out:
    xdr.encode_decode = XDR_FREE;
    xdr.data = buf;
    xdr.position = 0;
    xdr.bytes_left = bufsize;

    (void) myxdr_rpccallmsg( &xdr, &call );
    if ( xf )
        (void) (xf->myxdr_arg)( &xdr, &argument );

    return ret;
}

// convert a unix errno into an nfs errno.
// funny thing is, on this OS they are all the
// same value.  in fact, i think they are standardized
// values in the BSD kernel interface world...
// but let's not make that assumption. this also gives
// us the opportunity to validate the value (only those
// nfs errno values listed here are valid in nfs v2).

// static
enum nfsstat
nfssrv :: nfs_error( void )
{
    const struct {
        int nfs_error;
        int unix_error;
    } errortab[] = {
        { NFS_OK,             0            },
        { NFSERR_PERM,        EPERM        },
        { NFSERR_NOENT,       ENOENT       },
        { NFSERR_IO,          EIO          },
        { NFSERR_NXIO,        ENXIO        },
        { NFSERR_ACCES,       EACCES       },
        { NFSERR_EXIST,       EEXIST       },
        { NFSERR_NODEV,       ENODEV       },
        { NFSERR_NOTDIR,      ENOTDIR      },
        { NFSERR_ISDIR,       EISDIR       },
        { NFSERR_FBIG,        EFBIG        },
        { NFSERR_NOSPC,       ENOSPC       },
        { NFSERR_ROFS,        EROFS        },
        { NFSERR_NAMETOOLONG, ENAMETOOLONG },
        { NFSERR_NOTEMPTY,    ENOTEMPTY    },
        { NFSERR_DQUOT,       EDQUOT       },
        { NFSERR_STALE,       ESTALE       },
        { NFSERR_WFLUSH,      EIO          },
        { NFSERR_IO,          -1           }
    };
    int i, ex = errno;

    for ( i = 0; ; i++ )
    {
        int e = errortab[i].unix_error;
        if ( e == ex || e == -1 )
            break;
    }

    return (enum nfsstat) errortab[i].nfs_error;
}

// now for the implementation of the nfs v2 procedures.

bool
nfssrv :: FHDEC( int &trid, nfs_fh *fh )
{
    bool ok = true;

    if ( !handle.decode( crypt, fh ))
        ok = false;
    else
    {
        trid = handle.tree_id.get();
        if (( trid >= MAX_TREES ) || ( it[trid] == NULL ) ||
            ( it[trid]->mount_id != handle.mount_id.get() ))
        {
            ok = false;
        }
    }

    if ( !ok )
        errno = EINVAL;

    return ok;
}

bool
nfssrv :: GETI( int trid, FileHandle &handle, Inode * &ino )
{
    ino = it[trid]->get( handle.file_id.get() );
    if ( ino == NULL )
    {
        errno = EINVAL;
        return false;
    }
    return true;
}

void
nfssrv :: getattr_2( void )
{
#define arg argument.getattr_2_arg
#define res replies.attrstat_result
    int trid;
    Inode * i;

    if ( !FHDEC( trid, &arg ))
        goto out;
    if ( !GETI( trid, handle, i ))
        goto out;

    i->ifattr( res.attrstat_u.attributes );
    it[trid]->deref( i );

 out:
    res.status = nfs_error();
#undef  res
#undef  arg
}

void
nfssrv :: setattr_2( void )
{
#define arg argument.setattr_2_arg
#define res replies.attrstat_result
    int trid;
    Inode * i;

    if ( !FHDEC( trid, &arg.file ))
        goto out;
    if ( !GETI( trid, handle, i ))
        goto out;

    if ( i->isattr( arg.attributes ) < 0 )
        goto out2;

    i->ifattr( res.attrstat_u.attributes );

 out2:
    it[trid]->deref( i );

 out:
    res.status = nfs_error();
#undef  res
#undef  arg
}

void
nfssrv :: lookup_2( void )
{
#define arg argument.lookup_2_arg
#define res replies.diropres_result
    int trid;
    Inode *i, *i2;

    if ( !FHDEC( trid, &arg.dir ))
        goto out;
    if ( !GETI( trid, handle, i ))
        goto out;

    if ( strcmp( (char*)arg.name, "." ) == 0 )
    {
        // fetch the dir itself, never ask for "."
        i2 = it[trid]->get( i->file_id );
    }
    else if ( strcmp( (char*)arg.name, ".." ) == 0 )
    {
        // ask for the dir's parent, never ask for ".."
        i2 = it[trid]->get_parent( i->file_id );
        if ( i2 == NULL )
        {
            // we didn't find it.  perhaps its in one of the other trees?
            for ( int c = 0; c < MAX_TREES; c++ )
            {
                if ( it[c] )
                    i2 = it[c]->get_parent( i->file_id );
                if ( i2 )
                    break;
            }
        }
    }
    else
    {
        i2 = it[trid]->get( i->file_id, arg.name );
    }

    if ( i2 == NULL )
    {
        errno = ENOENT;
        goto out2;
    }

    handle.userid.set( getuid() );
    handle.tree_id.set( i2->tree_id );
    handle.file_id.set( i2->file_id );
    handle.mount_id.set( it[i2->tree_id]->mount_id );

    handle.encode( crypt, &res.diropres_u.diropres.file );

    i2->ifattr( res.diropres_u.diropres.attributes );
    errno = 0;

 out2:
    it[trid]->deref( i );
    if ( i2 != NULL )
        it[trid]->deref( i2 );

 out:
    res.status = nfs_error();
#undef  res
#undef  arg
}

void
nfssrv :: read_2( void )
{
#define arg argument.read_2_arg
#define res replies.readres_result
    Inode * i = NULL;
    int size;
    int trid;

    if ( !FHDEC( trid, &arg.file ))
        goto out;
    if ( !GETI( trid, handle, i ))
        goto out;

    size = arg.count;
    if ( i->iread( arg.offset, readbuf, size ) < 0 )
        goto out2;

    res.readres_u.reply.data.data_val = readbuf;
    res.readres_u.reply.data.data_len = size;
    i->ifattr( res.readres_u.reply.attributes );

 out2:
    it[trid]->deref( i );

 out:
    res.status = nfs_error();
#undef  res
#undef  arg
}

void
nfssrv :: write_2( void )
{
#define arg argument.write_2_arg
#define res replies.attrstat_result
    Inode * i;
    int trid;

    if ( !FHDEC( trid, &arg.file ))
        goto out;
    if ( !GETI( trid, handle, i ))
        goto out;

    if ( i->iwrite( arg.offset,
                    arg.data.data_val,
                    arg.data.data_len ) < 0 )
        goto out2;

    i->ifattr( res.attrstat_u.attributes );

 out2:
    it[trid]->deref( i );

 out:
    res.status = nfs_error();
#undef  res
#undef  arg
}

void
nfssrv :: rename_2( void )
{
#define arg argument.rename_2_arg
#define res replies.nfsstat_result
    int fromdirid, todirid;
    int trid, trid2;

    if ( !FHDEC( trid, &arg.from.dir ))
        goto out;
    fromdirid = handle.file_id.get();
    if ( !FHDEC( trid2, &arg.to.dir ))
        goto out;
    todirid = handle.file_id.get();
    if ( trid != trid2 )
    {
        errno = EXDEV;
        goto out;
    }

    (void) it[trid]->rename( fromdirid, arg.from.name,
                             todirid, arg.to.name );
 out:
    res = nfs_error();
#undef  res
#undef  arg
}

void
nfssrv :: readdir_2( void )
{
#define arg argument.readdir_2_arg
#define res2 replies.readdirres_result
#define res res2.readdirres_u.reply
    Inode * i;
    int cookie, j, bytes, max_bytes;
    int trid;

    if ( !FHDEC( trid, &arg.dir ))
        goto out;
    if ( !GETI( trid, handle, i ))
        goto out;

    cookie = *(int*)&arg.cookie;
    max_bytes = arg.count;

    if ( i->setdirpos( cookie ) < 0 )
        goto out2;

    res.entries = &entries[0];
    res.eof = false;

    for ( bytes = j = 0 ; j < 511; j++ )
    {
        int fileno;

        entries[j].fileid = 0;
        *(int*)entries[j].cookie = cookie + j + 1;
        entries[j].nextentry = NULL;
        entries[j].name = filenames[j];
        filenames[j][0] = 0;

        if ( i->readdir( fileno, filenames[j] ) < 0 )
        {
            res.eof = true;
            break;
        }

        bytes += sizeof(entry) + strlen( (char*)filenames[j] ) + 3;

        if ( bytes > max_bytes )
            break;

        if ( j > 0 )
            entries[j-1].nextentry = &entries[j];

        entries[j].fileid = fileno;
    }

    errno = 0;

 out2:
    it[trid]->deref( i );

 out:
    res2.status = nfs_error();
#undef  res
#undef  res2
#undef  arg
}

void
nfssrv :: statfs_2( void )
{
#define arg argument.statfs_2_arg
#define res replies.statfsres_result
#define res2 res.statfsres_u.reply

// we could actually use a good way to fetch filesystem
// information ... but its probably hard to depend on.
// for now, return garbage.

    res2.tsize  = 8192;
    res2.bsize  = 8192;
    res2.blocks = 9000000;
    res2.bfree  = 9000000;
    res2.bavail = 9000000;
    res.status  = NFS_OK;
#undef  res
#undef  res2
#undef  arg
}

void
nfssrv :: _create2( inode_file_type nftype )
{
#define arg argument.create_2_arg
#define res replies.diropres_result
#define res2 res.diropres_u.diropres
    Inode * f = NULL;
    int dirid, trid = -1, e;

    if ( !FHDEC( trid, &arg.where.dir ))
        goto out;

    dirid = handle.file_id.get();
    f = it[trid]->create( dirid, arg.where.name, nftype );
    if ( f == NULL )
        goto out;

    e = errno;
    f->isattr( arg.attributes );
    f->ifattr( res2.attributes );
    handle.userid.set( getuid() );
    handle.tree_id.set( f->tree_id );
    handle.file_id.set( f->file_id );
    handle.mount_id.set( it[trid]->mount_id );
    handle.encode( crypt, &res2.file );
    errno = e;

 out:
    if ( f && trid != -1 )
        it[trid]->deref( f );

    res.status = nfs_error();
#undef  res
#undef  res2
#undef  arg
}

void
nfssrv :: create_2( void )
{
    _create2( INODE_FILE );
}

void
nfssrv :: mkdir_2( void )
{
    _create2( INODE_DIR );
}

void
nfssrv :: _remove2( bool isdir )
{
#define arg argument.remove_2_arg
#define res replies.nfsstat_result
    int trid, dirid, fileid;
    inode_file_type rm_ftype;
    Inode * f;

    if ( !FHDEC( trid, &arg.dir ))
        goto out;
    dirid = handle.file_id.get();
    f = it[trid]->get( dirid, arg.name );
    if ( f == NULL )
        goto out;
    fileid = f->file_id;
    rm_ftype = f->ftype;

    it[trid]->deref( f );

    if ( isdir && rm_ftype != INODE_DIR )
    {
        errno = ENOTDIR;
    }
    else if ( !isdir && rm_ftype == INODE_DIR )
    {
        errno = EISDIR;
    }
    else
        it[trid]->destroy( fileid, rm_ftype );

 out:
    res = nfs_error();
#undef  res
#undef  arg
}

void
nfssrv :: remove_2( void )
{
    _remove2( false );
}

void
nfssrv :: rmdir_2( void )
{
#define arg argument.rmdir_2_arg
#define res replies.nfsstat_result
    if ( strcmp( (char*)arg.name, "." ) == 0 ||
         strcmp( (char*)arg.name, ".." ) == 0 )
    {
        res = NFSERR_PERM;
        return;
    }

    _remove2( true );
#undef  res
#undef  arg
}

void
nfssrv :: link_2(void)
{
#define arg argument.link_2_arg
#define res replies.nfsstat_result
    Inode * f = NULL;
    int dirid, fileid, trid = -1;

    if ( !FHDEC( trid, &arg.from ))
        goto out;
    fileid = handle.file_id.get();
    if ( !FHDEC( trid, &arg.to.dir ))
        goto out;
    dirid = handle.file_id.get();

    // create a link in dirid/arg.to.name which points to fileid.

    f = it[trid]->createhlink( dirid, arg.to.name, fileid );

    if ( f != NULL )
        it[trid]->deref( f );

 out:
    res = nfs_error();
#undef  res
#undef  arg
}

void
nfssrv :: symlink_2(void)
{
#define arg argument.symlink_2_arg
#define res replies.nfsstat_result
    Inode * f = NULL;
    int dirid, trid = -1, e;

    if ( !FHDEC( trid, &arg.from.dir ))
        goto out;

    dirid = handle.file_id.get();
    f = it[trid]->createslink( dirid, arg.from.name, arg.to );
    if ( f == NULL )
        goto out;

    e = errno;
    f->isattr( arg.attributes );
    errno = e;

    it[trid]->deref( f );
 out:
    res = nfs_error();
#undef  res
#undef  arg
}

void
nfssrv :: readlink_2(void)
{
#define arg argument.readlink_2_arg
#define res replies.readlinkres_result
    Inode * i = NULL;
    int trid;

    if ( !FHDEC( trid, &arg ))
        goto out;
    if ( !GETI( trid, handle, i ))
        goto out;

    if ( i->readlink( readbuf ) < 0 )
        goto out2;

    errno = 0;
    res.readlinkres_u.data = readbuf;

 out2:
    it[trid]->deref( i );
 out:
    res.status = nfs_error();
#undef  res
#undef  arg
}

// below this line is stuff that isn't used by nfs version 2

void nfssrv :: null_2(void) { }
void nfssrv :: root_2(void) { }
void nfssrv :: wrcache_2(void) { }
