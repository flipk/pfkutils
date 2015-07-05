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
