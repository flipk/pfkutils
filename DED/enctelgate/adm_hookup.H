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

#ifndef __ADM_HOOKUP_H__
#define __ADM_HOOKUP_H__

#include "fd_mgr.H"

class Adm_Hookup_Factory_iface {
public:
    virtual ~Adm_Hookup_Factory_iface( void ) { /* nothing */ }
    virtual void new_gateway( int fd_ear, int fd_outfd,
                              fd_mgr * fdmgr ) = 0;
};

class Adm_Hookup_fd : public fd_interface {
    Adm_Hookup_Factory_iface * factory;
    struct sockaddr_in * dest_sa;
public:
    Adm_Hookup_fd( Adm_Hookup_Factory_iface *,
                   short port, char * dest_host, short dest_port );

    /*virtual*/ ~Adm_Hookup_fd( void );
    /*virtual*/ rw_response read ( fd_mgr * );
    /*virtual*/ void select_rw ( fd_mgr *, bool * rd, bool * wr );

    // note write, over_write_threshold, and write_to_fd not implemented
};

#endif /* __ADM_HOOKUP_H__ */
