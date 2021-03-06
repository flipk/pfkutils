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

// this module understands all supported encryption types
// and parses them out.

#include "encrypt_iface.H"
#include "encrypt_rubik4.H"
#include "encrypt_rubik5.H"
#include "encrypt_none.H"
#include "lognew.H"

encrypt_iface *
parse_key( char * keystring )
{
    encrypt_key * key = NULL;
    encrypt_iface * ret = NULL;

    if ( encrypt_none_key::valid_keystring( keystring ))
        key = LOGNEW encrypt_none_key;
    else if ( rubik4_key::valid_keystring( keystring ))
        key = LOGNEW rubik4_key;
    else if ( rubik5_key::valid_keystring( keystring ))
        key = LOGNEW rubik5_key;
    
    if ( key == NULL )
        return NULL;

    if ( key->key_parse( keystring ) == false )
    {
        delete key;
        return NULL;
    }

    switch ( key->type )
    {
    case encrypt_none_key::TYPE:
        ret = LOGNEW encrypt_none;
        break;
    case rubik5_key::TYPE:
        ret = LOGNEW rubik5;
        break;
    case rubik4_key::TYPE:
        ret = LOGNEW rubik4;
        break;
    }

    if ( ret == NULL )
    {
        delete key;
        return NULL;
    }

    ret->key = key;

    return ret;
}
