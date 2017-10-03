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

/*
 * 5x5x5 rubik's cube encryption algorithm.
 *
 * treat each 150 bits of the input stream as the 150 stickers
 * on the 98 movable cubes of the professor's cube.
 *
 * i actually didn't technically do it right.  a real cube
 * rotates 90 degrees at a time, which i do correctly on the faces.
 * but when i rotate slices in the middle i do them one bit at a time.
 * that is wrong, in the sense that a real rubiks cube doesn't work like
 * that, but it makes for better encryption.
 * (if better encryption is really possible with an algorithm
 * this lame. ;)
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "encrypt_rubik5.H"
#include "threads.H"

/*
 * implement a 6-bit circular shift on five bits of a face
 * and an overflow bit.  uses CPP string-concatenation to
 * construct the array dereference.  the five bits of the face
 * that are affected are addressed by (a1,b1),(a2,b2),etc.
 */
#define _SHIFT5(f,ov,a1,a2,a3,a4,a5,b1,b2,b3,b4,b5) \
{ UINT tmp=f->p##a5##b5; \
f->p##a5##b5=f->p##a4##b4; \
f->p##a4##b4=f->p##a3##b3; \
f->p##a3##b3=f->p##a2##b2; \
f->p##a2##b2=f->p##a1##b1; \
f->p##a1##b1=ov; \
ov=tmp; }

/*
 * do a 6-bit circular shift in either direction.
 * note that "dir" is zero or one.
 */
#define SHIFT5(f,ov,a1,a2,a3,a4,a5,b1,b2,b3,b4,b5,dir) \
   if ((dir) == 0) \
   _SHIFT5(f,ov,a1,a2,a3,a4,a5,b1,b2,b3,b4,b5) else \
   _SHIFT5(f,ov,a5,a4,a3,a2,a1,b5,b4,b3,b2,b1)

/*
 * perform the 20-bit circular shift around the cube using the
 * specified four faces and the specified rows or columns on each face.
 * GRP1 is for manipulating slices parallel to the front face.
 * GRP2 is for manipulating slices parallel to the left face.
 * GRP3 is for manipulating slices parallel to the top face.
 * also include "dir", and if we're rotating backwards, do everything
 * in precisely the reverse order.
 * note that "ov" is used to carry the overflow bits from each 5-bit
 * shift from face to face.  it is preinitialized with the last bit that
 * is shifted, to prime the circular shift.
 */

#define SHIFTGRP1(ft,fr,fb,fl,toprow,rightcol,botrow,leftcol,dir) \
if ((dir)==0) { \
  UINT ov; \
  ov = fl->p##1##leftcol; \
  SHIFT5(ft,ov,toprow,toprow,toprow,toprow,toprow,1,2,3,4,5,dir); \
  SHIFT5(fr,ov,1,2,3,4,5,rightcol,rightcol,rightcol,rightcol,rightcol,dir); \
  SHIFT5(fb,ov,botrow,botrow,botrow,botrow,botrow,5,4,3,2,1,dir); \
  SHIFT5(fl,ov,5,4,3,2,1,leftcol,leftcol,leftcol,leftcol,leftcol,dir); \
} else { \
  UINT ov; \
  ov = ft->p##toprow##1; \
  SHIFT5(fl,ov,5,4,3,2,1,leftcol,leftcol,leftcol,leftcol,leftcol,dir); \
  SHIFT5(fb,ov,botrow,botrow,botrow,botrow,botrow,5,4,3,2,1,dir); \
  SHIFT5(fr,ov,1,2,3,4,5,rightcol,rightcol,rightcol,rightcol,rightcol,dir); \
  SHIFT5(ft,ov,toprow,toprow,toprow,toprow,toprow,1,2,3,4,5,dir); \
}

#define SHIFTGRP2(ft,fr,fb,fl,topcol,rightcol,botcol,leftcol,dir) \
if ((dir)==0) { \
  UINT ov; \
  ov = fl->p##1##leftcol; \
  SHIFT5(ft,ov,1,2,3,4,5,topcol,topcol,topcol,topcol,topcol,dir); \
  SHIFT5(fr,ov,1,2,3,4,5,rightcol,rightcol,rightcol,rightcol,rightcol,dir); \
  SHIFT5(fb,ov,1,2,3,4,5,botcol,botcol,botcol,botcol,botcol,dir); \
  SHIFT5(fl,ov,5,4,3,2,1,leftcol,leftcol,leftcol,leftcol,leftcol,dir); \
} else { \
  UINT ov; \
  ov = ft->p##1##topcol; \
  SHIFT5(fl,ov,5,4,3,2,1,leftcol,leftcol,leftcol,leftcol,leftcol,dir); \
  SHIFT5(fb,ov,1,2,3,4,5,botcol,botcol,botcol,botcol,botcol,dir); \
  SHIFT5(fr,ov,1,2,3,4,5,rightcol,rightcol,rightcol,rightcol,rightcol,dir); \
  SHIFT5(ft,ov,1,2,3,4,5,topcol,topcol,topcol,topcol,topcol,dir); \
}

#define SHIFTGRP3(ft,fr,fb,fl,toprow,rightrow,botrow,leftrow,dir) \
if ((dir)==0) { \
  UINT ov; \
  ov = fl->p##leftrow##1; \
  SHIFT5(ft,ov,toprow,toprow,toprow,toprow,toprow,5,4,3,2,1,dir); \
  SHIFT5(fr,ov,rightrow,rightrow,rightrow,rightrow,rightrow,5,4,3,2,1,dir); \
  SHIFT5(fb,ov,botrow,botrow,botrow,botrow,botrow,5,4,3,2,1,dir); \
  SHIFT5(fl,ov,leftrow,leftrow,leftrow,leftrow,leftrow,5,4,3,2,1,dir); \
} else { \
  UINT ov; \
  ov = ft->p##toprow##5; \
  SHIFT5(fl,ov,leftrow,leftrow,leftrow,leftrow,leftrow,5,4,3,2,1,dir); \
  SHIFT5(fb,ov,botrow,botrow,botrow,botrow,botrow,5,4,3,2,1,dir); \
  SHIFT5(fr,ov,rightrow,rightrow,rightrow,rightrow,rightrow,5,4,3,2,1,dir); \
  SHIFT5(ft,ov,toprow,toprow,toprow,toprow,toprow,5,4,3,2,1,dir); \
}

/*
 * rotate a face clockwise.  do this by specifying the target face (s)
 * and the spare face structure (d).  when done, the pointers for the target
 * and spare get swapped, so the old face becomes the new spare.
 */

#define ROTFACE_CW(s,d) { \
d->p15=s->p11; d->p14=s->p21; d->p13=s->p31; d->p12=s->p41; d->p11=s->p51; \
d->p25=s->p12; d->p24=s->p22; d->p23=s->p32; d->p22=s->p42; d->p21=s->p52; \
d->p35=s->p13; d->p34=s->p23; d->p33=s->p33; d->p32=s->p43; d->p31=s->p53; \
d->p45=s->p14; d->p44=s->p24; d->p43=s->p34; d->p42=s->p44; d->p41=s->p54; \
d->p55=s->p15; d->p54=s->p25; d->p53=s->p35; d->p52=s->p45; d->p51=s->p55; }

/*
 * rotate a face counter-clockwise.
 */

#define ROTFACE_CCW(s,d) { \
d->p51=s->p11; d->p52=s->p21; d->p53=s->p31; d->p54=s->p41; d->p55=s->p51; \
d->p41=s->p12; d->p42=s->p22; d->p43=s->p32; d->p44=s->p42; d->p45=s->p52; \
d->p31=s->p13; d->p32=s->p23; d->p33=s->p33; d->p34=s->p43; d->p35=s->p53; \
d->p21=s->p14; d->p22=s->p24; d->p23=s->p34; d->p24=s->p44; d->p25=s->p54; \
d->p11=s->p15; d->p12=s->p25; d->p13=s->p35; d->p14=s->p45; d->p15=s->p55; }

/*
 * rotate a face either way based on "dir".  swaps pointers when done, too.
 */

#define SWP(a,b) { struct face * tmp=a;a=b;b=tmp; }
#define ROTFACE(s,d,dir) \
   if ((dir)==0) ROTFACE_CW(s,d) else ROTFACE_CCW(s,d); SWP(s,d)

rubik5 :: rubik5( void )
{
    memory = new face[7];
    for ( int i = 0; i < 6; i++ )
        f[i] = &memory[i];
    spare = &memory[6];
    type = TYPE;
}

rubik5 :: ~rubik5( void )
{
    delete[] memory;
}
/*
 * the transformation format:
 *
 *    +--------+-----+-------+
 * bits:  | 7    5 |  4  | 3   0 |
 *    +--------+-----+-------+
 *    | zeros  | dir | layer |
 *    +--------+-----+-------+
 *
 * 1 bit for a direction, CW or CCW,
 * 4 bits to identify the layer affected:
 * 0,1,2,3,4 identify layers going back from the front face,
 * 5,6,7,8,9 identify layers going right from the left face,
 * a,b,c,d,e identify layers going down from the top.
 * 0,4,5,9,a,e also cause facial rotations.
 * the value f is equivalent to 0.
 */

char *
rubik5 :: transform( UCHAR transformation )
{
    int dir = ((transformation & 0x10) != 0) ? 1 : 0;
    switch ( transformation & 0x0f )
    {
    case 15: case 0:
        ROTFACE( f[1], spare, dir );
        SHIFTGRP1( f[2], f[4], f[3], f[0],5,1,1,5,dir);
        return "15 or 0, rotate front";
    case 1:
        SHIFTGRP1( f[2], f[4], f[3], f[0],4,2,2,4,dir);
        return "1, rotate front grp 2";
    case 2:
        SHIFTGRP1( f[2], f[4], f[3], f[0],3,3,3,3,dir);
        return "2, rotate front grp 3";
    case 3:
        SHIFTGRP1( f[2], f[4], f[3], f[0],2,4,4,2,dir);
        return "3, rotate front grp 4";
    case 4:
        SHIFTGRP1( f[2], f[4], f[3], f[0],1,5,5,1,dir);
        ROTFACE( f[5], spare, ( dir ^ 1 ));
        return "4, rotate back";
    case 5:
        ROTFACE( f[0], spare, dir );
        SHIFTGRP2( f[2], f[1], f[3], f[5],1,1,1,5,dir);
        return "5, rotate left";
    case 6:
        SHIFTGRP2( f[2], f[1], f[3], f[5],2,2,2,4,dir);
        return "6, rotate left grp 2";
    case 7:
        SHIFTGRP2( f[2], f[1], f[3], f[5],3,3,3,3,dir);
        return "7, rotate left grp 3";
    case 8:
        SHIFTGRP2( f[2], f[1], f[3], f[5],4,4,4,2,dir);
        return "8, rotate left grp 4";
    case 9:
        SHIFTGRP2( f[2], f[1], f[3], f[5],5,5,5,1,dir);
        ROTFACE( f[4], spare, dir ^ 1 );
        return "9, rotate right";
    case 10:
        ROTFACE( f[2], spare, dir );
        SHIFTGRP3( f[5], f[4], f[1], f[0],1,1,1,1,dir);
        return "10, rotate top";
    case 11:
        SHIFTGRP3( f[5], f[4], f[1], f[0],2,2,2,2,dir);
        return "11, rotate top grp 2";
    case 12:
        SHIFTGRP3( f[5], f[4], f[1], f[0],3,3,3,3,dir);
        return "12, rotate top grp 3";
    case 13:
        SHIFTGRP3( f[5], f[4], f[1], f[0],4,4,4,4,dir);
        return "13, rotate top grp 4";
    case 14:
        ROTFACE( f[2], spare, dir ^ 1 );
        SHIFTGRP3( f[5], f[4], f[1], f[0],5,5,5,5,dir);
        return "14, rotate bottom";
    }
    // should never get here.
    return "unknown operation\n";
}

void
rubik5 :: transform( UINT trans )
{
    transform( (UCHAR)(( trans >>  0) & 0x1f ));
    transform( (UCHAR)(( trans >>  5) & 0x1f ));
    transform( (UCHAR)(( trans >> 10) & 0x1f ));
    transform( (UCHAR)(( trans >> 15) & 0x1f ));
    transform( (UCHAR)(( trans >> 20) & 0x1f ));
    transform( (UCHAR)(( trans >> 25) & 0x1f ));
}

static char *
valtostr( int val )
{
    static char rets[150][10];
    static int retv = 0;
    char * r;

    r = rets[retv];

    retv = (retv + 1) % 150;

    if ( val )
        sprintf( r, "%c[7m  %c[m", 27, 27 );
    else
        sprintf( r, "  " );

    return r;
}

void
rubik5 :: print( void )
{

#define P5( face, r ) \
      valtostr(f[face]->p##r##1), valtostr(f[face]->p##r##2), \
      valtostr(f[face]->p##r##3), valtostr(f[face]->p##r##4), \
      valtostr(f[face]->p##r##5)
    printf( 
        "%c[H\n"
        "              +------------+              \n"
        "              | %s%s%s%s%s |              \n"
        "              | %s%s%s%s%s |              \n"
        "              | %s%s%s%s%s |              \n"
        "              | %s%s%s%s%s |              \n"
        "              | %s%s%s%s%s |              \n"
        "+------------+------------+------------+------------+ \n"
        "| %s%s%s%s%s | %s%s%s%s%s | %s%s%s%s%s | %s%s%s%s%s | \n"
        "| %s%s%s%s%s | %s%s%s%s%s | %s%s%s%s%s | %s%s%s%s%s | \n"
        "| %s%s%s%s%s | %s%s%s%s%s | %s%s%s%s%s | %s%s%s%s%s | \n"
        "| %s%s%s%s%s | %s%s%s%s%s | %s%s%s%s%s | %s%s%s%s%s | \n"
        "| %s%s%s%s%s | %s%s%s%s%s | %s%s%s%s%s | %s%s%s%s%s | \n"
        "+------------+------------+------------+------------+ \n"
        "              | %s%s%s%s%s |              \n"
        "              | %s%s%s%s%s |              \n"
        "              | %s%s%s%s%s |              \n"
        "              | %s%s%s%s%s |              \n"
        "              | %s%s%s%s%s |              \n"
        "              +------------+              \n"
        "\n",
        27,
        P5(2,1),P5(2,2),P5(2,3),P5(2,4),P5(2,5),
        P5(0,1),P5(1,1),P5(4,1),P5(5,1),
        P5(0,2),P5(1,2),P5(4,2),P5(5,2),
        P5(0,3),P5(1,3),P5(4,3),P5(5,3),
        P5(0,4),P5(1,4),P5(4,4),P5(5,4),
        P5(0,5),P5(1,5),P5(4,5),P5(5,5),
        P5(3,1),P5(3,2),P5(3,3),P5(3,4),P5(3,5) );

}

void
rubik5 :: fill( UCHAR * buf )
{
    const int f_size = sizeof( struct face );
    int last = buf[18];

    memset( memory, 0, 7 * f_size );

    memcpy( ((char*)f[0])+1, buf +  0, 3 );
    memcpy( ((char*)f[1])+1, buf +  3, 3 );
    memcpy( ((char*)f[2])+1, buf +  6, 3 );
    memcpy( ((char*)f[3])+1, buf +  9, 3 );
    memcpy( ((char*)f[4])+1, buf + 12, 3 );
    memcpy( ((char*)f[5])+1, buf + 15, 3 );

    *(UINT32*)f[0] = ntohl( *(UINT32*)f[0] ) | ((last << 24) & 0x01000000);
    *(UINT32*)f[1] = ntohl( *(UINT32*)f[1] ) | ((last << 23) & 0x01000000);
    *(UINT32*)f[2] = ntohl( *(UINT32*)f[2] ) | ((last << 22) & 0x01000000);
    *(UINT32*)f[3] = ntohl( *(UINT32*)f[3] ) | ((last << 21) & 0x01000000);
    *(UINT32*)f[4] = ntohl( *(UINT32*)f[4] ) | ((last << 20) & 0x01000000);
    *(UINT32*)f[5] = ntohl( *(UINT32*)f[5] ) | ((last << 19) & 0x01000000);

    extrabits = last & 192;
}

void
rubik5 :: empty( UCHAR * buf )
{
    buf[18] =
        ((*(UINT32*)f[0] & 0x01000000) >> 24) +
        ((*(UINT32*)f[1] & 0x01000000) >> 23) +
        ((*(UINT32*)f[2] & 0x01000000) >> 22) +
        ((*(UINT32*)f[3] & 0x01000000) >> 21) +
        ((*(UINT32*)f[4] & 0x01000000) >> 20) +
        ((*(UINT32*)f[5] & 0x01000000) >> 19) +
        extrabits;

    *(UINT32*)f[0] = htonl( *(UINT32*)f[0] );
    *(UINT32*)f[1] = htonl( *(UINT32*)f[1] );
    *(UINT32*)f[2] = htonl( *(UINT32*)f[2] );
    *(UINT32*)f[3] = htonl( *(UINT32*)f[3] );
    *(UINT32*)f[4] = htonl( *(UINT32*)f[4] );
    *(UINT32*)f[5] = htonl( *(UINT32*)f[5] );

    memcpy( buf +  0, ((char*)f[0])+1, 3 );
    memcpy( buf +  3, ((char*)f[1])+1, 3 );
    memcpy( buf +  6, ((char*)f[2])+1, 3 );
    memcpy( buf +  9, ((char*)f[3])+1, 3 );
    memcpy( buf + 12, ((char*)f[4])+1, 3 );
    memcpy( buf + 15, ((char*)f[5])+1, 3 );
}


bool
rubik5::encrypt( UCHAR * outbuf, UCHAR * inbuf, int numbytes )
{
    rubik5_key * mykey = (rubik5_key *)key;

    if ( key->type != rubik5_key::TYPE )
    {
        printf( "rubik5::encrypt: key argument is "
                    "not a rubik5 key! type = 0x%08x\n",
                    key->type );
        return false;
    }

    memcpy( outbuf, inbuf, numbytes );

    if ( numbytes < minsize() )
    {
        printf( "rubik5::encrypt: warning: cannot encrypt "
                    "buffer smaller than %d bytes\n", minsize() );
        return false;
    }

    int index;

    while ( numbytes >= minsize() )
    {
        fill( outbuf );
        for ( index = 0; index < mykey->numkeys; index++ )
            transform( mykey->keys[ index ]);
        empty( outbuf );

        numbytes -= minsize();
        outbuf   += minsize();
    }

    if ( numbytes > 0 )
    {
        outbuf -= minsize() - numbytes;

        fill( outbuf );
        for ( index = 0; index < mykey->numkeys; index++ )
            transform( mykey->keys[ index ]);
        empty( outbuf );
    }

    return true;
}

bool
rubik5::decrypt( UCHAR * outbuf, UCHAR * inbuf, int numbytes )
{
    rubik5_key * mykey = (rubik5_key *)key;

    if ( key->type != rubik5_key::TYPE )
    {
        printf(  "rubik5::encrypt: key argument is "
                     "not a rubik5 key! type = 0x%08x\n",
                     mykey->type );
        return false;
    }

    memcpy( outbuf, inbuf, numbytes );

    if ( numbytes < minsize() )
    {
        printf( "rubik5::decrypt: warning: cannot decrypt "
                    "buffer smaller than %d bytes\n", minsize() );
        return false;
    }

    int lastbytes = numbytes % 19;

    if ( lastbytes > 0 )
    {
        UCHAR * lastbuf = outbuf + numbytes - minsize();

        fill( lastbuf );
        for ( int index = 0; index < mykey->numkeys; index++ )
            transform( mykey->inverse_keys[ index ]);
        empty( lastbuf );
    }

    while ( numbytes >= minsize() )
    {
        fill( outbuf );
        for ( int index = 0; index < mykey->numkeys; index++ )
            transform( mykey->inverse_keys[ index ]);
        empty( outbuf );

        numbytes -= minsize();
        outbuf   += minsize();
    }

    return true;
}

rubik5_key :: rubik5_key( void )
{
    numkeys = 0;
    keys = inverse_keys = NULL;
    type = TYPE;
}

rubik5_key :: ~rubik5_key( void )
{
    if ( keys != NULL )
        delete[] keys;
    if ( inverse_keys != NULL )
        delete[] inverse_keys;
}

bool
rubik5_key::valid_keystring( char * keystring )
{
    if ( strncmp( keystring, "rubik5:",
                  strlen( "rubik5:" )) != 0 )
        return false;
    return true;
}

// textual key format:
//   numkeys,hexhexhexhexhex
// for instance:
//   4,00000000111111112222222233333333

bool
rubik5_key::key_parse( char * key )
{
    if ( strncmp( key, "rubik5:", 7 ) != 0 )
        return false;

    key += 7;

    if ( numkeys != 0 )
        delete[] keys;
    numkeys = atoi( key );
    if ( numkeys == 0 || numkeys > 200 )
    {
    fail:
        numkeys = 0;
        return false;
    }
    if (( key = strchr( key, ',' )) == NULL )
        goto fail;
    key++;
    UINT * local_keys = new UINT[numkeys];
    for ( int i = 0; i < numkeys; i++ )
    {
        UINT v = 0;
        for ( int j = 0; j < 8; j++ )
        {
            int c = *key++;
            if ( c >= 'a' && c <= 'f' )
                c -= 0x20;
            if ( c < '0' || c > 'F' )
                goto fail;
            if ( c > '9' && c < 'A' )
                goto fail;
            if ( c > '9' )
                c -= 7;
            c -= 0x30;
            v <<= 4;
            v += c;
        }
        local_keys[i] = v;
    }
    keys = new UINT[numkeys];
    memcpy( (UCHAR*) keys, (UCHAR*) local_keys, sizeof(UINT) * numkeys );
    delete local_keys;
    make_inverse();
    return true;
}

char *
rubik5_key::key_dump( void )
{
    char * ret, * cp;

    if ( numkeys == 0 )
        return "rubik5:0,0";

    cp = ret = new char[19 + (numkeys * 8)];

    sprintf( cp, "rubik5:%d,", numkeys );
    cp += strlen( cp );

    for ( int i = 0; i < numkeys; i++ )
    {
        sprintf( cp, "%08x", keys[i] );
        cp += strlen( cp );
    }

    return ret;
}

void
rubik5_key::random_key( int _numkeys )
{
    if ( numkeys != 0 )
        delete[] keys;
    numkeys = _numkeys;
    keys = new UINT[numkeys];
    for ( int i = 0; i < numkeys; i++ )
        keys[i] = random();
    make_inverse();
}

void
rubik5_key::make_inverse( void )
{
    if ( numkeys == 0 )
        return;
    if ( inverse_keys != NULL )
        delete[] inverse_keys;
    inverse_keys = new UINT[ numkeys ];
    for ( int i = 0; i < numkeys; i++ )
        inverse_keys[numkeys-i-1] = invertkey( keys[i] );
}

