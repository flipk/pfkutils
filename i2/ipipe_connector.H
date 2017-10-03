/*
    This file is part of the "pfkutils" tools written by Phil Knaack
    (pknaack1@netscape.net).
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

#ifndef __ipipe_connector_H__
#define __ipipe_connector_H__

#include "fd_mgr.H"

#include "ipipe_factories.H"

// generic connector; calls the factory when a new connection is successful.
class ipipe_connector : public fd_interface {
    ipipe_new_connection * connection_factory;
public:
    ipipe_connector( struct sockaddr_in *, ipipe_new_connection * );

    /*virtual*/ ~ipipe_connector( void );
    /*virtual*/ rw_response write( fd_mgr * );
    /*virtual*/ void select_rw ( fd_mgr *, bool * rd, bool * wr );

    /* note virtual functions read, write_to_buf, and over_write_threshold
       are not implemented here. */
};

#endif /* __ipipe_connector_H__ */
