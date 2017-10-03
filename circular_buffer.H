
#include <stdio.h>
#include <string.h>

class circular_buffer {
    int size;
    int used;
    int in;   /* write data into buf */
    int out;  /* read  data out of buf */
    char * buf;

public:
    circular_buffer( int _size ) {
        size = _size + 1;
        in = out = used = 0;
        buf = new char[size];
        memset( buf, 0, size );
    }
    ~circular_buffer  ( void ) { delete[] buf; }

    void print ( void ) {
        printf( "\nbuf: s %3d, u %3d, in %3d, out %3d, cw %3d, cr %3d\n",
                size, used, in, out, contig_write(), contig_read() );
        for ( int i = 0; i < size; i++ )
            printf( "%c%02x%c ", (i==out)?'o':' ', buf[i], (i==in)?'i':' ' );
        printf( "\n\n" );
    }

    bool   empty      ( void ) { return used == 0;         }
    bool   full       ( void ) { return used == (size-1);  }
    int    free_space ( void ) { return (size-1) - used;   }
    int    used_space ( void ) { return used;              }

    int    write      ( char * bp, int bsz ) {
        /* truncate if there isn't enough space for the whole packet. */
        if ( bsz > free_space() )
            bsz = free_space();
        /* find how much contiguous space is available currently. */
        int cpy = contig_write();
        /* if contiguous space is greater than packet size, we will
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
    int    read       ( char * bp, int bsz ) {
        /* trim bsz to the data remaining in the buffer. */
        if ( bsz > used_space() )
            bsz = used_space();
        /* find out how much to read is contiguous. */
        int cpy = contig_read();
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

    int contig_write (void) {
        int r = (out >  in) ? (out - in) : (size -  in);
        int f = free_space();
        return (r > f) ? f : r;
    }
    int contig_read  (void) {
        return (in >= out) ? (in - out) : (size - out);
    }

    void record_write( int s ) { in  += s; in  %= size; used += s; }
    void record_read ( int s ) { out += s; out %= size; used -= s; }
    char * write_pos ( void ) { return buf + in;  }
    char * read_pos  ( void ) { return buf + out; }
};
