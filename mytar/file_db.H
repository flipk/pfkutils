
#include "types.H"
#include "dll2.H"
#include "btree.H"
#include "pk-md5.h"

#include <string.h>

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
    UINT64   size;
    time_t   mtime;
    UINT32   uid;
    UINT32   gid;
    UINT16   mode;
private:
// used internally by file_db
    friend class file_db;
    UINT32   id;
    UINT32   mark; // random walking mark
    Btree::rec * rec;
    Btree * bt;
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

class file_db {
    FileBlockNumber * fbn;
    Btree * bt;
    UINT32 current_mark;
    file_info_list  update_list;
    file_info_list  add_list;
    void sync_blocklist( file_info * );
public:
    file_db( char * fname, bool create_it );
    ~file_db( void );

    static const int BLOCK_SIZE = 1024 * 12;
    // calculate how many total blocks will be required
    // to store this file, and also calculate how
    // many bytes are in the final block.
    static void calc_blocks( UINT64 file_size,
                             UINT32 * num_blocks,
                             UINT32 * last_block_size );

    // these 2 return null if not found;
    // when done with the retval, delete it,
    // or call update.
    file_info * get_info_by_id( UINT32 id );
    file_info * get_info_by_fname( char * fname );

    // this will update the db and also delete the arg.
    // will also check for rogue datums 3+4 to clean
    // info update: size, mtime, uid, gid, mark, mode
    void update_info( file_info * );

    // arg must be new'd.  this function will NOT delete when done--
    // the user must do that.
    // note that info->id and info->mark should not be populated--
    // they are populated by this function.
    void add_info( file_info * );

    // delete everything from this file not
    // bearing the current mark
    void delete_old( void );

    void update_block( file_info *, UINT32 block_num,
                       char * buf, int buflen );
    void extract_block( file_info *, UINT32 block_num,
                        char * buf, int * buflen );
};

#ifdef FILE_DB_INTERNAL

// datum 1
//    key = "n" plus ascii filename with no nul
//    data = id
// datum 2
//    key = "i" plus id
//    data = size[8], mtime, uid, gid, mark, mode, fname with no nul
// datum 3
//    key = "d" plus id plus block#
//    data = blocksize[2] + recno_block_num + md5sum


struct datum_2 {
    UINT64_t   size;
    UINT32_t   mtime;
    UINT32_t   uid;
    UINT32_t   gid;
    UINT32_t   mark; // random walking mark
    UINT16_t   mode;
    char       fname[0];
};

struct datum_3 {
    UINT16_t    bsize; // max size is BLOCK_SIZE
    UINT32_t    blockno;
    MD5_DIGEST  digest;
};

#endif
