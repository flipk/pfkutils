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
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "encrypt_iface.H"
#include "types.H"
#include "encrypt_rubik4.H"
#include "encrypt_rubik5.H"

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

    char * string = argv[2];
    int type = atoi( argv[1] );

    encrypt_key * key = NULL;
    switch ( type )
    {
    case 4:
        key = new rubik4_key;
        break;
    case 5:
        key = new rubik5_key;
        break;
    default:
        printf( "type must be 4 or 5 (not %d)\n", type );
        return 1;
    }
    key->random_key( atoi( string ));
    printf( "%s\n", key->key_dump());
    return 0;
}
