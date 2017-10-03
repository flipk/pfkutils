
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
