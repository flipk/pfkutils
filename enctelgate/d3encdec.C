/*
    This file is part of the "pfkutils" tools written by Phil Knaack
    (pknaack1@netscape.net).
    Copyright (C) 2008  Phillip F Knaack

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "d3encdec.H"

#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define DEFAULT_KEYFILE "00_ETG_DES_KEY"

static char *
get_keyfile(void)
{
    char * ret = getenv("ETG_DES_KEY_FILE");
    if (!ret)
    {
        ret = (char*)malloc(128);
        sprintf(ret, "%s/.etgdeskey", getenv("HOME"));
        fprintf(stderr, "ETG_DES_KEY_FILE not set, using %s\n", ret);
    }
    else
        ret = strdup(ret);
    return ret;
}

bool
d3des_crypt_genkey( void )
{
    unsigned char   key[8];
    int     i, fd;

    for ( i = 0; i < 8; i++ )
        key[i] = random() & 0xff;
    char * keyfile = get_keyfile();
    fd = open( keyfile, O_WRONLY | O_CREAT | O_TRUNC, 0600 );
    if ( fd < 0 )
    {
        fprintf( stderr, "cannot write des key file '%s': %s\n",
                 keyfile, strerror( errno ));
        free(keyfile);
        return false;
    }
    write( fd, key, 8 );
    close( fd );
    fprintf( stderr, "new key written to %s\n", keyfile );
    free(keyfile);
    return true;
}

// return NULL if no key found
d3des_crypt *
d3des_crypt_loadkey( void )
{
    unsigned char  key[8];
    int    cc, fd;

    char * keyfile = get_keyfile();
    fd = open( keyfile, O_RDONLY );
    free(keyfile);
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
d3des_crypt :: encrypt_packet( unsigned char * in,  int in_len,
                               unsigned char * out, int * _out_len )
{
    in_len += 2; // account for length field itself

    bool    first            = true;
    unsigned char   temp[8];
    unsigned char * src;
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
d3des_crypt :: decrypt_packet( unsigned char * in,  int in_len,
                               unsigned char * out, int * _out_len )
{
    bool     first            = true;
    int      out_len = 0;
    unsigned char    temp[8];
    unsigned char  * dest;

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
