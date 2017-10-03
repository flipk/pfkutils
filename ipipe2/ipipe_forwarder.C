
#include <unistd.h>
#include <stdlib.h>

#include "ipipe_forwarder.H"

ipipe_forwarder :: ipipe_forwarder( int _fd, bool _doread, bool _dowrite,
                                    bool _dowuncomp, bool _dowcomp )
{
    fd            = _fd;
    doread        = _doread;
    dowrite       = _dowrite;
    dowuncomp     = _dowuncomp;
    dowcomp       = _dowcomp;
    reader_done   = false;
    writer_done   = false;
    reader        = NULL;
    bytes_read    = 0;
    bytes_written = 0;
    zs            = NULL;

    if ( dowuncomp && dowcomp )
    {
        fprintf( stderr,
                 "ipipe_forwarder error cannot specify "
                 "dowuncomp && dowcomp\n" );
        exit( 1 );
    }
    buf         = dowrite ? new circular_buffer( buf_size ) : NULL;
    if ( dowuncomp || dowcomp )
    {
        inbuf         = new char[ zbuf_size ];
        zs            = new z_stream;
        zs->zalloc    = NULL;
        zs->zfree     = NULL;
        zs->opaque    = NULL;
        zs->next_in   = (Bytef*) inbuf;
        zs->next_out  = (Bytef*) buf->write_pos();
        zs->avail_in  = 0;
        zs->avail_out = zbuf_size;
    }
    if ( dowuncomp )
        inflateInit( zs );
    else if ( dowcomp )
        deflateInit( zs, 4 );
}

//virtual
ipipe_forwarder :: ~ipipe_forwarder( void )
{
    if ( zs && dowuncomp )
        inflateEnd( zs );
    if ( zs && dowcomp )
        deflateEnd( zs );
    if ( zs )
    {
        delete[] inbuf;
        delete zs;
    }
    close( fd );
    if ( writer && !writer_done )
        writer->reader_done = true;
    if ( reader && !reader_done )
        reader->writer_done = true;
    if ( buf )
        delete buf;
}

//virtual
bool
ipipe_forwarder :: select_for_read( fd_mgr * mgr )
{
    if ( writer_done )
        do_close = true;
    if ( !doread || reader_done || do_close )
        return false;
    if ( !writer )
    {
        fprintf( stderr,
                 "ipipe_forwarder :: select_for_read : null writer!\n" );
        abort();
    }
    if ( writer->write_space_remaining() < buf_lowater )
        return false;
    return true;
}

//virtual
fd_interface :: rw_response
ipipe_forwarder :: read ( fd_mgr * mgr )
{
    if ( !doread )
        return DEL;
    if ( writer_done )
        return OK;

    int len = writer->write_space_remaining();
    int cc = ::read( fd,
                     writer->write_space(),
                     len );

    if ( cc <= 0 )
    {
        do_close = true;
        if ( writer )
            writer->reader_done = true;
        if ( reader )
            reader->writer_done = true;
        return DEL;
    }

    bytes_read += cc;
    writer->record_write( cc );

    return OK;
}

//virtual
bool
ipipe_forwarder :: select_for_write( fd_mgr * mgr )
{
    if ( reader_done )
        return true;
    if ( !dowrite || do_close )
        return false;
    if ( buf->used_space() > 0 )
        return true;
    if ( zs && zs->avail_in > 0 )
        return true;
    return false;
}

//virtual
fd_interface :: rw_response
ipipe_forwarder :: write( fd_mgr * mgr )
{
    if ( !dowrite )
        return DEL;

    if ( zs )
        zloop();

    int len = buf->contig_readable();

    if ( len > 0 )
    {
        char * bufptr = buf->read_pos();
        int    cc     = ::write( fd, bufptr, len );
        if ( cc <= 0 )
        {
            do_close = true;
            if ( writer )
                writer->do_close = true;
            if ( reader )
                reader->do_close = true;
            return DEL;
        }
        bytes_written += cc;
        buf->record_read( cc );
        return OK;
    }
    /* else */

    if ( reader_done )
        return DEL;
    /* else */

    return OK;
}

int
ipipe_forwarder :: write_space_remaining( void )
{
    if ( !dowrite )
        return 0;
    if ( zs )
        return zbuf_size - zs->avail_in;
    // else
    return buf->contig_writeable();
}

char *
ipipe_forwarder :: write_space( void )
{
    if ( zs )
        return inbuf + zs->avail_in;
    // else
    return buf->write_pos();
}

void
ipipe_forwarder :: record_write( int len )
{
    if ( zs )
        zs->avail_in += len;
    else
        buf->record_write( len );
}

void
ipipe_forwarder :: zloop( void )
{
    while ( 1 )
    {
        int ret, outfirst, infirst;
        int flush = Z_NO_FLUSH;

        if ( dowcomp && reader_done )
            flush = Z_FINISH;
        else if ( dowuncomp && writer_done )
            flush = Z_FINISH;

        outfirst = buf->contig_writeable();
        if ( outfirst == 0 )
            break;
        zs->next_out = (Bytef*) buf->write_pos();
        zs->avail_out = outfirst;
        infirst = zs->avail_in;

        if ( dowcomp )
            ret = deflate( zs, flush );
        else
            ret = inflate( zs, flush );

        if ( (int) zs->avail_out != outfirst )
            buf->record_write( outfirst - zs->avail_out );

        if ( zs->avail_in != 0 )
        {
            int consumed  = infirst - zs->avail_in;
            int remaining = zs->avail_in;
            if ( remaining > 0 )
                memmove( inbuf, inbuf + consumed, remaining );
        }
        zs->next_in = (Bytef*) inbuf;

        if ( flush == Z_FINISH )
        {
            if ( ret == Z_STREAM_END )
            {
                if ( dowcomp )
                    deflateEnd( zs );
                else
                    inflateEnd( zs );
                delete zs;
                delete[] inbuf;
                zs = NULL;
            }
            break;
        }
        else
        {
            if ( zs->avail_in == 0 )
                break;
        }
    }
}
