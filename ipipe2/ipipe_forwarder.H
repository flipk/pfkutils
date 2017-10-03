
#ifndef __ipipe_forwarder_H__
#define __ipipe_forwarder_H__

#include <zlib.h>

#include "fd_mgr.H"
#include "circular_buffer.H"

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
    bool reader_done;
    bool writer_done;
//    bool finish; // flush out libz buffers and then do_close
    static const int buf_size    = 5000;
    static const int buf_lowater = 1000;
    circular_buffer * buf;
    z_streamp zs;
    static const int zbuf_size   = 5000;
    char *  inbuf;
    void zloop( void );
public:
    ipipe_forwarder( int _fd, bool doread, bool dowrite,
                     bool dowuncomp, bool dowcomp );
    void register_others( ipipe_forwarder * _w, ipipe_forwarder * _r )
    { writer = _w;  reader = _r; }

    int write_space_remaining( void );
    char * write_space( void );
    void record_write( int len );

    /*virtual*/ ~ipipe_forwarder( void );
    /*virtual*/ rw_response read ( fd_mgr * );
    /*virtual*/ rw_response write( fd_mgr * );
    /*virtual*/ bool select_for_read( fd_mgr * );
    /*virtual*/ bool select_for_write( fd_mgr * );
};

#endif /* __ipipe_forwarder_H__ */
