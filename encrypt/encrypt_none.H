/*
 * This code is written by Phillip F Knaack. This code is in the
 * public domain. Do anything you want with this code -- compile it,
 * run it, print it out, pass it around, shit on it -- anything you want,
 * except, don't claim you wrote it.  If you modify it, add your name to
 * this comment in the COPYRIGHT file and to this comment in every file
 * you change.
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
