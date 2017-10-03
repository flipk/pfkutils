
#ifndef __FD_MGR_H__
#define __FD_MGR_H__

#define DLL2_INCLUDE_LOGNEW 0
#define DLL2_CHECKSUMS      1
#define DLL2_INCLUDE_BTREE  0
#include "dll2.H"

#include <stdio.h>

class fd_mgr;

class fd_interface {
public:
    LListLinks <fd_interface> links[ 1 ];
    fd_interface( void ) { do_close = false; }
    virtual ~fd_interface( void ) { /* placeholder */ }

    int fd;
    bool do_close;

    enum rw_response {
        OK,   // the read or write was normal
        DEL   // the fd should be deleted
    };

    // true means it was ok, false means delete this object.
    virtual rw_response read ( fd_mgr * ) = 0;
    virtual rw_response write( fd_mgr * ) = 0;

    // true means select, false means dont
    virtual bool select_for_read ( fd_mgr * ) = 0;
    virtual bool select_for_write( fd_mgr * ) = 0;
};

class fd_mgr {
    LList <fd_interface,0>  ifds;
    bool debug;
    int  die_threshold;
public:
    fd_mgr( bool _debug, int _die_threshold = 0 ) {
        debug = _debug; die_threshold = _die_threshold;
    }
    void register_fd( fd_interface * ifd ) {
        if ( debug )
            fprintf( stderr, "registering fd %d\n", ifd->fd );
        ifds.add( ifd );
    }
    void unregister_fd( fd_interface * ifd ) {
        if ( debug )
            fprintf( stderr, "unregistering fd %d\n", ifd->fd );
        ifds.remove( ifd );
    }
    void loop( void );
};

#endif /* __FD_MGR_H__ */
