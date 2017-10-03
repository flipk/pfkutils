
#ifndef __FD_MGR_H__
#define __FD_MGR_H__

#define DLL2_INCLUDE_LOGNEW 0
#define DLL2_CHECKSUMS      1
#define DLL2_INCLUDE_BTREE  0
#include "dll2/dll2.H"

#include <stdio.h>

class fd_mgr;

class fd_interface {
protected:
    void make_nonblocking( void );
    int fd;
    bool do_close;
public:
    LListLinks <fd_interface> links[ 1 ];
    fd_interface( void ) { do_close = false; }
    virtual ~fd_interface( void ) {
        // placeholder : note that fd is NOT closed here, because
        //  perhaps the derived class wants to do something with it.
        //  if not, all derived class destructors must close( fd ).
    }

    enum rw_response {
        OK,   // the read or write was normal
        DEL   // the fd should be deleted
    };

    // if a fd doesn't need read or write, don't implement
    // them in the class; just allow the defaults here to be used.

    virtual rw_response read ( fd_mgr * ) { return DEL; }
    virtual rw_response write( fd_mgr * ) { return DEL; }

    // true means select, false means dont;
    //   note:   never return true if you've set do_close !!
    virtual void select_rw ( fd_mgr *, bool * do_read, bool * do_write ) = 0;

    // provided for convenience; if these are not used,
    // just use the defaults here.
    virtual bool write_to_fd( char * buf, int len ) { return false; }
    virtual bool over_write_threshold( void ) { return false; }

    friend class fd_mgr;
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
