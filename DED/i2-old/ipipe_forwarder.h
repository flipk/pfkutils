/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */


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

    int pausing_bytes;
    int pausing_delay;

    bool reader_done;
    bool writer_done;

    static const int buf_size    = 16000;
    static const int buf_lowater =  1000;
    circular_buffer<char> * buf;

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
                     bool _outdisc, bool _inrand,
                     int _pausing_bytes, int _pausing_delay );

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
