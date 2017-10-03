
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include "ipipe_forwarder.H"
#include "ipipe_stats.H"

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

//    this is an option if we can optimize the 
//      write path to reduce the number of select_for_writes that happen
//    make_nonblocking();

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
        zs->avail_in  = 0;
        zs->next_out  = (Bytef*) buf->write_pos();
        zs->avail_out = buf->contig_writeable();
    }
    if ( dowuncomp )
        inflateInit( zs );
    else if ( dowcomp )
        deflateInit( zs, 4 );

    stats_add_conn();
}

//virtual
ipipe_forwarder :: ~ipipe_forwarder( void )
{
    stats_del_conn();

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
void
ipipe_forwarder :: select_rw ( fd_mgr * mgr, bool * rd, bool * wr )
{
    if ( writer_done )
        do_close = true;
    if ( !doread || reader_done || do_close )
        *rd = false;
    else
    {
        if ( !writer )
        {
            fprintf( stderr,
                     "ipipe_forwarder :: select_for_read : null writer!\n" );
            abort();
        }
        if ( writer->over_write_threshold() )
            *rd = false;
        else
            *rd = true;
    }
    if ( reader_done )
        *wr = true;
    else if ( !dowrite || do_close )
        *wr = false;
    else if ( buf->used_space() > 0 )
        *wr = true;
    else if ( zs && zs->avail_in > 0 )
        *wr = true;
    else
        *wr = false;
}

//virtual
fd_interface :: rw_response
ipipe_forwarder :: read ( fd_mgr * mgr )
{
    if ( !doread )
        return DEL;
    if ( writer_done )
        return OK;

    char * bufptr = writer->write_space();
    int len = writer->contig_write_space_remaining();
    int cc = ::read( fd, bufptr, len );

    if ( cc < 0 )
    {
        if ( errno == EAGAIN )
            return OK;

        fprintf( stderr, "ipipe_forwarder :: read : %s\n",
                 strerror( errno ));
    }
    
    if ( cc <= 0 )
    {
        do_close = true;
        if ( writer )
            writer->reader_done = true;
        if ( reader )
            reader->writer_done = true;
        return DEL;
    }

    stats_add( cc, 0 );
    bytes_read += cc;
    writer->record_write( cc );

    return OK;
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

        if ( cc < 0 )
        {
            if ( errno == EAGAIN )
                return OK;

            fprintf( stderr, "ipipe_forwarder :: write: %s\n",
                     strerror( errno ));
        }

        if ( cc <= 0 )
        {
            do_close = true;
            if ( writer )
                writer->do_close = true;
            if ( reader )
                reader->do_close = true;
            return DEL;
        }

        stats_add( 0, cc );

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

bool
ipipe_forwarder :: over_write_threshold( void )
{
    if ( zs )
    {
        if (( zbuf_size - zs->avail_in ) > 0 )
            return false;
        return true;
    }
    if ( buf->free_space() > buf_lowater )
        return false;
    return true;
}

int
ipipe_forwarder :: contig_write_space_remaining( void )
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
        {
            ret = inflate( zs, flush );
            if ( ret < 0  &&  ret != Z_BUF_ERROR )
            {
                fprintf( stderr, "libz: %d: %s\n", ret, zs->msg );
                exit( 1 );
            }
        }

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
