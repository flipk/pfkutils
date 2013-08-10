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
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __RUBIK4_H_
#define __RUBIK4_H_

#include "encrypt_iface.H"
#include "magic.H"

class rubik4_key : public encrypt_key {
public:
    static const int TYPE = MagicNumbers_Encrypt_KeyRubik4;
    static const int BUFLEN = 12;
    static bool valid_keystring( char * keystring );
    rubik4_key( void );
    ~rubik4_key( void );
    // returns false if bogus key
    bool key_parse( char * key );
    // delete[] the return value when you're done
    char * key_dump( void );
    // generate new key
    void  random_key( int _numkeys );
    void print( void );
private:
    int numkeys;
    UINT * keys;
    void  transform32( UINT trans );
    void  transform4 ( UINT trans );
    void  rotface( int face, int dir );
    void  rotslice( int slice, int dir );
    void  compile( void );
    void  encode( UCHAR * out, UCHAR * in );
    void  decode( UCHAR * out, UCHAR * in );
    UCHAR stickers[0x60];
    UCHAR xorpattern[BUFLEN];
    friend class rubik4;
};

class rubik4 : public encrypt_iface {
    static const int BUFLEN = rubik4_key::BUFLEN;
    UCHAR data1[BUFLEN];
    UCHAR data2[BUFLEN];
    UCHAR * data;
public:
    static const int TYPE = MagicNumbers_Encrypt_ClassRubik4;
    int minsize( void ) { return BUFLEN; }
    rubik4( void );
    ~rubik4( void );
    bool encrypt( UCHAR * outbuf, UCHAR * inbuf, int numbytes );
    bool decrypt( UCHAR * outbuf, UCHAR * inbuf, int numbytes );
};

#endif // __RUBIK4_H_
