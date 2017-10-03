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

#include "xdr.h"
#include "lognew.H"

void * malloc( int size );
void free( void * );
u_int ntohl( u_int );
u_int htonl( u_int );

bool_t
myxdr_bytes( XDR * xdr, uchar ** data, u_int * data_len, u_int max_data )
{
    if ( !myxdr_u_int( xdr, data_len ))
        return FALSE;

    if ( *data_len == 0 )
        return TRUE;

    switch ( xdr->encode_decode )
    {
    case XDR_DECODE:
        if ( *data_len > max_data )
            return FALSE;
        if ( *data == NULL )
            *data = (uchar*)MALLOC( *data_len );
        /* fallthru */

    case XDR_ENCODE:
        return myxdr_opaque( xdr, *data, *data_len );

    case XDR_FREE:
        if ( *data != NULL )
        {
            free( *data );
            *data = NULL;
        }
        return TRUE;
    }
    return FALSE;
}

bool_t
myxdr_opaque( XDR * xdr, uchar * data, u_int size )
{
    u_int roundup;

    roundup = size % 4;
    if ( roundup > 0 )
        roundup = 4 - roundup;

    switch ( xdr->encode_decode )
    {
    case XDR_DECODE:
        if ( xdr->bytes_left < size )
            return FALSE;
        memcpy( data, xdr->data + xdr->position, size );
        xdr->position += size + roundup;
        xdr->bytes_left -= size + roundup;
        return TRUE;

    case XDR_ENCODE:
        if ( xdr->bytes_left < size )
            return FALSE;
        memcpy( xdr->data + xdr->position, data, size );
        xdr->position += size;
        xdr->bytes_left -= size;
        if ( roundup > 0 )
        {
            memset( xdr->data + xdr->position, 0, roundup );
        }
        xdr->position += roundup;
        xdr->bytes_left -= roundup;
        return TRUE;

    case XDR_FREE:
        /* nothing to free */
        return TRUE;
    }
    return FALSE;
}

bool_t
myxdr_reference( XDR * xdr, uchar ** data, u_int size, xdrproc_t method )
{
    bool_t stat;

    if ( *data == NULL )
    {
        switch ( xdr->encode_decode )
        {
        case XDR_FREE:
            return TRUE;

        case XDR_DECODE:
            *data = (uchar*)MALLOC( size );
            memset( *data, 0, size );
            break;
        }
    }

    stat = (*method)( xdr, *data );

    if ( xdr->encode_decode == XDR_FREE )
    {
        free( *data );
        *data = NULL;
    }

    return stat;
}

bool_t
myxdr_pointer( XDR * xdr, uchar ** data, u_int size, xdrproc_t method )
{
    bool_t more_data;

    more_data = (*data != NULL);
    if ( !myxdr_bool( xdr, &more_data ))
        return FALSE;

    if ( !more_data )
    {
        *data = NULL;
        return TRUE;
    }

    return myxdr_reference( xdr, data, size, method );
}

bool_t
myxdr_string( XDR * xdr, uchar ** str, u_int max )
{
    u_int size;

    switch ( xdr->encode_decode )
    {
    case XDR_FREE:
        if ( *str != NULL )
        {
            free( *str );
            *str = NULL;
        }
        return TRUE;

    case XDR_ENCODE:
        size = strlen( (char*)*str );
        break;
    }

    if ( !myxdr_u_int( xdr, &size ))
        return FALSE;
    if ( size > max )
        return FALSE;

    switch ( xdr->encode_decode )
    {
    case XDR_DECODE:
        if ( *str == NULL )
            *str = (uchar*)MALLOC( size+1 );
        (*str)[size] = 0;
        /* fallthru */

    case XDR_ENCODE:
        return myxdr_opaque( xdr, *str, size );
    }

    return FALSE;
}

bool_t
myxdr_int( XDR * xdr, int * v )
{
    return myxdr_u_int( xdr, (u_int*)v );
}

bool_t
myxdr_u_int( XDR * xdr, u_int * v )
{
    uchar * p;
    u_int v2;

    if ( xdr->encode_decode != XDR_FREE )
        if ( xdr->bytes_left < 4 )
            return FALSE;

    switch ( xdr->encode_decode )
    {
    case XDR_ENCODE:
        p = xdr->data + xdr->position;
        v2 = *v;
        *p++ = (v2 >> 24) & 0xff;
        *p++ = (v2 >> 16) & 0xff;
        *p++ = (v2 >>  8) & 0xff;
        *p++ = (v2 >>  0) & 0xff;
        break;

    case XDR_DECODE:
        p = xdr->data + xdr->position;
        v2 = *p++;
        v2 <<= 8;
        v2 |= *p++;
        v2 <<= 8;
        v2 |= *p++;
        v2 <<= 8;
        v2 |= *p++;
        *v = v2;
        break;

    case XDR_FREE:
        return TRUE;

    default:
        return FALSE;
    }

    xdr->position += 4;
    xdr->bytes_left -= 4;

    return TRUE;
}

bool_t
myxdr_void( XDR * xdr )
{
    return TRUE;
}

bool_t
myxdr_enum( XDR * xdr, enum_t * v )
{
    return myxdr_u_int( xdr, (u_int*)v );
}

bool_t
myxdr_bool( XDR * xdr, bool_t * v )
{
    return myxdr_u_int( xdr, (u_int*)v );
}

bool_t
myxdr_union( XDR * xdr, enum_t * disc, uchar * union_ptr,
           xdrunion_table_t * tab, xdrproc_t def )
{
    enum_t mydisc;

    if ( !myxdr_enum( xdr, disc ))
        return FALSE;

    mydisc = *disc;

    for (; tab->proc != NULL; tab++ )
        if ( tab->val == mydisc )
            return tab->proc( xdr, union_ptr );

    if ( def == NULL )
        return FALSE;

    return def( xdr, union_ptr );
}
