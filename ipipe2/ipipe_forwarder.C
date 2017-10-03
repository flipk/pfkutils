
#include <unistd.h>
#include <stdlib.h>

#include "ipipe_forwarder.H"

ipipe_forwarder :: ipipe_forwarder( int _fd, bool _doread, bool _dowrite,
                                    bool _dowuncomp, bool _dowcomp )
{
    fd        = _fd;
    doread    = _doread;
    dowrite   = _dowrite;
    dowuncomp = _dowuncomp;
    dowcomp   = _dowcomp;
    reader    = NULL;
    if ( dowuncomp && dowcomp )
    {
        fprintf( stderr,
                 "ipipe_forwarder error cannot specify "
                 "dowuncomp && dowcomp\n" );
        exit( 1 );
    }
    buf       = dowrite ? new circular_buffer( buf_size ) : NULL;
    if ( dowuncomp || dowcomp )
    {
        inbuf  = new char[ zbuf_size ];
        outbuf = new char[ zbuf_size ];
        zs = new z_stream;
        zs->zalloc    = NULL;
        zs->zfree     = NULL;
        zs->opaque    = NULL;
        zs->next_in   = (Bytef*) inbuf;
        zs->next_out  = (Bytef*) outbuf;
        zs->avail_in  = 0;
        zs->avail_out = zbuf_size;
    }
    else
        zs = NULL;
    if ( dowuncomp )
        inflateInit( zs );
    else if ( dowcomp )
        deflateInit( zs, 4 );
}

//virtual
ipipe_forwarder :: ~ipipe_forwarder( void )
{
    if ( zs && dowuncomp )
    {
        inflateEnd( zs );
    }
    if ( zs && dowcomp )
    {
        deflateEnd( zs );
    }
    if ( dowuncomp || dowcomp )
    {
        delete[] inbuf;
        delete[] outbuf;
        delete zs;
    }
    close( fd );
    if ( writer )
        writer->do_close = true;
    if ( reader )
        reader->do_close = true;
    if ( buf )
        delete buf;
}

//virtual
bool
ipipe_forwarder :: select_for_read( fd_mgr * mgr )
{
    // xxx if ( dowuncomp )

    if ( !doread || do_close )
        return false;
    if ( writer->write_space_remaining() < buf_lowater )
        return false;
    return true;
}

//virtual
bool
ipipe_forwarder :: read ( fd_mgr * mgr )
{
    if ( !doread )
        return false;

    // xxx if ( dowuncomp )

    char tmpbuf[ buf_size ];

    int cc = ::read( fd, tmpbuf, writer->write_space_remaining() );

    if ( cc <= 0 )
    {
        do_close = true;
        if ( writer )
            writer->do_close = true;
        if ( reader )
            reader->do_close = true;
        return false;
    }

    writer->write_other( tmpbuf, cc );

    return true;
}

//virtual
bool
ipipe_forwarder :: select_for_write( fd_mgr * mgr )
{
    // xxx if ( dowcomp )

    if ( !dowrite || do_close )
        return false;
    if ( buf->used_space() > 0 )
        return true;
    return false;
}

//virtual
bool
ipipe_forwarder :: write( fd_mgr * mgr )
{
    if ( !dowrite )
        return false;

    // xxx if ( dowcomp )

    char * bufptr = buf->read_pos();
    int    len    = buf->contig_readable();
    int    cc     = ::write( fd, bufptr, len );

    if ( cc <= 0 )
    {
        do_close = true;
        if ( writer )
            writer->do_close = true;
        if ( reader )
            reader->do_close = true;
        return false;
    }

    buf->record_read( cc );

    return true;
}

int
ipipe_forwarder :: write_space_remaining( void )
{
    // xxx if ( dowcomp )

    if ( !dowrite )
        return 0;
    return buf->free_space();
}

void
ipipe_forwarder :: write_other( char * bufp, int len )
{
    if ( buf->write( bufp, len ) != len )
    {
        fprintf( stderr, "ipipe_forwarder :: write_other : impossible?\n" );
    }
}
