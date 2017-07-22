/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */
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

#include <stdio.h>
#include <string.h>
#include <inttypes.h>

template <class T>
class circular_buffer {
    uint32_t size;
    uint32_t used;
    uint32_t in;   /* write data into buf */
    uint32_t out;  /* read  data out of buf */
    T * buf;

public:
    circular_buffer( uint32_t _size ) {
        size = _size + 1;
        in = out = used = 0;
        buf = new char[size];
        memset( buf, 0, size );
    }
    ~circular_buffer  ( void ) { delete[] buf; }

    void print ( bool printbody, const char * header = NULL ) const
    {
        if ( !header )
            header = (char*)"";
        fprintf( stderr,
                 "%sbuf: s %3d, u %4d, in %4d, out %4d, cw %4d, cr %4d\n",
                 header,
                 size, used, in, out, contig_writeable(), contig_readable() );
        if ( printbody )
        {
            for ( uint32_t i = 0; i < size; i++ )
                fprintf( stderr,
                         "%c%02x%c ", (i==out)?'o':' ',
                         buf[i],
                         (i==in)?'i':' ' );
            fprintf( stderr, "\n\n" );
        }
    }

    bool   empty      ( void ) const { return used == 0;         }
    bool   full       ( void ) const { return used == (size-1);  }
    int    free_space ( void ) const { return (size-1) - used;   }
    int    used_space ( void ) const { return used;              }

    int    write      ( T * bp, uint32_t bsz ) {
        /* truncate if there isn't enough space for the whole packet. */
        if ( bsz > free_space() )
            bsz = free_space();
        /* find how much contiguous space is available currently. */
        uint32_t cpy = contig_writeable();
        /* if contiguous space is less than packet size, we will
           copy in two parts; "cpy" then represents first half. */
        if ( cpy > bsz )
            cpy = bsz;
        /* now we can copy; this works if (a) we truncated the packet
           because of space concerns, (b) we're copying in two pieces
           because of wraparound, or (c) the whole packet fits in one
           piece. */
        if ( cpy > 0 )
            memcpy( buf + in, bp, cpy );
        /* in the wrap case, there's more data to copy. */
        if ( cpy != bsz )
            memcpy( buf, bp + cpy, bsz - cpy );
        /* update pointers. */
        record_write( bsz );
        return bsz;
    }
    int    read       ( T * bp, uint32_t bsz ) {
        /* trim bsz to the data remaining in the buffer. */
        if ( bsz > used_space() )
            bsz = used_space();
        /* find out how much to read is contiguous. */
        uint32_t cpy = contig_readable();
        /* determine if the read is contiguous or if it must
           be done in 2 parts. */
        if ( cpy > bsz )
            cpy = bsz;
        /* now decide to copy; note this works if (a) we're returning
           less than the caller asked for because there's not enough
           in the buffer to satisfy the full request, (b) we're copying
           in two pieces because of wraparound, or (c) we're returning
           all of the bytes the user requested in a single contiguous
           piece. */
        if ( cpy > 0 )
            memcpy( bp, buf + out, cpy );
        /* clean up the wraparound case, too. */
        if ( cpy != bsz )
            memcpy( bp + cpy, buf, bsz - cpy );
        /* update pointers. */
        record_read( bsz );
        return bsz;
    }

    uint32_t contig_writeable (void) const {
        int r = (out >  in) ? (out - in) : (size -  in);
        int f = free_space();
        return (r > f) ? f : r;
    }
    uint32_t contig_readable  (void) const {
        return (in >= out) ? (in - out) : (size - out);
    }

    void record_write( uint32_t s ) {
        in  += s; if ( in >= size ) in -= size; used += s;
    }
    void record_read ( uint32_t s ) {
        out += s; if ( out >= size ) out -= size; used -= s;
    }
    T * write_pos ( void ) const { return buf + in;  }
    T * read_pos  ( void ) const { return buf + out; }
};
