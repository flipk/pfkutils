
#ifndef __ENCRYPT_IFACE_H_
#define __ENCRYPT_IFACE_H_

#include "types.H"

class encrypt_key {
public:
    int type;
    virtual ~encrypt_key( void ) {};
    // each derived class should implement the following:
    // static bool valid_keystring( char * keystring );
    virtual bool key_parse( char * keystring ) = 0;
    virtual char * key_dump( void ) = 0;
    virtual void random_key( int param ) = 0;
};

class encrypt_iface {
public:
    int type;
    virtual ~encrypt_iface( void ) { delete key; };
    virtual int minsize( void ) = 0;
    virtual bool encrypt( UCHAR * outbuf,
                          UCHAR * inbuf, int numbytes ) = 0;
    virtual bool decrypt( UCHAR * outbuf,
                          UCHAR * inbuf, int numbytes ) = 0;
    encrypt_key * key;
};


encrypt_iface *  parse_key( char * keystring );

#endif /* __ENCRYPT_IFACE_H_ */
