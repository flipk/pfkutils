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
