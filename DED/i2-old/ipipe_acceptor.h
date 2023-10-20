/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */


#ifndef __ipipe_acceptor_H__
#define __ipipe_acceptor_H__

#include "fd_mgr.h"

#include "ipipe_factories.h"

// generic acceptor; calls the factory when a new connection is received.
class ipipe_acceptor : public fd_interface {
    ipipe_new_connection * connection_factory;
public:
    ipipe_acceptor( short port, ipipe_new_connection * );

    /*virtual*/ ~ipipe_acceptor( void );
    /*virtual*/ rw_response read ( fd_mgr * );
    /*virtual*/ void select_rw ( fd_mgr *, bool * rd, bool * wr );

    /* note virtual functions write, write_to_fd, and over_write_threshold
       not implemented. */
};

#endif /* __ipipe_acceptor_H__ */
