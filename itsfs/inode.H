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

#ifndef __INODE_H_
#define __INODE_H_

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "ino_types.h"
#include "nfs_prot.h"  // for fattr and sattr
#include "id_name_db.H"

extern id_name_db * inode_name_db;

class Inode;

class Inode_tree {
public:
    // this value describes the maximum length of a file path;
    // the value chosen is mostly arbitrary.
    static const int MAX_PATH_LENGTH = 750;

    // this is the inode id of the "root" directory of this tree.
    // it is dynamically allocated when a tree is created.
    int     ROOT_INODE_ID;

    // a "tree" identifier is a small positive integer, which is
    // an index into a table within the NFS server.  tree ids are
    // encoded into NFS file handles for quicker location of files.
    int     tree_id;

    // a mount id is a 32 bit random number which is included in 
    // NFS file handles.  this is to prevent cache problems with
    // NFS file handles.  a tree identifier may be quickly removed, 
    // and reused, but the mount_id will change, thus forcing clients
    // to invalidate caches of old handles.
    u_int   mount_id;

    // describes the derived class type.
    inode_type type;

    Inode_tree( inode_type _type, int _tree_id )
        {
            mount_id = random();
            type = _type;
            tree_id = _tree_id;
            ROOT_INODE_ID = inode_name_db->alloc_id();
            // responsibility of each derived class!
            // inode_name_db->add( mount_id, (uchar*)".",
            //                     true, ROOT_INODE_ID );
        }

    virtual ~Inode_tree( void )
        {
            inode_name_db->purge_mount( mount_id );
        };

    virtual Inode *   get         ( int file_id )                         = 0;
    virtual Inode *   get         ( int dir_id, uchar * filename )        = 0;
    virtual Inode *   get_parent  ( int file_id )                         = 0;
    virtual int       deref       ( Inode * )                             = 0;

    virtual Inode *   create      ( int dirid,
                                    uchar * name, inode_file_type )       = 0;
    virtual Inode *   createslink ( int dirid,
                                    uchar * name, uchar * target )        = 0;
    virtual Inode *   createhlink ( int dirid,
                                    uchar * name, int fileid )            = 0;

    virtual int       destroy     ( int fileid, inode_file_type )         = 0;
    virtual int       rename      ( int olddir, uchar * oldfile,
                                    int newdir, uchar * newfile )         = 0;

    virtual void      clean       ( void )                                = 0;
    virtual void      print_cache ( int arg, void (*printfunc)
                                    (int arg, char *format,...) )         = 0;
    virtual bool      valid       ( void )                                = 0;

};

class Inode {
    int  refcount;
    time_t last_ref;
public:
    inode_type type;
    inode_file_type ftype;
    int  file_id;
    int  tree_id;

    Inode( inode_type _type, int _file_id, int _tree_id, inode_file_type ft )
        {
            ftype = ft;
            file_id = _file_id;
            tree_id = _tree_id;
            refcount = 1;
            type = _type;
            last_ref = time( NULL );
        }
    virtual ~Inode( void )
        {
            if ( refcount != 0 )
                printf( "Inode destructor ERROR! "
                        "Inode %d is still ref'd!\n", file_id );
        }
    int lastref( void )
        { return time( NULL ) - last_ref; }
    int refget( void )
        { return refcount; }

    // really, ref() should only be called by the tree object, such
    // as during a get() method.
    int ref( void )
        {
            last_ref = time( NULL );
            return ++refcount;
        }

    // deref() should never be called except by the deref() method
    // of the tree object.
    int deref( void )
        {
            if ( refcount == 0 )
            {
                printf( "Inode::deref() : refcount already zero!\n" );
                return 0;
            }
            return --refcount;
        }

    // all inodes can do the following.

    virtual int       isattr      ( sattr & )                            = 0;
    virtual int       ifattr      ( fattr & )                            = 0;

    // only files do the following.

    virtual int       iread       ( int offset, uchar * buf, int &size ) = 0;
    virtual int       iwrite      ( int offset, uchar * buf, int size )  = 0;

    // only directories do the following.

    virtual int       setdirpos   ( int cookie )                         = 0;
    virtual int       readdir     ( int &fileno, uchar * filename )      = 0;

    // only symlinks do the following.

    virtual int       readlink    ( uchar * buf )                        = 0;
};


#endif /* __INODE_H_ */
