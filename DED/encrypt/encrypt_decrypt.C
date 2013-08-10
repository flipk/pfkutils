#if 0
incs=-I..
g++ -c $incs encrypt_iface.C
g++ -c $incs encrypt_rubik4.C
g++ -c $incs encrypt_rubik5.C
g++ -c $incs -Dmakekey_main=main makekey.C
g++ encrypt_iface.o encrypt_rubik4.o encrypt_rubik5.o makekey.o -o mk
exit 0
#endif

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
 *

a file 6793292 bytes in size, encrypted
with keys of varying limb sizes and
using varying algorithms. times:

limbs     rubik5    rubik4
 1 :        0.56    1.30
 2 :        0.83    1.28
 3 :        1.39    1.31
 4 :        1.71    1.30
 5 :        1.87    1.29
 6 :        2.46    1.31
 7 :        2.83    1.30
 8 :        3.29    1.30
 9 :        4.28    1.31
10 :        4.25    1.29
11 :        4.75    1.31
12 :        5.26    1.28
13 :        5.69    1.30
14 :        6.18    1.30
15 :        6.04    1.31
16 :        6.97    1.31

 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "encrypt_iface.H"
#include "types.H"
#include "threads.H"

class encodedecodemain : public Thread {
    void entry( void );
    char * keystring;
public:
    encodedecodemain( char * _keystring )
        : Thread( "encdec", 10, 256 * 1024 ) {
        keystring = _keystring;
        resume( tid );
    }
};

static int
encrypt_decrypt_main( int argc, char ** argv )
{
    if ( argc != 2 )
    {
        printf(  "please put a key on the command line\n" );
        return 1;
    }

    ThreadParams p;
    p.my_eid = 1;
    Threads th( &p );
    (void) new encodedecodemain( argv[1] );
    th.loop();
    return 0;
}

static bool do_encrypt;

extern "C" {
    int encrypt_main( int argc, char ** argv );
    int decrypt_main( int argc, char ** argv );
};

int
encrypt_main( int argc, char ** argv )
{
    do_encrypt = true;
    return encrypt_decrypt_main( argc, argv );
}

int
decrypt_main( int argc, char ** argv )
{
    do_encrypt = false;
    return encrypt_decrypt_main( argc, argv );
}

void
encodedecodemain :: entry( void )
{
    encrypt_iface * crypt = parse_key( keystring );
    if ( crypt == NULL )
    {
        printf(  "error making crypt object\n" );
        return;
    }

    // disk buffer:
    // 2 bytes of cc, cc bytes of encrypted data
    //       which contains cc-2 bytes of real data

    register_fd( 0 );
    while ( 1 )
    {
        UCHAR inbuf[65536];
        UCHAR outbuf[65536];
        int cc;

        if ( do_encrypt )
        {
            cc = read( 0, (char*)inbuf+2, 65530 );
            if ( cc <= 0 )
                break;
            ((UINT16_t *)inbuf)->set( cc );
            if ( cc < crypt->minsize() )
                cc = crypt->minsize();
            crypt->encrypt( outbuf+2, inbuf, cc+2 );
            ((UINT16_t *)outbuf)->set( cc + 2 );
            th->write( 1, (char*)outbuf, cc+4 );
        }
        else
        {
            cc = read( 0, (char*)inbuf, 2 );
            if ( cc != 2 )
                break;
            int sz = ((UINT16_t *)inbuf)->get();
            int cc2 = read( 0, inbuf+2, sz );
            if ( cc2 != sz )
                break;
            crypt->decrypt( outbuf, inbuf+2, cc2 );
            int outsz = ((UINT16_t *)outbuf)->get();
            th->write( 1, (char*)outbuf+2, outsz );
        }
    }
}
