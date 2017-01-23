/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
 */

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
