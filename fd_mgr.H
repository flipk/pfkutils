
#ifndef __FD_MGR_H__
#define __FD_MGR_H__

#define DLL2_INCLUDE_LOGNEW 0
#define DLL2_CHECKSUMS      1
#define DLL2_INCLUDE_BTREE  0
#include "dll2.H"

class fd_mgr;

class fd_interface {
public:
    LListLinks <fd_interface> links[ 1 ];
    fd_interface( void ) { do_close = false; }
    virtual ~fd_interface( void ) { /* placeholder */ }

    int fd;
    bool do_close;

    // true means it was ok, false means delete this object.
    virtual bool read ( fd_mgr * ) = 0;
    virtual bool write( fd_mgr * ) = 0;

    // true means select, false means dont
    virtual bool select_for_read( fd_mgr * ) = 0;
    virtual bool select_for_write( fd_mgr * ) = 0;

    // true means low on write buffer space, should probably
    // try to quench your source if you need to write to this fd.
    virtual bool over_write_threshold( void ) = 0;

    // return false if it was not ok
    virtual bool write_to_fd( char * buf, int len ) = 0;
};

class fd_mgr {
    LList <fd_interface,0>  ifds;
public:
    void register_fd( fd_interface * ifd ) { ifds.add( ifd ); }
    void unregister_fd( fd_interface * ifd ) { ifds.remove( ifd ); }
    void loop( void );
};

#endif /* __FD_MGR_H__ */
