
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

#include "types.H"
#include "dll2.H"
#include "btree.H"
#include "pk-md5.h"

#include <string.h>
#include <stdlib.h>

static inline
char *
mystrdup( char * str )
{
    int len = strlen(str) + 1;
    char * ret = new char[len];
    memcpy( ret, str, len );
    return ret;
}

static inline
char *
mystrndup( char * str, int len )
{
    len ++;
    char * ret = new char[len];
    memcpy( ret, str, len-1 );
    ret[len-1] = 0;
    return ret;
}

struct file_info {
    LListLinks <file_info>  links[1];
    char   * fname;
    UINT32   id;
    UINT64   size;
    time_t   mtime;
    UINT32   uid;
    UINT32   gid;
    UINT16   mode;
private:
// used internally by file_db
    friend class file_db;
    UINT32   mark; // random walking mark
    OBtree::rec * rec;
    OBtree * bt;
public:
    file_info( char * _fn ) {
        rec = NULL;
        fname = mystrdup( _fn );
    }
    file_info( char * _fn, int _fnlen ) {
        rec = NULL;
        fname = mystrndup( _fn, _fnlen );
    }
    ~file_info( void ) {
        delete[] fname;
        if ( rec )
            bt->unlock_rec( rec );
    }
};
typedef LList <file_info,0> file_info_list;

class file_db_iterator {
public:
    virtual ~file_db_iterator(void) { /* placeholder */ }
    // NOTE THAT FILE_INFO STRUCTURES PASSED TO THIS FUNCTION
    // ARE NOT FREED WHEN CALL IS DONE, IT IS UP TO THIS FUNCTION
    // TO FREE IT IF IT DOESN'T WANT TO KEEP IT
    virtual void file( file_info * f ) = 0;
};

class file_db {
    FileBlockNumber * fbn_nodes;
    FileBlockNumber * fbn_keys;
    FileBlockNumber * fbn_data;
    FileBlockNumber * fbn_contents;
    OBtree * bt;
    UINT32 current_mark;
    file_info_list  update_list;
    file_info_list  add_list;
    friend class file_db_iterate_pi;
    void write_signature( struct mytar_sig * );
    file_info * _get_info_from_block( UINT32 id, UCHAR * data_block, int len );
public:
    file_db( char * fname, bool create_it, bool validate_sig );
    ~file_db( void );

    static const int PIECE_SIZE = 1024 * 50;

    // do something for every file in the database.
    void iterate( file_db_iterator * );

    // these 2 return null if not found;
    // when done with the retval, delete it,
    // or call update.
    file_info * get_info_by_id( UINT32 id );
    file_info * get_info_by_fname( char * fname );

    // this will update the db and also delete the arg.
    // info update: size, mtime, uid, gid, mark, mode
    void update_info( file_info * );

    // arg must be new'd.  this function will add to the db and
    // then delete.  will also return the new id number allocated
    // for this file.
    // note that info->id and info->mark should not be populated--
    // they are populated by this function.
    UINT32 add_info( file_info * );

    // delete everything from this file not
    // bearing the current mark
    void delete_old( void );

    // this deletes any excess datum 3's that may be present.
    // use num_pieces=='0' if a file is being deleted.
    void truncate_pieces( UINT32 id, UINT32 num_pieces );
    // this will update or create a datum 3. returns true if 
    // the piece was modified or false if it was unchanged.
    bool update_piece( UINT32 id, UINT32 piece_num,
                       char * buf, int buflen );
    void extract_piece( UINT32 id, UINT32 piece_num,
                        char * buf, int * buflen );
};

#ifdef FILE_DB_INTERNAL

// datum 0
//    key = "MYTARSIGVERSION"
//    data = version

struct datum_0_data {
    UINT32_t  version;
};

#define MYTAR_SIGKEY "MYTARSIGVERSION"
#define MYTAR_SIGFILE "0MTARSIG.BIN"
#define MYTAR_SIG_MAGIC 0x8c7d5b3dUL

struct mytar_sig {
    UINT32_t   magic;
    UINT32_t   version;
    UINT32_t   checksum;
    void init(void) {
        magic.set( MYTAR_SIG_MAGIC );
        version.set( random() );
        checksum.set( magic.get() + version.get() );
    }
    bool valid(void) {
        if (magic.get() != MYTAR_SIG_MAGIC)
            return false;
        return checksum.get() == (magic.get() + version.get());
    }
};

// datum 1
//    key = "n" plus ascii filename with no nul
//    data = id

struct datum_1_key {
    char   prefix_n;
    char   fname[ 0 ];
};

struct datum_1 {
    UINT32_t   id;
};

// datum 2
//    key = "i" plus id
//    data = size[8], mtime, uid, gid, mark, mode, fname with no nul

struct datum_2_key {
    char       prefix_i;
    UINT32_t   id;
};

struct datum_2 {
    UINT64_t   size;
    UINT32_t   mtime;
    UINT32_t   uid;
    UINT32_t   gid;
    UINT32_t   mark; // random walking mark
    UINT16_t   mode;
    char       fname[0];
};

// datum 3
//    key = "d" plus id plus piece#
//    data = piecesize[2] + recno_block_num + md5sum

struct datum_3_key {
    char      prefix_d;
    UINT32_t  id;
    UINT32_t  piece_num;
};

struct datum_3 {
    UINT16_t    size; // max size is PIECE_SIZE
    UINT32_t    blockno;
    MD5_DIGEST  digest;
};

#endif
