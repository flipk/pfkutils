
#ifndef __PK_TCP_MSG_H_
#define __PK_TCP_MSG_H_

#include <sys/types.h>
#include <signal.h>
#include <stdio.h>
#include "types.H"

// define before including for a different max msg length
#ifndef MAX_PK_TCP_MSG_LENGTH
#define MAX_PK_TCP_MSG_LENGTH 1024
#endif

#define PkTcpMsgDef( classname, typevalue, body ) \
class classname : public pk_tcp_msg { \
public: \
    static const UINT16 TYPE = typevalue ; \
    classname( void ) : pk_tcp_msg( sizeof( classname ), TYPE ) { } \
    body \
}

// the COMMA definition is useful if the 'constructor' argument below
// would like to insert member-constructions
#define COMMA ,

#define PkTcpMsgDef2( classname, typevalue, body, \
                      constructargs, constructor ) \
class classname : public pk_tcp_msg { \
public: \
    static const UINT16 TYPE = typevalue ; \
    classname( void ) : pk_tcp_msg( sizeof( classname ), TYPE ) { } \
    classname constructargs : pk_tcp_msg( sizeof( classname ), TYPE ) \
        constructor \
    body \
}

struct pk_tcp_msg {
    pk_tcp_msg( UINT16 _length, UINT16 _type )
        : length( _length ), type( _type ) {
        if ( _length > (MAX_PK_TCP_MSG_LENGTH+sizeof(pk_tcp_msg)) ) {
            fprintf( stderr, "message type %#x size %d exceeds max %d!\n",
                     _type, _length, MAX_PK_TCP_MSG_LENGTH );
            kill(0,6);
        }
    }
    void set_checksum( void ) {
        magic.set( magic_value );
        checksum.set( calc_checksum());
    }
    bool verif_checksum( void ) { return checksum.get() == calc_checksum(); }
    bool verif_magic( void ) { return magic.get() == magic_value; }
    int get_len( void ) { return length.get(); }
    UINT16 get_type( void ) { return type.get(); }
    char * get_ptr(void) { return (char*) this; }
    template <class T> bool convert( T ** ptr ) {
        if ( type.get() != T::TYPE )
            return false;
        *ptr = (T*)this;
        return true;
    }
private:
    static const int magic_value = 0x5048494c;
    UINT32_t  magic;
    UINT16_t  length;
    UINT16_t  type;
    UINT16_t  checksum;
    UINT16    calc_checksum( void ) {
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

PkTcpMsgDef( MaxPkMsgType, 0x0,
             UINT8 anonymous_body[ MAX_PK_TCP_MSG_LENGTH ];
    );

class pk_tcp_msgr {
protected:
    virtual int reader( void * buf, int buflen ) = 0;
    virtual int writer( void * buf, int buflen ) = 0;
    virtual bool read_pending( void ) = 0;
public:
    virtual ~pk_tcp_msgr( void ) { /* placeholder */ }
    bool send( pk_tcp_msg * m );
    bool recv_pending( void ) { return read_pending(); }
    bool recv( pk_tcp_msg * m, int max_size );
};

#endif /* __PK_TCP_MSG_H_ */
