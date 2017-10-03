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

#ifndef __NFSSRV_H_
#define __NFSSRV_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// the entry point to this object is the "dispatch" 
// function.  "dispatch" is called whenever a packet has
// arrived on the UDP port for this server.

#include "inode.H"
#include "nfs_prot.h"
#ifdef USE_CRYPT
#include "encrypt_iface.H"
#else
class encrypt_iface;
#endif
#include "filehandle.H"

class nfssrv {
private:
    static const int MAX_TREES = 256;
    Inode_tree * it[ MAX_TREES ];
    bool itreserved[ MAX_TREES ];
    static enum nfsstat nfs_error( void );

    encrypt_iface * crypt;
    FileHandle handle;

// the following is a declaration of some memory to hold any
// datatype needed as an argument for a procedure. any pointers
// within these end up getting dynamically allocated when the
// xdr decode is performed on the incoming rpc packet. when the
// particular procedure is complete, the xdr function is then used
// to free any memory allocated during decode.

    union {
        nfs_fh       getattr_2_arg;
        sattrargs    setattr_2_arg;
        diropargs    lookup_2_arg;
        nfs_fh       readlink_2_arg;
        readargs     read_2_arg;
        writeargs    write_2_arg;
        createargs   create_2_arg;
        diropargs    remove_2_arg;
        renameargs   rename_2_arg;
        linkargs     link_2_arg;
        symlinkargs  symlink_2_arg;
        createargs   mkdir_2_arg;
        diropargs    rmdir_2_arg;
        readdirargs  readdir_2_arg;
        nfs_fh       statfs_2_arg;
    } argument;

// the following are static declarations of variables used 
// in the return of each of the implemented nfs version 2 procedures.
// they cannot be dynamic per call because the semantics/structure of
// the rpcsvc framework are that return values are not freed after
// the reply has been sent.

    union {
        attrstat    attrstat_result;
        diropres    diropres_result;
        readres     readres_result;
        readdirres  readdirres_result;
        nfsstat     nfsstat_result;
        readlinkres readlinkres_result;
        statfsres   statfsres_result;
    } replies;

// these are data structures which fill in pointers in the above
// reply structures (so that we don't have to worry about freeing
// memory when the procedure is complete).  'readbuf' is used by the
// read_2 procedure (in the readres result) and 'entries' and 'filenames'
// are used by the readdir_2 procedure (in the readdirres result).
// we can do this cuz of course nfssrv must be accessed in a
// single-threaded manner anyway.

    // yikes, these chew a lot of space....
    uchar   replybuffer[9000];
    uchar   readbuf[8192];
    entry   entries[1024];
    uchar   filenames[1024][ Inode_tree::MAX_PATH_LENGTH ];
    int     transaction_count;

// internal helpers

    bool FHDEC( int &trid, nfs_fh *fh );
    bool GETI( int trid, FileHandle &handle, Inode * &ino );

// the following are the implemented procedures of nfs version 2.

    void    getattr_2    ( void );
    void    setattr_2    ( void );

    void    lookup_2     ( void );

    void    read_2       ( void );
    void    write_2      ( void );
    void    readdir_2    ( void );

    void    mkdir_2      ( void );
    void    create_2     ( void );

    void    rmdir_2      ( void );
    void    remove_2     ( void );

    void    rename_2     ( void );
    void    statfs_2     ( void );

    void    link_2       ( void );
    void    symlink_2    ( void );
    void    readlink_2   ( void );

// these three are actually not used by the nfsv2 protocol.
// these are depracated throwbacks from nfsv1.

    void    null_2       ( void );
    void    root_2       ( void );
    void    wrcache_2    ( void );

// these following 2 are actually not nfsv2 procedures, but
// the implementation for mkdir/rmdir/create/remove.

    void    _create2     ( inode_file_type );
    void    _remove2     ( bool isdir );

public:

    nfssrv( encrypt_iface * _c )
        {
            for ( int i = 0; i < MAX_TREES; i++ )
            {
                it[i] = NULL;
                itreserved[i] = false;
            }
            transaction_count = 0;
            crypt = _c;
        };

    ~nfssrv( void )
        {
            for ( int i = 0 ; i < MAX_TREES; i++ )
                if ( it[i] )
                    delete it[i];
#ifdef USE_CRYPT
            if ( crypt )
                delete crypt;
#endif
        }

    int reserve_treeid( void )
        {
            int i;
            for ( i = 0; i < MAX_TREES; i++ )
                if ( it[i] == NULL && !itreserved[i] )
                    break;
            if ( i == MAX_TREES )
                return -1;
            itreserved[i] = true;
            return i;
        }

    int register_tree( int treeid, Inode_tree * nit )
        {
            if ( it[treeid] != NULL )
                return -1;
            if ( !itreserved[treeid] )
                return -1;
            it[treeid] = nit;
            itreserved[treeid] = false;
            return treeid;
        }

    void unregister_tree( int trid )
        {
            if ( it[trid] )
                delete it[trid];
            it[trid] = NULL;
            itreserved[trid] = false;
        }

    uchar * dispatch( uchar * buf, int bufsize, int &replysize );

    int get_transaction_count(void)
        { return transaction_count; }

    void clean(void)
        {
            for ( int i = 0; i < MAX_TREES; i++ )
                if ( it[i] )
                    it[i]->clean();
        }

};

#endif /* __NFSSRV_H_ */
