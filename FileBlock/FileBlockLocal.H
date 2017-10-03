
#ifndef __FILE_BLOCK_LOCAL_H__
#define __FILE_BLOCK_LOCAL_H__

#include "FileBlock_iface.H"
#include "dll2.H"

class FileBlockLocalCache;

class FileBlockLocal : public FileBlockInterface {
    int fd;
    FileBlockLocalCache * cache;
public:
    FileBlockLocal( char * fname, bool create );
    ~FileBlockLocal( void );

    // return a block id. 0 means an error
    /*virtual*/ UINT32 alloc( int size );
    /*virtual*/ void free( UINT32 block );

    // retrieve an information block from the file
    // identified by some unique name.
    /*virtual*/ UINT32 get_data_info_block( char *info_name );
    /*virtual*/ void   set_data_info_block( UINT32 block, char *info_name );
    /*virtual*/ void   del_data_info_block( char * info_name );

    // utility function; other methods below are
    // frontends to this one.  get a block, return the
    // size and cookie.
    /*virtual*/ UCHAR * _get_block( UINT32 block, int *size,
                                    FileBlockCookie **cookie, bool for_write );
    /*virtual*/ void unlock_block( FileBlockCookie * cookie );
    /*virtual*/ void flush(void);

};

#endif /* __FILE_BLOCK_LOCAL_H__ */
