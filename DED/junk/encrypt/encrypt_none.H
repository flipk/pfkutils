
#ifndef __ENCRYPT_NONE_H
#define __ENCRYPT_NONE_H

#include <string.h>
#include <stdio.h>
#include "encrypt_iface.H"
#include "magic.H"

class encrypt_none_key : public encrypt_key {
public:
	static const int TYPE = MagicNumbers_Encrypt_KeyNone;
	static bool valid_keystring( char * keystring ) {
		if ( strncmp( keystring, "encrypt_none:",
			      strlen( "encrypt_none:" )) != 0 )
			return false;
		return true;
	}
	encrypt_none_key( void ) {
		type = TYPE;
	}
	bool key_parse( char * keystring ) { 
		return valid_keystring( keystring );
	}
	char * key_dump( void ) {
		return "encrypt_none:";
	}
	void random_key( int dummy ) {
		// there is no key
	}
};

class encrypt_none : public encrypt_iface {
public:
	static const int TYPE = MagicNumbers_Encrypt_ClassNone;
	int minsize( void ) { return 1; }
	encrypt_none( void ) {
		type = TYPE;
	}
	~encrypt_none( void ) { }
	bool encrypt( UCHAR * outbuf, UCHAR * inbuf, int numbytes ) {
		if ( key->type != MagicNumbers_Encrypt_KeyNone )
		{
			fprintf( stderr, "encrypt_none::encrypt: invalid "
				 "key type 0x%08x given!\n", key->type );
			return false;
		}
				// there is no key
		memcpy( outbuf, inbuf, numbytes );
		return true;
	}
	bool decrypt( UCHAR * outbuf, UCHAR * inbuf, int numbytes ) {
		if ( key->type != MagicNumbers_Encrypt_KeyNone )
		{
			fprintf( stderr, "encrypt_none::decrypt: invalid "
				 "key type 0x%08x given!\n", key->type );
			return false;
		}
				// there is no key
		memcpy( outbuf, inbuf, numbytes );
		return true;
	}
};

#endif /* __ENCRYPT_NONE_H */
