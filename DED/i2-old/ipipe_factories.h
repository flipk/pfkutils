/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */


#ifndef __ipipe_factories_H__
#define __ipipe_factories_H__

#include "fd_mgr.h"
#include "ipipe_rollover.h"

// abstract interface describing what acceptor should do when a 
// new connection is received.
class ipipe_new_connection {
public:
    virtual ~ipipe_new_connection( void ) { /* placeholder */ }

    enum new_conn_response { CONN_CONTINUE, CONN_DONE };
    virtual new_conn_response new_conn( fd_mgr *, int new_fd ) = 0;
};


// concrete factory for a data forwarder connection.
class ipipe_forwarder_factory : public ipipe_new_connection {
    bool dowuncomp;
    bool dowcomp;
    ipipe_rollover * rollover;
    bool outdisc;
    bool inrand;
    int pausing_bytes;
    int pausing_delay;
public:
    ipipe_forwarder_factory( bool dowuncomp, bool dowcomp,
                             ipipe_rollover * _rollover,
                             bool _outdisc, bool _inrand,
                             int pause_bytes, int pause_delay );
    virtual ~ipipe_forwarder_factory( void ) { /* nothing */ }
    // return false if the acceptor should die off
    virtual new_conn_response new_conn( fd_mgr *, int new_fd );
};

// concrete factory for a proxy connection
class ipipe_proxy_factory : public ipipe_new_connection {
    struct sockaddr_in * sa;
    int pausing_bytes;
    int pausing_delay;
public:
    ipipe_proxy_factory( struct sockaddr_in *,
                         int pause_bytes, int pause_delay );
    virtual ~ipipe_proxy_factory( void );
    // return false if the acceptor should die off
    virtual new_conn_response new_conn( fd_mgr *, int new_fd );
};

// concrete factory for a proxy connection
class ipipe_proxy2_factory : public ipipe_new_connection {
    int fda;
    int pausing_bytes;
    int pausing_delay;
public:
    ipipe_proxy2_factory( int fda,
                          int pause_bytes, int pause_delay );
    virtual ~ipipe_proxy2_factory( void ) { /* nothing */ }
    // return value irrelevant to connector
    virtual new_conn_response new_conn( fd_mgr *, int new_fd );
};

#endif /* __ipipe_factories_H__ */
