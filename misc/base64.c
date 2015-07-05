
/*
    This file is part of the "pfkutils" tools written by Phil Knaack
    (pfk@pfk.org).
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

#if INCLUDE_BASE64_HEADERS

/* return 4 if ok, 0 if not ok */
int b64_encode_quantum( unsigned char * in3, int in_len,
                        unsigned char * out4 );

/* return 0 if ok, return -1 if error (out_len too small) */
int
b64_encode_buffer( unsigned char * in,  int in_len,
                   unsigned char * out, int * arg_out_len );

/* return length of bytes decoded, or 0 if not ok */
int b64_decode_quantum( unsigned char * in4, unsigned char * out3 );

/* return 0 if ok, return -1 if error
   (out_len too small or invalid b64 char) */
int
b64_decode_buffer( unsigned char * in,  int in_len,
                   unsigned char * out, int * arg_out_len );

#endif /* INCLUDE_BASE64_HEADERS */

#if INCLUDE_BASE64_IMPL

static unsigned char value_to_b64[] = 
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static unsigned char b64_to_value_20[] = 
{
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
    -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
    -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51
};

#define VALUE_TO_B64(v) \
    ((((v)>=sizeof(value_to_b64))||((v)<0))?-1:value_to_b64[(v)])
#define B64_TO_VALUE(v) \
    ((((v)<0x20)||((v)>=(0x20+sizeof(b64_to_value_20))))?\
        -1:b64_to_value_20[(v)-0x20])

/* return 4 if ok, 0 if not ok */
int
b64_encode_quantum( unsigned char * in3, int in_len,
                    unsigned char * out4 )
{
    int v,val;

    if ( in_len > 3  || in_len < 1 )
        return 0;

    val = (in3[0] << 16) + (in3[1] << 8) + in3[2];

#define TRY(tlen,index,shift) \
    do { if ( in_len > tlen ) { \
        v = (val >> shift) & 0x3f;  v = VALUE_TO_B64( v ); \
        if ( v == -1 ) return 0; \
        out4[index] = v; \
    } else {  \
        out4[index] = '='; \
    }} while (0)

    TRY(0,0,18);
    TRY(0,1,12);
    TRY(1,2,6);
    TRY(2,3,0);

    return 4;
}

/* return 0 if ok, return -1 if error (out_len too small) */
int
b64_encode_buffer( unsigned char * in,  int in_len,
                   unsigned char * out, int * arg_out_len )
{
    int out_len = *arg_out_len;
    int e_len, ed_len;
    int ret_out_len = 0;

    while (in_len > 0)
    {
        if (in_len > 3)
            e_len = 3;
        else
            e_len = in_len;

        if (out_len < 4)
            return -1;

        ed_len = b64_encode_quantum( in, e_len, out );
        if (ed_len == 0)
            return -1;

        out_len -= ed_len;
        out += ed_len;
        ret_out_len += ed_len;
        in_len -= e_len;
        in += e_len;
    }
    *arg_out_len = ret_out_len;
    return 0;
}

/* return length of bytes decoded, or 0 if not ok */
int
b64_decode_quantum( unsigned char * in4, unsigned char * out3 )
{
    int val=0,v;
    int ret;

    v = B64_TO_VALUE(in4[0]);
    if ( v == -1 ) return 0;
    val += (v << 18);

    v = B64_TO_VALUE(in4[1]);
    if ( v == -1 ) return 0;
    val += (v << 12);

    if ( in4[2] == '='   &&  in4[3] == '=' )
    {
        ret = 1;
        goto unpack;
    }

    v = B64_TO_VALUE(in4[2]);
    if ( v == -1 ) return 0;
    val += (v << 6);

    if ( in4[3] == '=' )
    {
        ret = 2;
        goto unpack;
    }

    v = B64_TO_VALUE(in4[3]);
    if ( v == -1 ) return 0;
    val += (v << 0);

    ret = 3;

 unpack:
    out3[0] = (val >> 16) & 0xff;
    out3[1] = (val >>  8) & 0xff;
    out3[2] = (val >>  0) & 0xff;

    return ret;
}

/* return 0 if ok, return -1 if error
   (out_len too small or invalid b64 char) */
int
b64_decode_buffer( unsigned char * in,  int in_len,
                   unsigned char * out, int * arg_out_len )
{
    int out_len = *arg_out_len;
    int e_len, ed_len;
    int ret_out_len = 0;

    while (in_len > 0)
    {
        if (in_len < 4)
            return -1;
        e_len = 4;

        if (out_len < 3)
            return -2;

        ed_len = b64_decode_quantum( in, out );
        if (ed_len == 0)
            return -3;

        out_len -= ed_len;
        out += ed_len;
        ret_out_len += ed_len;
        in_len -= e_len;
        in += e_len;
    }
    *arg_out_len = ret_out_len;
    return 0;
}

#endif /* INCLUDE_BASE64_IMPL */

#if INCLUDE_BASE64_TEST

/*

GET http://www.cnn.com/ HTTP/1.0
Proxy-authorization: Basic $string

*/

int
main()
{
    unsigned char * in = "user:password";
    unsigned char out[ 100 ];
    unsigned char backin[ 100 ];
    int in_len = strlen(in);
    int out_len = sizeof(out);
    int backin_len = sizeof(backin);
    int cc;

    cc = b64_encode_buffer(in, in_len, out, &out_len);

    printf("in = '%s'\n", in);
    printf("encode buffer returns %d, out_len = %d\n", cc, out_len);
    printf("out = '%s'\n", out);

    cc = b64_decode_buffer(out, out_len, backin, &backin_len);

    printf("decode buffer returns %d, backin_len = %d\n", cc, backin_len);
    printf("backin = '%s'\n", backin);

    return 0;
}

#endif /* INCLUDE_BASE64_TEST */
