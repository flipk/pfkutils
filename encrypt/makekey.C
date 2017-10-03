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
