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
