
#ifndef __ipipe_acceptor_H__
#define __ipipe_acceptor_H__

#include "fd_mgr.H"

#include "ipipe_factories.H"

// generic acceptor; calls the factory when a new connection is received.
class ipipe_acceptor : public fd_interface {
    ipipe_new_connection * connection_factory;
public:
    ipipe_acceptor( short port, ipipe_new_connection * );

    /*virtual*/ ~ipipe_acceptor( void );
    /*virtual*/ rw_response read ( fd_mgr * );
    /*virtual*/ rw_response write( fd_mgr * );
    /*virtual*/ bool select_for_read ( fd_mgr * );
    /*virtual*/ bool select_for_write( fd_mgr * );
};

#endif /* __ipipe_acceptor_H__ */
