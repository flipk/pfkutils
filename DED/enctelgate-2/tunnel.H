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

#ifndef __TUNNEL_H__
#define __TUNNEL_H__

#include "fd_mgr.H"
#include "adm_gate.H"

class Tunnel_fd : public fd_interface {
    Adm_Gate_fd * encoder_fd;
    char * device;
    char * devshort;
    char * my_address;
    char * other_address;
    char * netmask;
public:
    Tunnel_fd( char * my_address,  char * other_address, char * netmask );
    /*virtual*/ ~Tunnel_fd( void );
    /*virtual*/ rw_response read ( fd_mgr * );
    /*virtual*/ void select_rw ( fd_mgr *, bool * rd, bool * wr );
    /*virtual*/ bool write_to_fd( char * buf, int len );

    /*       */ void register_encoder_fd( Adm_Gate_fd * encoder_fd );
    /*       */ void unregister_encoder_fd( void );

    // over_write_threshold and write not implemented
};

#endif /* __TUNNEL_H__ */
