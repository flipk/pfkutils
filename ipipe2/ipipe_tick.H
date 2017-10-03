
#ifndef __ipipe_tick_H__
#define __ipipe_tick_H__

#include "fd_mgr.H"

class tick_fd : public fd_interface {
    void ( * func )( void * );
    void * arg;
public:
    tick_fd( int tenths );
    void register_tick( void (*)(void*), void * );
    /*virtual*/ ~tick_fd( void );
    /*virtual*/ rw_response read ( fd_mgr * );
    /*virtual*/ rw_response write( fd_mgr * );
    /*virtual*/ bool select_for_read ( fd_mgr * );
    /*virtual*/ bool select_for_write( fd_mgr * );
};

#endif /* __ipipe_tick_H__ */
