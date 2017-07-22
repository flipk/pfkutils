/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
 */

#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include "ipipe_forwarder.h"
#include "ipipe_stats.h"
#include "ipipe_main.h"

char *
next_debug_file_name( void )
{
    static int debug_file_number = 1;
    static char filename[32];

    sprintf(filename, "dbg%06d.bin", debug_file_number++);
    return filename;
}

ipipe_forwarder :: ipipe_forwarder( int _fd, bool _doread, bool _dowrite,
                                    bool _dowuncomp, bool _dowcomp,
                                    ipipe_rollover * _rollover,
                                    bool _outdisc, bool _inrand,
                                    int _pausing_bytes, int _pausing_delay )
{
    fd            = _fd;
    doread        = _doread;
    dowrite       = _dowrite;
    dowuncomp     = _dowuncomp;
    dowcomp       = _dowcomp;
    rollover      = _rollover;
    outdisc       = _outdisc;
    inrand        = _inrand;
    pausing_bytes = _pausing_bytes;
    pausing_delay = _pausing_delay;
    reader_done   = false;
    writer_done   = false;
    reader        = NULL;
    bytes_read    = 0;
    bytes_written = 0;

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
    buf         = dowrite ? new circular_buffer<char>( buf_size ) : NULL;
#ifdef I2_ZLIB
    zs            = NULL;
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
#endif

    stats_add_conn();

    if (ipipe2_debug_log)
    {
        char * filename = next_debug_file_name();
        debug_log_file = fopen( filename, "w");
        if (debug_log_file)
        {
            // PFK
        }
        else
        {
            fprintf(stderr, "unable to open debug file %s: %s\n",
                    filename, strerror(errno));
        }
    }
    else
        debug_log_file = NULL;
}

//virtual
ipipe_forwarder :: ~ipipe_forwarder( void )
{
    stats_del_conn();

#ifdef I2_ZLIB
    if ( zs && dowuncomp )
        inflateEnd( zs );
    if ( zs && dowcomp )
        deflateEnd( zs );
    if ( zs )
    {
        delete[] inbuf;
        delete zs;
    }
#endif
    close( fd );
    if ( writer && !writer_done )
        writer->reader_done = true;
    if ( reader && !reader_done )
        reader->writer_done = true;
    if ( buf )
        delete buf;
    if (rollover)
        delete rollover;
    if (debug_log_file)
    {
        fclose(debug_log_file);
    }
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
#ifdef I2_ZLIB
    else if ( zs && zs->avail_in > 0 )
        *wr = true;
#endif
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
    int cc;

    if (pausing_bytes > 0  &&
        len > pausing_bytes)
    {
        len = pausing_bytes;
    }

    if ( inrand )
    {
        for ( cc = 0; cc < len; cc++ )
            bufptr[cc] = random();
        cc = len;
    }
    else
        cc = ::read( fd, bufptr, len );

    if ( cc < 0 )
    {
        if ( errno == EAGAIN )
            return OK;

        fprintf( stderr, "\nipipe_forwarder :: read : %s\n",
                 strerror( errno ));
    }
    
    if ( cc == 0 )
    {
        if (( buf && buf->used_space() > 0 )
#ifdef I2_ZLIB
            || ( zs && zs->avail_in > 0 )
#endif
            )
        {
            fprintf( stderr, "\nipipe_forwarder :: "
                     "closed unexpectedly during read!\n" );
        }
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

    if ( cc > 0 )
    {
        i2_add_md5_recv( bufptr, cc );
        if (cc == pausing_bytes)
            usleep(pausing_delay);
    }

    stats_add( cc, 0 );
    bytes_read += cc;
    writer->record_write( cc );

    if (debug_log_file && cc > 0)
    {
        fwrite(bufptr, cc, 1, debug_log_file);
        // PFK
    }

    return OK;
}

//virtual
fd_interface :: rw_response
ipipe_forwarder :: write( fd_mgr * mgr )
{
    if ( !dowrite )
        return DEL;

#ifdef I2_ZLIB
    if ( zs )
        zloop();
#endif

    int len = buf->contig_readable();

    if ( len > 0 )
    {
        char * bufptr = buf->read_pos();
        int cc;

        if ( outdisc )
            cc = len;
        else
            cc = ::write( fd, bufptr, len );

        if ( cc < 0 )
        {
            if ( errno == EAGAIN )
                return OK;

            fprintf( stderr, "\nipipe_forwarder :: write: %s\n",
                     strerror( errno ));
        }

        if ( cc == 0 )
            fprintf( stderr, "\nipipe_forwarder :: "
                     "closed unexpectedly during write!\n" );

        if ( cc <= 0 )
        {
            do_close = true;
            if ( writer )
                writer->do_close = true;
            if ( reader )
                reader->do_close = true;
            return DEL;
        }

        if ( cc > 0 )
            i2_add_md5_writ( bufptr, cc );

        stats_add( 0, cc );

        bytes_written += cc;
        buf->record_read( cc );

        if (rollover)
            rollover->check_rollover(fd, cc);

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
#ifdef I2_ZLIB
    if ( zs )
    {
        if (( zbuf_size - zs->avail_in ) > 0 )
            return false;
        return true;
    }
#endif
    if ( buf->free_space() > buf_lowater )
        return false;
    return true;
}

int
ipipe_forwarder :: contig_write_space_remaining( void )
{
    if ( !dowrite )
        return 0;
#ifdef I2_ZLIB
    if ( zs )
        return zbuf_size - zs->avail_in;
#endif
    // else
    return buf->contig_writeable();
}

char *
ipipe_forwarder :: write_space( void )
{
#ifdef I2_ZLIB
    if ( zs )
        return inbuf + zs->avail_in;
#endif
    // else
    return buf->write_pos();
}

void
ipipe_forwarder :: record_write( int len )
{
#ifdef I2_ZLIB
    if ( zs )
        zs->avail_in += len;
    else
#endif
        buf->record_write( len );
}

#ifdef I2_ZLIB
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
                fprintf( stderr, "\nlibz: %d: %s\n", ret, zs->msg );
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
#endif
