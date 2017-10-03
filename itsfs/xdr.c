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

#include "xdr.h"

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
            *data = (uchar*)malloc( *data_len );
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
            *data = (uchar*)malloc( size );
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
            *str = (uchar*)malloc( size+1 );
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
