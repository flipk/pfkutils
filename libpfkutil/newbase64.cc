
#include "newbase64.h"
#include <string.h>

bool Base64 :: VERBOSE = false;

Base64 :: Base64(Base64Variant  _variant /*= __BASE64_VARIANT_DEFAULT*/)
{
    set_variant(_variant);
}

bool
Base64 :: set_variant(Base64Variant _variant
                      /*= __BASE64_VARIANT_DEFAULT*/)
{
    padding_required = false;
    switch (_variant)
    {
    case BASE64_RFC2045_MIME:
        padding_required = true;
        //fallthru
    case BASE64_RFC4648_S4_STANDARD:
    case BASE64_RFC2152_UTF7:
        charset =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdef"
            "ghijklmnopqrstuvwxyz0123456789+/";
        break;

    case BASE64_RFC4648_S5_BASE64URL:
        charset =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdef"
            "ghijklmnopqrstuvwxyz0123456789-_";
        break;

    case BASE64_UUENCODE:
        charset =
            " !\"#$%&'()*+,-./0123456789:;<=>?"
            "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_";
        break;

    case BASE64_BINHEX4:
        charset =
            "!\"#$%&'()*+,-012345689@ABCDEFGHI"
            "JKLMNPQRSTUVXYZ[`abcdefhijklmpqr";
        break;

    case BASE64_B64_CRYPT:
        charset =
            "./0123456789ABCDEFGHIJKLMNOPQRST"
            "UVWXYZabcdefghijklmnopqrstuvwxyz";
        break;

    case BASE64_BCRYPT:
        charset =
            "./ABCDEFGHIJKLMNOPQRSTUVWXYZabcd"
            "efghijklmnopqrstuvwxyz0123456789";
        break;

    case BASE64_GNUBASH:
        charset =
            "0123456789abcdefghijklmnopqrstuv"
            "wxyzABCDEFGHIJKLMNOPQRSTUVWXYZ@_";
        break;

    default:
        set_variant(__BASE64_VARIANT_DEFAULT);
        return false;
    }
    variant = _variant;

    memset(reverse, 0xFF, sizeof(reverse));
    for (char i = 0; i < 64; i++)
        reverse[charset[(int)i]] = i;

    return true;
}

const char *
Base64 :: variant_name(Base64Variant  variant)
{
    switch (variant)
    {
#define _N(m)  case m : return #m
        _N(BASE64_RFC4648_S4_STANDARD);
        _N(BASE64_RFC4648_S5_BASE64URL);
        _N(BASE64_RFC2045_MIME);
        _N(BASE64_RFC2152_UTF7);
        _N(BASE64_UUENCODE);
        _N(BASE64_BINHEX4);
        _N(BASE64_B64_CRYPT);
        _N(BASE64_BCRYPT);
        _N(BASE64_GNUBASH);
#undef  _N
    }
    return NULL;
}


void
Base64 :: encode_quantum( const unsigned char * in3, int in_len,
                          char * out4 )
{
    unsigned int v;
    unsigned int val;
    val = (in3[0] << 16);
    if (in_len > 1)
        val |= (in3[1] << 8);
    if (in_len == 3)
        val |= (in3[2] << 0);

#define TRY(tlen,index,shift)                   \
    do {                                        \
        if ( in_len > tlen )                    \
        {                                       \
            v = (val >> shift) & 0x3f;          \
            out4[index] = charset[v];           \
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

#undef TRY
}

bool
Base64 :: encode(char *dest, int &destlen,
                 const unsigned char *src, int srclen)
{
    int i;
    int outputlen = ((srclen * 4) + 2) / 3;
    if (outputlen > destlen)
        return false;

    if (Base64::VERBOSE)
        printf("srclen %d -> calc outputlen %d\n", srclen, outputlen);

    while (srclen > 0)
    {
        int thislen = srclen;
        if (thislen > 3)
            thislen = 3;

        encode_quantum(src, thislen, dest);
        if (Base64::VERBOSE)
        {
            printf("(%d) ", thislen);
            for (i = 0; i < thislen; i++)
                printf("%02x ", src[i]);
            printf("-> ");
            for (i = 0; i < 4; i++)
                printf("%02x ", dest[i]);
            printf("\n");
        }
        src += 3;
        dest += 4;
        srclen -= thislen;
    }

    // in most cases, the output len is as calculated.
    destlen = outputlen;

    // but sometimes, we must add some '='. note encode_quantum
    // already did, we just have to increment length.
    if (padding_required)
    {
        while ((destlen % 4) != 0)
            destlen++;
    }

    if (Base64::VERBOSE)
        printf("destlen = %d, actual outputlen = %d\n", destlen, outputlen);

    return true;
}

bool
Base64 :: encode(std::string &dest, const std::string &src)
{
    bool ret = false;
    int destlen = src.size() * 4 / 3;
    destlen += 8; // extra just in case
    dest.resize(destlen);
    if (encode((char*) dest.c_str(), destlen,
               (const unsigned char *) src.c_str(), src.size()))
    {
        dest.resize(destlen);
        ret = true;
    }
    else
        dest.resize(0);
    return ret;
}

int
Base64 :: decode_quantum( const char * in4,
                          unsigned char * out3 )
{
    int val=0,v;
    int ret;

    v = reverse[in4[0]];
    if ( v == 0xFF )
        return 0;
    val += (v << 18);

    v = reverse[in4[1]];
    if ( v == -1 )
        return 0;
    val += (v << 12);

    // UUENCODE standard is the only standard in which
    // '=' is a valid encoding character, so treat
    // it special.
    if (variant != BASE64_UUENCODE)
    {
        if ( in4[2] == '='   &&  in4[3] == '=' )
        {
            ret = 1;
            goto unpack;
        }
    }

    v = reverse[in4[2]];
    if ( v == -1 )
        return 0;
    val += (v << 6);

    if (variant != BASE64_UUENCODE)
    {
        if ( in4[3] == '=' )
        {
            ret = 2;
            goto unpack;
        }
    }

    v = reverse[in4[3]];
    if ( v == -1 )
        return 0;
    val += (v << 0);

    ret = 3;

 unpack:
    out3[0] = (val >> 16) & 0xff;
    out3[1] = (val >>  8) & 0xff;
    out3[2] = (val >>  0) & 0xff;

    return ret;
}


bool
Base64 :: decode(unsigned char *dest, int &destlen,
                 const char *src, int srclen)
{
    char tempsrc[4];
    int outputlen = srclen * 3 / 4;
    if (outputlen > destlen)
        return false;

    destlen = 0;
    while (srclen > 0)
    {
        int thislen = srclen;
        if (thislen > 4)
            thislen = 4;
        const char * srcptr = src;
        if (thislen < 4)
        {
            memset(tempsrc, '=', 4);
            memcpy(tempsrc, src, thislen);
            srcptr = tempsrc;
        }
        int retlen = decode_quantum(srcptr, dest);
        if (Base64::VERBOSE)
            printf("(%d) %02x %02x %02x %02x -> (%d) %02x %02x %02x\n",
                   thislen,
                   (unsigned char) srcptr[0],
                   (unsigned char) srcptr[1],
                   (unsigned char) srcptr[2],
                   (unsigned char) srcptr[3],
                   retlen, dest[0], dest[1], dest[2]);
        if (retlen == 0)
            return false;
        src += 4;
        srclen -= 4;
        dest += 3;
        destlen += retlen;
    }

    return true;
}

bool
Base64 :: decode(std::string &dest, const std::string &src)
{
    bool ret = false;
    int destlen = src.size() * 3 / 4;
    destlen += 8; // just in case
    dest.resize(destlen);
    if (decode((unsigned char *) dest.c_str(), destlen,
               (const char *) src.c_str(), src.size()))
    {
        dest.resize(destlen);
        ret = true;
    }
    else
        dest.resize(0);

    return ret;
}
