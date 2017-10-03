
#ifndef __FILE_BLOCK_LOCAL_H__
#define __FILE_BLOCK_LOCAL_H__

#include "FileBlock_iface.H"
#include "dll2.H"

class FileBlockLocal : public FileBlockInterface {
// xxx
    BlockCache * bc;
public:
    FileBlockLocal( BlockCache * _bc );
    ~FileBlockLocal( void );

    // xxx file info junk goes here eventually.

    /*virtual*/ UINT32 alloc( int size );
    /*virtual*/ void free( UINT32 id );
    /*virtual*/ FileBlock * get_block( UINT32 id, bool for_write = false );
    /*virtual*/ void unlock_block( FileBlock * blk );
    /*virtual*/ void flush(void);
    /*virtual*/ void compact(int time_limit);
};

#endif /* __FILE_BLOCK_LOCAL_H__ */
