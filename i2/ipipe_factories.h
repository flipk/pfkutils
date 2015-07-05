/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

/*
    This file is part of the "pfkutils" tools written by Phil Knaack
    (pfk@pfk.org).
    Copyright (C) 2008  Phillip F Knaack

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
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
