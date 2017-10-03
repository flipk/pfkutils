
#ifndef __PK_TCP_MSG_H_
#define __PK_TCP_MSG_H_

#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include "types.H"

struct pk_tcp_msg {
    pk_tcp_msg( UINT16 _type, UINT16 _length )
        : type( _type ), length( _length ) { }
    void set_checksum( void ) { checksum.set( calc_checksum()); }
    bool verif_checksum( void ) { return checksum.get() == calc_checksum(); }
    bool verif_magic( void ) { return magic.get() == magic_value; }
    int get_len( void ) { return length.get(); }
private:
    static const int magic_value = 0x5048494c;
    UINT32_t magic;
    UINT16_t length;
    UINT16_t type;
    UINT16_t checksum;
    UINT16 calc_checksum( void ) {
        UINT8 * p = (UINT8*) this;
        UINT16 cs = 0x504b;
        UINT16 oldcs = checksum.get();
        checksum.set( 0 );
        for ( int i = 0; i < length.get(); i++ )
            cs += (*p++ ^ 0x46);
        checksum.set( oldcs );
        return cs;
    }
};

class pk_tcp_msgr {
    enum states { HEADER, BODY };
    int fd;
public:
    pk_tcp_msgr( int _fd ) { fd = _fd; }
    ~pk_tcp_msgr( void ) { close( fd ); }
    bool send( pk_tcp_msg * m );
    bool recv( pk_tcp_msg * m, int max_size );
};

#endif /* __PK_TCP_MSG_H_ */
