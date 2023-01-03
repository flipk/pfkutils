
#ifndef __NEWBASE64_H__
#define __NEWBASE64_H__

#include <string>

enum Base64Variant
{
// RFC 3548 attempts to unify RFC 1421 and RFC 2045
//   obsoleted by RFC 4648
// RFC 4648 section 4 standard base64 (www etc)
//   no line breaks, padding with = optional
//   ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/
    BASE64_RFC4648_S4_STANDARD,

// RFC 4648 section 5 base64url (url and filename-safe)
//   no line breaks, padding with = optional
//   ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_
    BASE64_RFC4648_S5_BASE64URL,

// RFC 2045 base64 for MIME
//   lines no longer than 76, padding with = mandatory
//   ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/
//   non-decoding chars discarded
    BASE64_RFC2045_MIME,

// RFC 2152 UTF-7
//   no line breaks, no padding
//   ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/
    BASE64_RFC2152_UTF7,

// uuencode / uudecode (6 bit value plus 0x20 --> ascii)
//   note below, first char is space (0x20)
//   note also this code uses '=' as a normal char and thus
//   CANNOT pad things which are not a multiple of 3 bytes long.
//   (this implies the length value MUST be encoded outside the code.)
//    !"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_
    BASE64_UUENCODE,

// binhex4 (mac OS) excludes visually confusing chars
//   !"#$%&'()*+,-012345689@ABCDEFGHIJKLMNPQRSTUVXYZ[`abcdefhijklmpqr
    BASE64_BINHEX4,

// unix crypt ("B64")
//   ./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz
// GEDCOM 5.5 standard (geological data interchange)
//   <same as unix crypt>
    BASE64_B64_CRYPT,

// bcrypt (niels provos, david mazières, based on blowfish)
//   ./ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789
    BASE64_BCRYPT,

// GNU BASH
//   0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ@_
    BASE64_GNUBASH,

// RFC 4880 radix-64 for OpenPGP
//   lines no longer than 76, padding with = mandatory
//   ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/
//   ALSO requires radix-64 encoded 24-bit CRC
//   no
// xxencoding
//   +-0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz
//   no
// RFC 1421 privacy enhanced mail (deprecated)
//   lines no longer than 64, padding with = mandatory
//   ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/
//   non-decoding chars error
//   no
// RFC 3501 IMAP mailbox names
//   no line breaks, no padding
//   ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+,
//   no
// 6PACK
//   no!!
//
// NOTE : there are NO PLANS WHATSOEVER to support yEnc, either.

    __BASE64_NUM_VARIANTS,
    __BASE64_VARIANT_DEFAULT = BASE64_RFC4648_S4_STANDARD,
    __BASE64_VARIANT_FIRST = BASE64_RFC4648_S4_STANDARD,
    __BASE64_VARIANT_LAST  = BASE64_GNUBASH
};

class Base64
{
    const char * charset;
    char reverse[256];
    bool padding_required;
    Base64Variant variant;

public:
    static bool VERBOSE;
    // NOTE this returns NULL if the specified "variant" value is
    //      not a valid member of the Base64Variant enum.
    static const char * variant_name(Base64Variant  variant);

    Base64(Base64Variant  _variant = __BASE64_VARIANT_DEFAULT);

    // returns false if the specified variant is not valid;
    // leaves class configured for default.
    bool set_variant(Base64Variant  _variant = __BASE64_VARIANT_DEFAULT);
    Base64Variant get_variant(void) const { return variant; }

    void encode_quantum( const unsigned char * in3, int in_len,
                         char * out4 );
    // encoding can fail if the output buffer isn't big enough.
    // in: destlen is the size of the output buffer.
    // out: destlen is the number of bytes consumed.
    // if the input is not padded, the output will be srclen*4/3;
    // if it is padded (in those codings that allow padding),
    // it could be 1 or 2 shorter than that.
    bool encode(char *dest, int &destlen,
                const unsigned char *src, int srclen);
    // encoding into a string normally shouldn't fail because
    // the destination is resized to sufficient size.
    bool encode(std::string &dest, const std::string &src);


    // returns false if the specified character is not valid
    // in the configured variant.
    inline bool valid_char(char c);
    // returns # of chars decoded, 0 if invalids found
    int decode_quantum( const char * in4,
                        unsigned char * out3 );
    // decode will return false if a character in the input
    // is not a valid base64 character in the configured variant.
    // NOTE when decoding BASE64_UUENCODE variant, the returned length
    // will always round up to the next multiple of three, because
    // '=' is part of the alphabet and therefore padding can't be used
    // to encode the fractional 4-to-3 decoding.
    bool decode(unsigned char *dest, int &destlen,
                const char *src, int srclen);
    bool decode(std::string &dest, const std::string &src);
};




// inline implementations

inline bool
Base64 :: valid_char(char c)
{
    if (reverse[(unsigned char) c] == 0xFF)
        return false;
    return true;
}

#endif /* __NEWBASE64_H__ */
