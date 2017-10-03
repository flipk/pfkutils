
#include "d3encdec.H"

#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define KEYFILE "00_ETG_DES_KEY"

bool
d3des_crypt_genkey( void )
{
    uchar   key[8];
    int     i, fd;

    for ( i = 0; i < 8; i++ )
        key[i] = random() & 0xff;
    fd = open( KEYFILE, O_WRONLY | O_CREAT | O_TRUNC, 0600 );
    if ( fd < 0 )
    {
        fprintf( stderr, "cannot write des key file '%s': %s\n",
                 KEYFILE, strerror( errno ));
        return false;
    }
    write( fd, key, 8 );
    close( fd );
    fprintf( stderr, "new key written to %s\n", KEYFILE );
    return true;
}

// return NULL if no key found
d3des_crypt *
d3des_crypt_loadkey( void )
{
    uchar  key[8];
    int    cc, fd;

    fd = open( KEYFILE, O_RDONLY );
    if ( fd > 0 )
    {
        cc = read( fd, key, 8 );
        close( fd );
        if ( cc == 8 )
        {
            fprintf( stderr, "loaded encryption key\n" );
            return new d3des_crypt( key );
        }
    }
    fprintf( stderr, "encryption key not found--PLAINTEXT MODE\n" );
    return NULL;
}

//virtual
void
d3des_crypt :: encrypt_packet( uchar * in,  int in_len,
                               uchar * out, int * _out_len )
{
    in_len += 2; // account for length field itself

    bool    first            = true;
    uchar   temp[8];
    uchar * src;
    int     out_len = 0;

    while ( in_len >= 8 )
    {
        if ( first )
        {
            first = false;
            temp[0] = ((in_len-2) >> 8) & 0xff;
            temp[1] = ((in_len-2)     ) & 0xff;
            memcpy( temp+2, in, 6 );
            in += 6;
            src = temp;
        }
        else
        {
            src = in;
            in += 8;
        }

        crypt.encrypt( src, out );
        out_len += 8;
        out += 8;
        in_len -= 8;
    }

    if ( in_len > 0 )
    {
        if ( first )
        {
            temp[0] = ((in_len-2) >> 8) & 0xff;
            temp[1] = ((in_len-2)     ) & 0xff;
            memcpy( temp+2, in, in_len );
            src = temp;
        }
        else
        {
            src = in;
        }
        crypt.encrypt( src, out );
        out_len += 8;
    }

    *_out_len = out_len;
}

//virtual
bool
d3des_crypt :: decrypt_packet( uchar * in,  int in_len,
                               uchar * out, int * _out_len )
{
    bool     first            = true;
    int      out_len = 0;
    uchar    temp[8];
    uchar  * dest;

    while ( in_len >= 8 )
    {
        if ( first )
            dest = temp;
        else
            dest = out;

        crypt.decrypt( in, dest );

        if ( first )
        {
            first = false;
            out_len = (temp[0] << 8) + temp[1];
            memcpy( out, temp+2, 6 );
            out += 6;
        }
        else
        {
            out += 8;
        }

        in += 8;
        in_len -= 8;
    }

    if ( in_len > 0 )
    {
        dest = temp;

        crypt.decrypt( in, dest );

        if ( first )
        {
            out_len = (temp[0] << 8) + temp[1];
            memcpy( out, temp+2, 6 );
        }
        else
        {
            memcpy( out, temp, 8 );
        }
    }

    *_out_len = out_len;
}
