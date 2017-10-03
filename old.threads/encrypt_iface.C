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

// this module understands all supported encryption types
// and parses them out.

#include "encrypt_iface.H"
#include "encrypt_rubik4.H"
/* #include "encrypt_rubik5.H" */
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
/*    else if ( rubik5_key::valid_keystring( keystring ))
        key = LOGNEW rubik5_key;
*/
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
/*    case rubik5_key::TYPE:
        ret = LOGNEW rubik5;
        break; */
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
