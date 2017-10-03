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
