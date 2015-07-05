/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

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

#ifndef __ipipe_forwarder_H__
#define __ipipe_forwarder_H__

#ifdef I2_ZLIB
#include <zlib.h>
#endif

#include "fd_mgr.h"
#include "circular_buffer.h"
#include "ipipe_rollover.h"

// each half of a data forwarder 
class ipipe_forwarder : public fd_interface {
    ipipe_forwarder * writer; /* data read from my fd 
                                 should be written to this fd */
    ipipe_forwarder * reader; /* if data is written to my fd,
                                 it will come from this fd */
    bool doread;
    bool dowuncomp;
    bool dowrite;
    bool dowcomp;
    bool outdisc;
    bool inrand;

    bool reader_done;
    bool writer_done;

    static const int buf_size    = 16000;
    static const int buf_lowater =  1000;
    circular_buffer * buf;

#ifdef I2_ZLIB
    z_streamp zs;
    static const int zbuf_size   = 32000;
    char *  inbuf;
    void zloop( void );
#endif

    int bytes_read;
    int bytes_written;

    int contig_write_space_remaining( void );
    char * write_space( void );
    void record_write( int len );

    ipipe_rollover * rollover;

    FILE * debug_log_file;

public:
    ipipe_forwarder( int _fd, bool doread, bool dowrite,
                     bool dowuncomp, bool dowcomp,
                     ipipe_rollover * _rollover,
                     bool _outdisc, bool _inrand );

    void register_others( ipipe_forwarder * _w, ipipe_forwarder * _r ) {
        writer = _w;  reader = _r;
    }
    void query_stats( int * _read, int * _written ) { 
        *_read = bytes_read;  *_written = bytes_written;
    }

    /*virtual*/ ~ipipe_forwarder( void );
    /*virtual*/ rw_response read ( fd_mgr * );
    /*virtual*/ rw_response write( fd_mgr * );
    /*virtual*/ void select_rw ( fd_mgr *, bool * rd, bool * wr );
    /*virtual*/ bool over_write_threshold( void );

    /* note virtual function write_to_fd not implemented here */
};

#endif /* __ipipe_forwarder_H__ */
