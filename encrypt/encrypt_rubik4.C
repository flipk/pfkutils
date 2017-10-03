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


/*
              +-------------+
  U           | 00 01 02 03 |
L F R B       | 04 05 06 07 |
  D           | 08 09 0a 0b |
              | 0c 0d 0e 0f |
+-------------+-------------+-------------+-------------+
| 10 11 12 13 | 20 21 22 23 | 30 31 32 33 | 40 41 42 43 |
| 14 15 16 17 | 24 25 26 27 | 34 35 36 37 | 44 45 46 47 |
| 18 19 1a 1b | 28 29 2a 2b | 38 39 3a 3b | 48 49 4a 4b |
| 1c 1d 1e 1f | 2c 2d 2e 2f | 3c 3d 3e 3f | 4c 4d 4e 4f |
+-------------+-------------+-------------+-------------+
              | 50 51 52 53 |
              | 54 55 56 57 |
              | 58 59 5a 5b |
              | 5c 5d 5e 5f |
              +-------------+

the following moves are possible:

          00: U     01: U-1   02: L     03: L-1   
          04: F     05: F-1   06: R     07: R-1   
          08: B     09: B-1   10: D     11: D-1   
          12: LH    13: LH-1  14: UH    15: UH-1  

before beginning an encryption sequence, the encryption key has to be
compiled.  this is done when a key is parsed.

key compilation involves taking a cube with numbers written on all the
stickers in order, performing the sequence of moves specified in the key,
and then writing down the resulting arrangement of the original numbers.

then in future encryption sequences, instead of doing "n" transformations
found in the key, we do 1 transformation found in the list written down
above.  this is a tremendous time saver as compared to the design for the
rubik5 encryption algorithm, where the "n" transformations are performed
for each 19-byte group of input.

 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "encrypt_rubik4.H"
#include "lognew.H"

rubik4_key :: rubik4_key( void )
{
    numkeys = 0;
    keys = NULL;
    type = TYPE;
}

rubik4_key :: ~rubik4_key( void )
{
    if ( keys != NULL )
        delete[] keys;
}

bool
rubik4_key :: key_parse( char * key )
{
    int r;

    if ( numkeys != 0 )
    {
        numkeys = 0;
        delete[] keys;
        keys = NULL;
    }

    r = sscanf( key, "rubik4:%08x%08x%08x,%d,",
                (int*)&xorpattern[0],
                (int*)&xorpattern[4],
                (int*)&xorpattern[8],
                &numkeys );

    if ( r != 4 )
        return false;

    if ( numkeys == 0 || numkeys > 200 )
    {
        numkeys = 0;
        return false;
    }

    key += 34;
    if ( numkeys > 9 )
        key ++;
    if ( numkeys > 99 )
        key ++;

    UINT * local_keys = LOGNEW UINT[numkeys];

    for ( int i = 0; i < numkeys; i++ )
    {
        if ( sscanf( key, "%08x", &local_keys[i] ) != 1 )
        {
            delete[] local_keys;
            numkeys = 0;
            return false;
        }
        key += 8;
    }
    keys = local_keys;
    compile();
    return true;
}

void
rubik4_key :: transform32( UINT trans )
{
    transform4( (trans >>  0) & 0xf );
    transform4( (trans >>  4) & 0xf );
    transform4( (trans >>  8) & 0xf );
    transform4( (trans >> 12) & 0xf );
    transform4( (trans >> 16) & 0xf );
    transform4( (trans >> 20) & 0xf );
    transform4( (trans >> 24) & 0xf );
    transform4( (trans >> 28) & 0xf );
}

void
rubik4_key :: rotface( int f, int dir )
{
    UCHAR t[4];
    UCHAR * s;
    UCHAR * fa = NULL, * fb = NULL, * fc = NULL, * fd = NULL;
    int a0=0,a1=0,a2=0,a3=0;
    int b0=0,b1=0,b2=0,b3=0;
    int c0=0,c1=0,c2=0,c3=0;
    int d0=0,d1=0,d2=0,d3=0;

    s = &stickers[ f << 4 ];

#define cpy3(d,s,da,db,dc,sa,sb,sc) \
d[da]=s[sa];d[db]=s[sb];d[dc]=s[sc]

    if ( dir == 0 )
    {
        cpy3(t,s,0x0,0x1,0x2,0x0,0x1,0x2);
        cpy3(s,s,0x0,0x1,0x2,0xc,0x8,0x4);
        cpy3(s,s,0xc,0x8,0x4,0xf,0xe,0xd);
        cpy3(s,s,0xf,0xe,0xd,0x3,0x7,0xb);
        cpy3(s,t,0x3,0x7,0xb,0x0,0x1,0x2);
        t[0]=s[5];s[5]=s[9];s[9]=s[10];s[10]=s[6];s[6]=t[0];
    }
    else
    {
        cpy3(t,s,0x0,0x1,0x2,0x0,0x1,0x2);
        cpy3(s,s,0x0,0x1,0x2,0x3,0x7,0xb);
        cpy3(s,s,0x3,0x7,0xb,0xf,0xe,0xd);
        cpy3(s,s,0xf,0xe,0xd,0xc,0x8,0x4);
        cpy3(s,t,0xc,0x8,0x4,0x0,0x1,0x2);
        t[0]=s[5];s[5]=s[6];s[6]=s[10];s[10]=s[9];s[9]=t[0];
    }

#undef cpy3

#define df(w,x,a,b,c,d)  f##w=&stickers[x<<4];w##0=a;w##1=b;w##2=c;w##3=d

    switch ( f )
    {
    case 0:
        df(a,4,0x3,0x2,0x1,0x0);
        df(b,3,0x3,0x2,0x1,0x0);
        df(c,2,0x3,0x2,0x1,0x0);
        df(d,1,0x3,0x2,0x1,0x0);
        break;
    case 1:
        df(a,0,0x0,0x4,0x8,0xc);
        df(b,2,0x0,0x4,0x8,0xc);
        df(c,5,0x0,0x4,0x8,0xc);
        df(d,4,0xf,0xb,0x7,0x3);
        break;
    case 2:
        df(a,0,0xc,0xd,0xe,0xf);
        df(b,3,0x0,0x4,0x8,0xc);
        df(c,5,0x3,0x2,0x1,0x0);
        df(d,1,0xf,0xb,0x7,0x3);
        break;
    case 3:
        df(a,2,0xf,0xb,0x7,0x3);
        df(b,0,0xf,0xb,0x7,0x3);
        df(c,4,0x0,0x4,0x8,0xc);
        df(d,5,0xf,0xb,0x7,0x3);
        break;
    case 4:
        df(a,3,0xf,0xb,0x7,0x3);
        df(b,0,0x3,0x2,0x1,0x0);
        df(c,1,0x0,0x4,0x8,0xc);
        df(d,5,0xc,0xd,0xe,0xf);
        break;
    case 5:
        df(a,1,0xc,0xd,0xe,0xf);
        df(b,2,0xc,0xd,0xe,0xf);
        df(c,3,0xc,0xd,0xe,0xf);
        df(d,4,0xc,0xd,0xe,0xf);
        break;
    }

#undef df

#define cpy(d,s,da,db,dc,dd,sa,sb,sc,sd) \
d[da]=s[sa];d[db]=s[sb];d[dc]=s[sc];d[dd]=s[sd]

    if ( dir == 0 )
    {
        cpy(t,fa,0,1,2,3,a0,a1,a2,a3);
        cpy(fa,fd,a0,a1,a2,a3,d0,d1,d2,d3);
        cpy(fd,fc,d0,d1,d2,d3,c0,c1,c2,c3);
        cpy(fc,fb,c0,c1,c2,c3,b0,b1,b2,b3);
        cpy(fb,t,b0,b1,b2,b3,0,1,2,3);
    }
    else
    {
        cpy(t,fa,0,1,2,3,a0,a1,a2,a3);
        cpy(fa,fb,a0,a1,a2,a3,b0,b1,b2,b3);
        cpy(fb,fc,b0,b1,b2,b3,c0,c1,c2,c3);
        cpy(fc,fd,c0,c1,c2,c3,d0,d1,d2,d3);
        cpy(fd,t,d0,d1,d2,d3,0,1,2,3);
    }

#undef cpy

}

void
rubik4_key :: rotslice( int slice, int dir )
{
    UCHAR t[4];
    UCHAR * s = &stickers[0];

#define cpy(fa,a1,a2,a3,a4,fb,b1,b2,b3,b4) \
fa[a1]=fb[b1];fa[a2]=fb[b2];fa[a3]=fb[b3];fa[a4]=fb[b4];

    if ( slice == 0 )
    {
        if ( dir == 0 )
        {
            cpy(t,0x00,0x01,0x02,0x03,s,0x01,0x05,0x09,0x0d);
            cpy(s,0x01,0x05,0x09,0x0d,s,0x4e,0x4a,0x46,0x42);
            cpy(s,0x4e,0x4a,0x46,0x42,s,0x51,0x55,0x59,0x5d);
            cpy(s,0x51,0x55,0x59,0x5d,s,0x21,0x25,0x29,0x2d);
            cpy(s,0x21,0x25,0x29,0x2d,t,0x00,0x01,0x02,0x03);
        }
        else
        {
            cpy(t,0x00,0x01,0x02,0x03,s,0x01,0x05,0x09,0x0d);
            cpy(s,0x01,0x05,0x09,0x0d,s,0x21,0x25,0x29,0x2d);
            cpy(s,0x21,0x25,0x29,0x2d,s,0x51,0x55,0x59,0x5d);
            cpy(s,0x51,0x55,0x59,0x5d,s,0x4e,0x4a,0x46,0x42);
            cpy(s,0x4e,0x4a,0x46,0x42,t,0x00,0x01,0x02,0x03);
        }
    }
    else
    {
        if ( dir == 0 )
        {
            cpy(t,0x00,0x01,0x02,0x03,s,0x14,0x15,0x16,0x17);
            cpy(s,0x14,0x15,0x16,0x17,s,0x24,0x25,0x26,0x27);
            cpy(s,0x24,0x25,0x26,0x27,s,0x34,0x35,0x36,0x37);
            cpy(s,0x34,0x35,0x36,0x37,s,0x44,0x45,0x46,0x47);
            cpy(s,0x44,0x45,0x46,0x47,t,0x00,0x01,0x02,0x03);
        }
        else
        {
            cpy(t,0x00,0x01,0x02,0x03,s,0x14,0x15,0x16,0x17);
            cpy(s,0x14,0x15,0x16,0x17,s,0x44,0x45,0x46,0x47);
            cpy(s,0x44,0x45,0x46,0x47,s,0x34,0x35,0x36,0x37);
            cpy(s,0x34,0x35,0x36,0x37,s,0x24,0x25,0x26,0x27);
            cpy(s,0x24,0x25,0x26,0x27,t,0x00,0x01,0x02,0x03);
        }
    }

#undef cpy

}

void
rubik4_key :: transform4( UINT trans )
{
    int dir = trans & 1;
    trans >>= 1;

    if ( trans < 6 )
    {
        rotface(trans,dir);
    }
    else if ( trans == 6 )
    {
        /* LEFTHALF */

        rotface(1,dir);
        rotslice(0,dir);
    }
    else
    {
        /* RIGHTHALF */

        rotface(0,dir);
        rotslice(1,dir);
    }
}

void
rubik4_key :: compile( void )
{
    int i;
    for ( i = 0; i < 0x60; i++ )
        stickers[i] = i;
    for ( i = 0; i < numkeys; i++ )
        transform32( keys[i] );
}

void
rubik4_key :: encode( UCHAR * out, UCHAR * in )
{
    int i;
    memset( out, 0, 12 );

    for ( i = 0; i < 0x60; i++ )
    {
        int frompos, frombit;
        
        frompos = stickers[i];
        frombit = (in[ frompos >> 3 ] >> ( frompos & 7 )) & 1;

        if ( frombit )
            out[i>>3] |= ( 1 << ( i & 7 ));
    }

    for ( i = 0; i < BUFLEN; i++ )
        out[i] ^= xorpattern[i];

}

void
rubik4_key :: decode( UCHAR * out, UCHAR * in )
{
    int i;
    memset( out, 0, 12 );

    for ( i = 0; i < BUFLEN; i++ )
        in[i] ^= xorpattern[i];

    for ( i = 0; i < 0x60; i++ )
    {
        int topos, frombit;

        frombit = (in[ i >> 3 ] >> ( i & 7 )) & 1;

        if ( frombit )
        {
            topos = stickers[i];
            out[ topos >> 3 ] |= ( 1 << ( topos & 7 ));
        }
    }
}

void
rubik4_key :: print( void )
{
    printf( 
        "              +-------------+\n"
        "  U           | %02x %02x %02x %02x |\n"
        "L F R B       | %02x %02x %02x %02x |\n"
        "  D           | %02x %02x %02x %02x |\n"
        "              | %02x %02x %02x %02x |\n"
        "+-------------+-------------+-------------+-------------+\n"
        "| %02x %02x %02x %02x | %02x %02x %02x %02x |"
        " %02x %02x %02x %02x | %02x %02x %02x %02x |\n"
        "| %02x %02x %02x %02x | %02x %02x %02x %02x |"
        " %02x %02x %02x %02x | %02x %02x %02x %02x |\n"
        "| %02x %02x %02x %02x | %02x %02x %02x %02x |"
        " %02x %02x %02x %02x | %02x %02x %02x %02x |\n"
        "| %02x %02x %02x %02x | %02x %02x %02x %02x |"
        " %02x %02x %02x %02x | %02x %02x %02x %02x |\n"
        "+-------------+-------------+-------------+-------------+\n"
        "              | %02x %02x %02x %02x |\n"
        "              | %02x %02x %02x %02x |\n"
        "              | %02x %02x %02x %02x |\n"
        "              | %02x %02x %02x %02x |\n"
        "              +-------------+\n",

/* four stickers in a row */
#define s(a) stickers[a],stickers[a+1],stickers[a+2],stickers[a+3]
/*   */
#define t(a) s(a),s(a+16),s(a+32),s(a+48)
#define u(a) s(a),s(a+4),s(a+8),s(a+12)

        u(0x00),t(0x10),t(0x14),t(0x18),t(0x1c),u(0x50)

        );

#undef s

}

char *
rubik4_key :: key_dump( void )
{
    char * ret, * cp;

    if ( numkeys == 0 )
        return "rubik4:0,0";

    cp = ret = LOGNEW char[45 + (numkeys * 8)];

    UINT * xp = (UINT*) &xorpattern[0];

    sprintf( cp, "rubik4:%08x%08x%08x",
             xp[0], xp[1], xp[2] );
    cp += strlen( cp );

    sprintf( cp, ",%d,", numkeys );
    cp += strlen( cp );

    for ( int i = 0; i < numkeys; i++ )
    {
        sprintf( cp, "%08x", keys[i] );
        cp += strlen( cp );
    }

    return ret;
}

void
rubik4_key :: random_key( int _numkeys )
{
    if ( numkeys != 0 )
        delete[] keys;
    numkeys = _numkeys;
    keys = LOGNEW UINT[numkeys];
    for ( int i = 0; i < numkeys; i++ )
        keys[i] = random();
    compile();
    UINT * p = (UINT*)&xorpattern[0];
    p[0] = random();
    p[1] = random();
    p[2] = random();
    compile();
}

bool
rubik4_key :: valid_keystring( char * keystring )
{
    if ( strncmp( keystring, "rubik4:",
                  strlen( "rubik4:" )) != 0 )
        return false;
    return true;
}

rubik4 :: rubik4( void )
{
    type = TYPE;
}

rubik4 :: ~rubik4( void )
{
}

bool
rubik4 :: encrypt( UCHAR * outbuf, UCHAR * inbuf, int numbytes )
{
    rubik4_key * mykey = (rubik4_key *)key;

    if ( key->type != rubik4_key::TYPE )
    {
        printf( "rubik4::encrypt: key argument is "
                "not a rubik4 key! type = 0x%08x\n",
                key->type );
        return false;
    }

    if ( numbytes < BUFLEN )
    {
        printf( "rubik4::encrypt: warning: cannot encrypt "
                "buffer smaller than %d bytes\n", BUFLEN );
        return false;
    }

    while ( numbytes >= BUFLEN )
    {
        mykey->encode( outbuf, inbuf );
        numbytes -= BUFLEN;
        outbuf   += BUFLEN;
        inbuf    += BUFLEN;
    }

    if ( numbytes > 0 )
    {
        UCHAR temp[BUFLEN];
        memcpy( outbuf, inbuf, numbytes );
        outbuf -= BUFLEN - numbytes;
        mykey->encode( temp, outbuf );
        memcpy( outbuf, temp, BUFLEN );
    }

    return true;
}

bool
rubik4 :: decrypt( UCHAR * outbuf, UCHAR * inbuf, int numbytes )
{
    rubik4_key * mykey = (rubik4_key *)key;

    if ( key->type != rubik4_key::TYPE )
    {
        printf( "rubik4::encrypt: key argument is "
                "not a rubik4 key! type = 0x%08x\n",
                key->type );
        return false;
    }

    if ( numbytes < BUFLEN )
    {
        printf( "rubik4::encrypt: warning: cannot decrypt "
                "buffer smaller than %d bytes\n", BUFLEN );
        return false;
    }

    int lastbytes = numbytes % BUFLEN;

    if ( lastbytes > 0 )
    {
        UCHAR temp[ BUFLEN ];
        UCHAR * lastinbuf  = inbuf  + numbytes - BUFLEN;
        UCHAR * inbuf_trailer  =  inbuf + numbytes - lastbytes;
        UCHAR * outbuf_trailer = outbuf + numbytes - lastbytes;

        mykey->decode( temp, lastinbuf );
        memcpy( lastinbuf, temp, BUFLEN );
        memcpy( outbuf_trailer, inbuf_trailer, lastbytes );
    }

    while ( numbytes >= BUFLEN )
    {
        mykey->decode( outbuf, inbuf );
        outbuf   += BUFLEN;
        inbuf    += BUFLEN;
        numbytes -= BUFLEN;
    }

    return true;
}
