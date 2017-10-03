
#ifndef __FILE_BLOCK_H__
#define __FILE_BLOCK_H__

#include "types.H"

// a block id is a UINT32. 0 is invalid.

class FileBlockCookie {
// nothing
};

class FileBlockInterface {
public:
    virtual ~FileBlockInterface(void) { /* placeholder */ }

    // return a block id. 0 means an error
    virtual UINT32 alloc( int size ) = 0;
    virtual void free( UINT32 block ) = 0;

    // retrieve an information block from the file
    // identified by some unique name.
    virtual UINT32 get_data_info_block( char *info_name ) = 0;
    virtual void   set_data_info_block( UINT32 block, char *info_name ) = 0;
    virtual void   del_data_info_block( char * info_name ) = 0;

    // utility function; other methods below are
    // frontends to this one.  get a block, return the
    // size and cookie.
    virtual UCHAR * _get_block( UINT32 block, int *size,
                                FileBlockCookie **cookie, bool for_write ) = 0;
    virtual void unlock_block( FileBlockCookie * cookie ) = 0;
    virtual void flush(void) = 0;

    UCHAR * get_block( UINT32 block, int * size, FileBlockCookie ** cookie ) {
        return _get_block( block, size, cookie, false );
    }
    UCHAR * get_block( UINT32 block, FileBlockCookie ** cookie ) {
        return _get_block( block, NULL, cookie, false );
    }
    UCHAR * get_block_for_write( UINT32 block, int * size,
                                 FileBlockCookie ** cookie ) {
        return _get_block( block, size, cookie, true );
    }
    UCHAR * get_block_for_write( UINT32 block, FileBlockCookie ** cookie ) {
        return _get_block( block, NULL, cookie, true );
    }
};

#endif /* __FILE_BLOCK_H__ */
