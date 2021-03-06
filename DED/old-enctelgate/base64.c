
#include "base64.h"

static char value_to_b64[] = 
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static char b64_to_value_20[] = 
{
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
    -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
    -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51
};

#define VALUE_TO_B64(v) \
    ((((v)>=sizeof(value_to_b64))||((v)<0))?-1:value_to_b64[(int)(v)])
#define B64_TO_VALUE(v) \
    ((((v)<0x20)||((v)>=(0x20+sizeof(b64_to_value_20))))?\
        -1:b64_to_value_20[(int)(v)-0x20])

int
b64_is_valid_char( char c )
{
    if ( c == '=' )
        return 1;
    if ( B64_TO_VALUE(c) == -1 )
        return 0;
    return 1;
}

/* return 4 if ok, 0 if not ok */
int
b64_encode_quantum( char * in3, int in_len, char * out4 )
{
    int v,val;

    if ( in_len > 3  || in_len < 1 )
        return 0;

    /*
     * don't have to worry about sign, since three bytes
     * cannot reach the sign bit in a 31-bit signed number.
     */
    val = (int)((in3[0] << 16) + (in3[1] << 8) + in3[2]);

#define TRY(tlen,index,shift) \
    do { \
        if ( in_len > tlen ) \
        { \
            v = (val >> shift) & 0x3f; \
            v = VALUE_TO_B64( v ); \
            if ( v == -1 ) \
                return 0; \
            out4[index] = v; \
        } \
        else \
        { \
            out4[index] = '='; \
        } \
    } while (0)

    TRY(0,0,18);
    TRY(0,1,12);
    TRY(1,2,6);
    TRY(2,3,0);

    return 4;
}

/* return length of bytes decoded, or 0 if not ok */
int
b64_decode_quantum( char * in4, char * out3 )
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
