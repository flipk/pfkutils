/*
 * This code is originally written by Phillip F Knaack.
 * There are absolutely no restrictions on the use of this code.  It is
 * NOT GPL!  If you use this code in a product, you do NOT have to release
 * your alterations, nor do you have to acknowledge in any way that you
 * are using this software.
 * The only thing I ask is this: do not claim you wrote it.  My name
 * should appear in this file forever as the original author, and every
 * person who modifies this file after me should also place their names
 * in this file (so those that follow know who broke what).
 * This restriction is not enforced in any way by any license terms, it
 * is merely a personal request from me to you.  If you wanted, you could
 * even completely remove this entire comment and place a new one with your
 * company's confidential proprietary license on it-- but if you are a good
 * internet citizen, you will comply with my request out of the goodness
 * of your heart.
 * If you do use this code and you make a buttload of money off of it,
 * I would appreciate a little kickback-- but again this is a personal
 * request and is not required by any licensing of this code.  (Of course
 * in the offchance that anyone every actually DOES make a lot of money 
 * using this code, I know I'm going to regret that statement, but at
 * this point in time it feels like the right thing to say.)
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.	IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

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
