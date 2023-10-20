
#ifndef __RUBIK5_H_
#define __RUBIK5_H_

#include "encrypt_iface.H"
#include "magic.H"

class rubik5_key : public encrypt_key {
public:
	static const int TYPE = MagicNumbers_Encrypt_KeyRubik5;
	static bool valid_keystring( char * keystring );
	rubik5_key( void );
	~rubik5_key( void );
	// returns false if bogus key
	bool key_parse( char * key );
	// delete[] the return value when you're done
	char * key_dump( void );
	// generate new key
	void random_key( int _numkeys );
private:
	void make_inverse( void );
	int numkeys;
	UINT * keys;
	UINT * inverse_keys;
	static UINT invertkey( UINT x ) {
		// reverse order of the six transformations
		// present in a UINT, and also invert the 
		// direction bits.
		return
			(((x & 0x0000001f) << 25) +
			 ((x & 0x000003e0) << 15) +
			 ((x & 0x00007c00) <<  5) +
			 ((x & 0x000f8000) >>  5) +
			 ((x & 0x01f00000) >> 15) +
			 ((x & 0x3e000000) >> 25) + 
			 ((x & 0xc0000000))) ^ 0x21084210;
	}
	friend class rubik5;
};

class rubik5 : public encrypt_iface {
	struct face {
#if BITFIELDS_FROM_LEFT  // like sparc
		UINT spare:7;
		UINT p55:1; UINT p54:1;	UINT p53:1; UINT p52:1; UINT p51:1;
		UINT p45:1; UINT p44:1; UINT p43:1; UINT p42:1; UINT p41:1;
		UINT p35:1; UINT p34:1; UINT p33:1; UINT p32:1; UINT p31:1;
		UINT p25:1; UINT p24:1;	UINT p23:1; UINT p22:1; UINT p21:1;
		UINT p15:1; UINT p14:1; UINT p13:1; UINT p12:1; UINT p11:1;
#else // bitfields from right, like intel
		UINT p11:1; UINT p12:1; UINT p13:1; UINT p14:1; UINT p15:1;
		UINT p21:1; UINT p22:1; UINT p23:1; UINT p24:1; UINT p25:1;
		UINT p31:1; UINT p32:1; UINT p33:1; UINT p34:1; UINT p35:1;
		UINT p41:1; UINT p42:1; UINT p43:1; UINT p44:1; UINT p45:1;
		UINT p51:1; UINT p52:1; UINT p53:1; UINT p54:1; UINT p55:1;
		UINT spare:7;
#endif
	};
	static const int RUBIK_BUF_SIZE = 19;
	void fill( UCHAR * buf );
	void empty( UCHAR * buf );
	// perform 6 transformations in the 30-bit format
	void transform( UINT trans );
	void print( void );
	// perform one transformation in the 5-bit format
	char * transform( UCHAR transformation );
	struct face * memory;
	struct face * f[6];
	struct face * spare;
	int extrabits;
public:
	static const int TYPE = MagicNumbers_Encrypt_ClassRubik5;
	int minsize( void ) { return RUBIK_BUF_SIZE; }
	rubik5( void );
	~rubik5( void );
	bool encrypt( UCHAR * outbuf, UCHAR * inbuf, int numbytes );
	bool decrypt( UCHAR * outbuf, UCHAR * inbuf, int numbytes );
};

#endif // __RUBIK5_H_
