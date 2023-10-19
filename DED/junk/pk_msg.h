
#ifndef __PK_MSG_H_
#define __PK_MSG_H_

#include <sys/types.h>
#include <signal.h>
#include <stdio.h>
#include "types.H"

// define before including for a different max msg length
#ifndef MAX_PK_MSG_LENGTH
#define MAX_PK_MSG_LENGTH 1024
#endif

#define PkMsgDef( classname, typevalue, body ) \
class classname : public pk_msg { \
public: \
    static const UINT16 TYPE = typevalue ; \
    classname( void ) : pk_msg( sizeof( classname ), TYPE ) { } \
    body \
}

// the COMMA definition is useful if the 'constructor' argument below
// would like to insert member-constructions.  yeah, i know, it's kind
// of gross -- i admit, not my finest hour.
#define COMMA ,

#define PkMsgDef2( classname, typevalue, body, \
                      constructargs, constructor ) \
class classname : public pk_msg { \
public: \
    static const UINT16 TYPE = typevalue ; \
    classname( void ) : pk_msg( sizeof( classname ), TYPE ) { } \
    classname constructargs : pk_msg( sizeof( classname ), TYPE ) \
        constructor \
    body \
}

struct pk_msg {
    pk_msg( UINT16 _length, UINT16 _type )
        : length( _length ), type( _type ) {
        if ( _length > (MAX_PK_MSG_LENGTH+sizeof(pk_msg)) ) {
            fprintf( stderr, "message type %#x size %d exceeds max %d!\n",
                     _type, _length, MAX_PK_MSG_LENGTH );
            kill(0,6);
        }
    }
    void set_checksum( void ) {
        magic.set( magic_value );
        checksum.set( calc_checksum());
    }
    bool verif_checksum( void ) { return checksum.get() == calc_checksum(); }
    bool verif_magic( void ) { return magic.get() == magic_value; }
    bool verif( void ) { return verif_magic() && verif_checksum(); }
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
    friend class pk_msgr;
    static const unsigned int magic_value = 0x5048494c;
    static const unsigned int magic_1 = (magic_value >> 24) & 0xFF;
    static const unsigned int magic_2 = (magic_value >> 16) & 0xFF;
    static const unsigned int magic_3 = (magic_value >>  8) & 0xFF;
    static const unsigned int magic_4 = (magic_value >>  0) & 0xFF;
    static const unsigned short checksum_start = 0x504b;
    static const unsigned short checksum_const = 0x4621;
    static const unsigned char checksum_xor = 0x46;
    UINT32_t  magic;
    UINT16_t  length;
    UINT16_t  type;
    UINT16_t  checksum;
    UINT16    calc_checksum( void ) {
        UINT8 * p = (UINT8*) this;
        UINT16 cs = checksum_start;
        UINT16 oldcs = checksum.get();
        checksum.set( checksum_const );
        for ( int i = 0; i < length.get(); i++ )
            cs += (*p++ ^ checksum_xor);
        checksum.set( oldcs );
        return cs;
    }
};

PkMsgDef( MaxPkMsgType, 0x0,
          UINT8 anonymous_body[ MAX_PK_MSG_LENGTH ];
    );

class pk_msgr {
protected:
    virtual int writer( void * buf, int buflen ) = 0;
    virtual void recv( pk_msg *m ) = 0;
//
    int max_msg_len;
    MaxPkMsgType  * msg;
    UINT8 * outbuf;
    int stateleft;
    int state;
public:
    pk_msgr( int _max_msg_len );
    virtual ~pk_msgr( void );
    bool send( pk_msg * m );
    // user already did a read and passed data to me
    void process_read( UINT8 * buf, int buflen );
    // user wants me to do the read. returns same as read(2)
    int  process_read( int fd );
};

#endif /* __PK_MSG_H_ */
