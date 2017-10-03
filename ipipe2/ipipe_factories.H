/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

#ifndef __ipipe_factories_H__
#define __ipipe_factories_H__

#include "fd_mgr.H"
#include "ipipe_rollover.H"

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
public:
    ipipe_forwarder_factory( bool dowuncomp, bool dowcomp,
                             ipipe_rollover * _rollover,
                             bool _outdisc, bool _inrand );
    virtual ~ipipe_forwarder_factory( void ) { /* nothing */ }
    // return false if the acceptor should die off
    virtual new_conn_response new_conn( fd_mgr *, int new_fd );
};

// concrete factory for a proxy connection
class ipipe_proxy_factory : public ipipe_new_connection {
    struct sockaddr_in * sa;
public:
    ipipe_proxy_factory( struct sockaddr_in * );
    virtual ~ipipe_proxy_factory( void );
    // return false if the acceptor should die off
    virtual new_conn_response new_conn( fd_mgr *, int new_fd );
};

// concrete factory for a proxy connection
class ipipe_proxy2_factory : public ipipe_new_connection {
    int fda;
public:
    ipipe_proxy2_factory( int fda );
    virtual ~ipipe_proxy2_factory( void ) { /* nothing */ }
    // return value irrelevant to connector
    virtual new_conn_response new_conn( fd_mgr *, int new_fd );
};

#endif /* __ipipe_factories_H__ */
