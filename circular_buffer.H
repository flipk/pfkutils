
#include <string.h>

class circular_buffer {
    int size;
    int used;
    int in;   /* write data into buf */
    int out;  /* read  data out of buf */
    char * buf;
public:
    circular_buffer( int _size )
        { size = _size + 1; in = out = used = 0; buf = new char[size]; }
    ~circular_buffer  ( void ) { delete[] buf; }
    bool   empty      ( void ) { return used == 0; }
    bool   full       ( void ) { return used == (size-1); }
    int    free_space ( void ) { return (size-1) - used; }
    int    used_space ( void ) { return used; }
    int    write      ( char * bp, int bsz ) {
        if ( bsz > free_space() )   bsz = free_space();
        int cpy = contig_write();
        if ( cpy > bsz ) cpy = bsz;
        if ( cpy > 0 ) memcpy( buf + in, bp, cpy );
        if ( cpy != bsz ) memcpy( buf, bp + cpy, bsz - cpy );
        record_write( bsz );
        return bsz; }
    int    read       ( char * bp, int bsz ) {
        if ( bsz > used_space() ) bsz = used_space();
        int cpy = contig_read();
        if ( cpy > bsz ) cpy = bsz;
        if ( cpy > 0 ) memcpy( bp, buf + out, cpy );
        if ( cpy != bsz ) memcpy( bp + cpy, buf, bsz - cpy );
        record_read( bsz );
        return bsz; }

    int contig_write (void) { return (out >  in) ? (out - in) : (size -  in); }
    int contig_read  (void) { return (in >= out) ? (in - out) : (size - out); }
    void record_write( int s ) { in  += s; in  %= size; used += s; }
    void record_read ( int s ) { out += s; out %= size; used -= s; }
    char * write_pos ( void ) { return buf + in; }
    char * read_pos  ( void ) { return buf + out; }
    
};
