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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "encrypt_iface.H"
#include "types.H"
#include "threads.H"
#include "threads/h_internal/encrypt_rubik4.H"
/* #include "threads/h_internal/encrypt_rubik5.H" */

class makekeymain : public Thread {
    void entry( void ) {
        encrypt_key * key = NULL;
        switch ( type )
        {
        case 4:
            key = new rubik4_key;
            break;
/*        case 5:
            key = new rubik5_key;
            break; */
        default:
/*            printf( "type must be 4 or 5 (not %d)\n", type ); */
            printf( "type must be 4 (not %d)\n", type );
            return;
        }
        key->random_key( atoi( string ));
        ::printf( "%s\n", key->key_dump());
    };
    char * string;
    int type;
public:
    makekeymain( int _type, char * _string )
        : Thread( "makekey" ) {
        string = _string;
        type = _type;
        resume( tid );
    }
};

extern "C" {
    int makekey_main( int argc, char ** argv );
}

int
makekey_main( int argc, char ** argv )
{
    if ( argc != 3 )
    {
        printf(  "usage: makekey <4 or 5> <# limbs>\n" );
        return 1;
    }

    ThreadParams p;
    p.my_eid = 1;
    Threads th( &p );
    (void) new makekeymain( atoi( argv[1] ), argv[2] );
    th.loop();
    return 0;
}
