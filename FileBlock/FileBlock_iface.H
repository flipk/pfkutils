
#ifndef __FILE_BLOCK_H__
#define __FILE_BLOCK_H__

#include "BlockCache.H"
#include "types.H"

// a block id is a UINT32. 0 is invalid.

class FileBlock {
    UINT32 id;
    BlockCacheBlock * bcb;
    FileBlock(void) {
        // user is now allowed to create these.
    }
    virtual ~FileBlock(void) {
        // user is not allowed to delete them either.
    }
    friend class FileBlockInterface;
public:
    UINT32  get_id    (void) { return id; }
    off_t   get_offset(void) { return bcb->get_offset(); }
    int     get_size  (void) { return bcb->get_size  (); }
    UCHAR * get_ptr   (void) { return bcb->get_ptr   (); }
    void    mark_dirty(void) { bcb->mark_dirty(); }
};

class FileBlockInterface {
public:
    virtual ~FileBlockInterface(void) { /* placeholder */ }

    // return a block id. 0 means an error
    virtual UINT32 alloc( int size ) = 0;
    virtual void   free( UINT32 id ) = 0;

    // retrieve an information block from the file
    // identified by some unique name.
// needs work
//    virtual UINT32 get_data_info_block( char * info_name ) = 0;
//    virtual void   set_data_info_block( UINT32 id, char *info_name ) = 0;
//    virtual void   del_data_info_block( char * info_name ) = 0;

    virtual FileBlock * get_block( UINT32 id, bool for_write = false ) = 0;
    virtual void unlock_block( FileBlock * blk ) = 0;
    virtual void flush(void) = 0;

    // compact the file. time_limit is the maximum number
    // of seconds it is allowed to spend trying; when this
    // amount of time passes, compact should give up and return.
    // 0 means continue until the file is just about contiguous.
    virtual void compact(int time_limit) = 0;
};

#endif /* __FILE_BLOCK_H__ */
