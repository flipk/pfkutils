#if 0
set -e -x
g++ -Wall -Werror -DINCLUDE_BASE64_TEST_MAIN base64.cc -o base64_test
./base64_test
exit 0
;;
#endif

/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
*/

#include "base64.h"

static inline unsigned char
value_to_b64( int v )
{
    static unsigned char table[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    if ( v >= (int)sizeof(table)   ||   v < 0 )
        return -1;
    return table[ v ];
}

static inline int
b64_to_value( unsigned char b64 )
{
    static signed char table[] = {   /* table starts with char 0x20 */
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
        -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
        -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51
    };
    if ( b64 < 0x20 )
        return -1;
    b64 -= 0x20;
    if ( b64 >= sizeof(table) )
        return -1;
    return table[ b64 ];
}

int
b64_is_valid_char( unsigned char c )
{
    if ( c == '=' )
        return 1;
    if ( b64_to_value(c) == -1 )
        return 0;
    return 1;
}

/* return 4 if ok, 0 if not ok */
int
b64_encode_quantum( const unsigned char * in3, int in_len,
                    char * out4 )
{
    int v;
    unsigned int val;

    if ( in_len > 3  || in_len < 1 )
        return 0;

    val = (in3[0] << 16);
    if (in_len > 1)
        val |= (in3[1] << 8);
    if (in_len == 3)
        val |= in3[2];

#define TRY(tlen,index,shift)                   \
    do {                                        \
        if ( in_len > tlen )                    \
        {                                       \
            v = (val >> shift) & 0x3f;          \
            v = value_to_b64( v );              \
            if ( v == -1 )                      \
                return 0;                       \
            out4[index] = v;                    \
        }                                       \
        else                                    \
        {                                       \
            out4[index] = '=';                  \
        }                                       \
    } while (0)

    TRY(0,0,18);
    TRY(0,1,12);
    TRY(1,2,6);
    TRY(2,3,0);

    return 4;
}

/* return length of bytes decoded, or 0 if not ok */
int
b64_decode_quantum( const char * in4, unsigned char * out3 )
{
    int val=0,v;
    int ret;

    v = b64_to_value(in4[0]);
    if ( v == -1 ) return 0;
    val += (v << 18);

    v = b64_to_value(in4[1]);
    if ( v == -1 ) return 0;
    val += (v << 12);

    if ( in4[2] == '='   &&  in4[3] == '=' )
    {
        ret = 1;
        goto unpack;
    }

    v = b64_to_value(in4[2]);
    if ( v == -1 ) return 0;
    val += (v << 6);

    if ( in4[3] == '=' )
    {
        ret = 2;
        goto unpack;
    }

    v = b64_to_value(in4[3]);
    if ( v == -1 ) return 0;
    val += (v << 0);

    ret = 3;

 unpack:
    out3[0] = (val >> 16) & 0xff;
    out3[1] = (val >>  8) & 0xff;
    out3[2] = (val >>  0) & 0xff;

    return ret;
}

// init destlen to sizeof dest buffer, on return
// it will be <= that size. destlen must be 4/3 bigger
// than srclen, or this returns false.
bool b64_encode(char *dest, int &destlen,
                const unsigned char *src, int srclen)
{
    int destleft = destlen;
    destlen = 0;

    for (int inleft = srclen; inleft > 0; inleft -= 3)
    {
        if (destleft < 4)
            // not big enough
            return false;
        int inlen = inleft;
        if (inlen > 3)
            inlen = 3;
        b64_encode_quantum(src, inlen, dest);
        src += 3;
        dest += 4;
        destleft -= 4;
        destlen += 4;
    }

    return true;
}

// init destlen to sizeof dest buffer, on return
// it will be <= that size. destlen must be at least
// 3/4 of srclen, or this returns false.
// also returns false if src contains non-base64 chars.
bool b64_decode(unsigned char *dest, int &destlen,
                const char *src, int srclen)
{
    int destleft = destlen;
    destlen = 0;

    for (int inleft = srclen; inleft > 0; inleft -= 4)
    {
        if (destleft < 3)
            return false;
        if (inleft < 4)
            return false;
        int outlen = b64_decode_quantum(src, dest);
        if (outlen == 0)
            return false;
        src += 4;
        dest += outlen;
    }

    return true;
}

void b64_encode(std::string &dest, const std::string &src)
{
    char out4[4];
    dest.clear();
    dest.reserve((src.size()+4) * 4 / 3);
    unsigned char *in3 = (unsigned char *) src.c_str();

    for (int inleft = src.size(); inleft > 0; inleft -= 3)
    {
        int inlen = inleft;
        if (inlen > 3)
            inlen = 3;
        b64_encode_quantum(in3, inlen, out4);
        in3 += 3;
        dest.append(out4, 4);
    }
}

bool b64_decode(std::string &dest, const std::string &src)
{
    unsigned char out3[3];
    dest.clear();
    dest.reserve(src.size() * 3 / 4);
    const char * in = src.c_str();

    for (int inleft = src.size(); inleft > 0; inleft -= 4)
    {
        if (inleft < 4)
            return false;
        int outlen = b64_decode_quantum(in, out3);
        if (outlen == 0)
            return false;
        in += 4;
        dest.append((char*) out3, outlen);
    }

    return true;
}

#ifdef INCLUDE_BASE64_TEST_MAIN

#include <string.h>

void test_strings(void)
{
    printf("running std::string test\n");

    std::string  in = "This is a test of some data.";
    std::string out;

    b64_encode(out, in);
    printf("input : %d of %d '%s'\noutput: %d of %d '%s'\n",
           (int) in.size(),  (int) in.capacity(),  in.c_str(),
           (int) out.size(), (int) out.capacity(), out.c_str());

    std::string out2;
    if (b64_decode(out2, out) == false)
        printf("ERROR: unable to decode\n");
    else
        printf("output: %d of %d '%s'\n",
               (int) out2.size(), (int) out2.capacity(), out2.c_str());
}

void test_chars(void)
{
    printf("running char* test\n");

    // some casting is necessary in this test case
    // because we're actually encoding text to text and
    // decoding text to text, instead of encoding binary to text
    // and text to binary.

    const char *in = "This is a test of some data.";
    int inlen = strlen(in); // include nul
    char out[80];
    int outlen;
    char out2[80];
    int out2len;

    outlen = sizeof(out);
    b64_encode(out, outlen, (const unsigned char*) in, inlen);
    printf("input :          '%s'\n"
           "output:          '%s'\n",
           in, out);

    // NOTE we did not encode the trailing nul, so the decode
    //      will not write one back.
    memset(out2,0,sizeof(out2));

    out2len = sizeof(out2);
    if (b64_decode((unsigned char*) out2, out2len, out, outlen) == false)
        printf("ERROR: cannot decode\n");
    else
        printf("output:          '%s'\n", out2);
}


int main(int argc, char ** argv)
{
    test_chars();
    test_strings();
    return 0;
}

#endif
