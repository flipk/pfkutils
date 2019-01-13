#if 0
set -e -x
g++ utf16toutf8.cc -o utf16toutf8
exit 0
#endif

#include <string>
#include <iostream>
#include <stdio.h>
#include <inttypes.h>

using namespace std;


// TODO: write a function which can validate what type of string
//       a buffer is. e.g. if it contains 110xxxxx, 1110xxxx, or 11110xxx,
//       or it contains only 0xxxxxxx then it must be UTF8.
//       if it contains 11011xxx bytes then it must be UTF16, and
//       whether those patterns are in even or odd determines BE/LE.
//       if it contains FEFF / FFFE it must be UTF16BE / UTF16LE.
//       could look for UTF32 but who cares, does anyone use it?
//       should also verify if one of the telltale signs exists, that
//       none of the other telltales for the other type does NOT exist,
//       which would indicate malformedness.
//       actually, there are numerous signs of malformedness mentioned
//       in unicode 11 chapter 3 that could be checked.


// this conforms to unicode version 11, chapter 3 (sections 3.9 and 3.10)
void utf16_to_utf8(const std::string  &in, std::string &out,
                   bool big_endian = true)
{
    uint32_t surrogate_left = 0;
    out.clear();
    for (int in_pos = 0; in_pos < (in.length()-1); in_pos += 2)
    {
        uint32_t i, b0, b1;
        b0 = (uint32_t) (uint8_t) in[in_pos  ];
        b1 = (uint32_t) (uint8_t) in[in_pos+1];
        if (b0 == 0xFE && b1 == 0xFF) // BE BOM, zero width no break space
            big_endian = true;
        if (b0 == 0xFF && b1 == 0xFE) // LE BOM, zero width no break space
            big_endian = false;
        if (big_endian)
            i = (b0 << 8) + b1;
        else
            i = (b1 << 8) + b0;

        // look for surrogates
        // 1101 10xx xxxx xxxx  or   1101 11xx xxxx xxxx
        //  F    C    0   0

        uint32_t o = 0;
        if ((i & 0xF800) == 0xD800)
        {
            if ((i & 0x0400) == 0x0000) // LEFT
            {
                if (surrogate_left != 0)
                {
                    printf("OOPS two left surrogs in a row?\n");
                }
                surrogate_left = i;
                i = 0; // SKIP
            }
            else // RIGHT
            {
                if (surrogate_left == 0)
                {
                    printf("OOPS right surrog with no left surrog?\n");
                }
                i = i & 0x03FF;
                i += (surrogate_left & 0x3F) << 10;
                uint32_t w = (surrogate_left >> 6) & 0xF;
                uint32_t u = w + 1;
                i += (u << 16);
                surrogate_left = 0;
            }
        }
        else
        {
            if (surrogate_left != 0)
            {
                printf("OOPS left surrog with no right surrog?\n");
            }
            surrogate_left = 0;
        }

        if (i != 0)
        {
            if (i < 0x80)
            {
                // one byte UTF8
                o = i;
                out += (char) o;
            }
            else if (i < 0x800)
            {
                // two byte UTF8
                o = ((i >> 6) & 0x1F) + 0xC0;
                out += (char) o;
                o = ((i     ) & 0x3F) + 0x80;
                out += (char) o;
            }
            else if (i < 0x10000)
            {
                // three byte UTF8
                o = ((i >> 12) & 0xF) + 0xE0;
                out += (char) o;
                o = ((i >> 6) & 0x3F) + 0x80;
                out += (char) o;
                o = ((i     ) & 0x3F) + 0x80;
                out += (char) o;
            }
            else
            {
                // four byte UTF8
                o = ((i >> 18) & 0x07) + 0xF0;
                out += (char) o;
                o = ((i >> 12) & 0x3F) + 0x80;
                out += (char) o;
                o = ((i >>  6) & 0x3F) + 0x80;
                out += (char) o;
                o = ((i      ) & 0x3F) + 0x80;
                out += (char) o;
            }
        }
    }
    if (surrogate_left != 0)
    {
        printf("OOPS left surrog at end with no right surrog?\n");
    }
}



void print_hex(const char *prefix, const std::string &s)
{
    if (prefix)
        printf("%s", prefix);
    for (int i = 0; i < s.length(); i++)
    {
        uint8_t b = (uint8_t) s[i];
        printf("%02x ", b);
    }
    printf("\n");
}


// samples mentioned in unicode 11 chapter 3

const unsigned char seq_1[] =
{
    0xFE, 0xFF, 0x00, 0x4D, 0x04, 0x30, 0x4E, 0x8C, 0xD8, 0x00, 0xDF, 0x02
};
const unsigned char seq_2[] =
{
    0x00, 0x4D, 0x04, 0x30, 0x4E, 0x8C, 0xD8, 0x00, 0xDF, 0x02
};
const unsigned char seq_3[] =
{
    0xFF, 0xFE, 0x4D, 0x00, 0x30, 0x04, 0x8C, 0x4E, 0x00, 0xD8, 0x02, 0xDF
};

int
main()
{
    string i, o;

    i.assign((const char *)seq_1, sizeof(seq_1));
    utf16_to_utf8(i, o);
    print_hex("utf-16 in = ", i);
    print_hex("utf-8 out = ", o);
    printf(   "   expect = ef bb bf 4d d0 b0 e4 ba 8c f0 90 8c 82\n\n");

    i.assign((const char *)seq_2, sizeof(seq_2));
    utf16_to_utf8(i, o);
    print_hex("utf-16 in = ", i);
    print_hex("utf-8 out = ", o);
    printf(   "   expect = 4d d0 b0 e4 ba 8c f0 90 8c 82\n\n");

    i.assign((const char *)seq_3, sizeof(seq_3));
    utf16_to_utf8(i, o);
    print_hex("utf-16 in = ", i);
    print_hex("utf-8 out = ", o);
    printf(   "   expect = ef bb bf 4d d0 b0 e4 ba 8c f0 90 8c 82\n\n");

    return 0;
}
