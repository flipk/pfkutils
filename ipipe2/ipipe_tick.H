
#ifndef __ipipe_tick_H__
#define __ipipe_tick_H__

#include "fd_mgr.H"

class tick_fd : public fd_interface {
    void ( * func )( void * );
    void * arg;
public:
    tick_fd( int tenths, void (*)(void*), void * );
    /*virtual*/ ~tick_fd( void );
    /*virtual*/ rw_response read ( fd_mgr * );
    /*virtual*/ void select_rw ( fd_mgr *, bool * rd, bool * wr );

    /* note virtual functions write, write_to_fd, and over_write_threshold
       are not implemented here. */
};

#endif /* __ipipe_tick_H__ */
