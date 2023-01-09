
#include "newbase64.h"
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

void test_quantums(void)
{
    int success = 0, fail = 0;

    Base64  b;
    for (int vctr = (int) BASE64_RFC4648_S4_STANDARD;
         vctr != (int) __BASE64_NUM_VARIANTS;
         vctr++)
    {
        Base64Variant v = (Base64Variant) vctr;
        b.set_variant(v);

        for (int ind = 0; ind < 1000; ind++)
        {
            unsigned char in3[3] = {
                (unsigned char) (random() & 0xFF),
                (unsigned char) (random() & 0xFF),
                (unsigned char) (random() & 0xFF)
            };
            char out4[4];
            b.encode_quantum(in3, 3, out4);
            unsigned char out3[3];
            b.decode_quantum(out4, out3);

            if (memcmp(in3, out3, 3) != 0)
            {
                printf("FAIL: v=%d: ", vctr);
                printf("%08x %08x %08x -> '%c%c%c%c' -> %08x %08x %08x\n",
                       in3[0], in3[1], in3[2],
                       out4[0], out4[1], out4[2], out4[3],
                       out3[0], out3[1], out3[2]);
                fail++;
            }
            else
                success++;
        }
    }
    printf("newbase64 quantum tests: %d passed, %d failed\n",
           success, fail);
}

void _test_encode_buf(Base64 &b,
                      const unsigned char *_src,
                      int srclen,
                      int &success, int &fail)
{
    int i;
    int destlen;
    char dest[512];
    int outlen;
    unsigned char out[130];
    const unsigned char *src = _src;

    if (Base64::VERBOSE)
    {
        printf("in = (%d) ", srclen);
        for (i = 0; i < srclen; i++)
            printf("%02x ", src[i]);
        printf("\n");
    }

    destlen = sizeof(dest);
    if (b.encode(dest, destlen, src, srclen) == false)
    {
        if (Base64::VERBOSE)
            printf("%d: ERROR ENCODING\n", (int) b.get_variant());
        fail ++;
        return;
    }

    if (Base64::VERBOSE)
    {
        printf("encoded = (%d) '", destlen);
        for (i = 0; i < destlen; i++)
            printf("%c", dest[i]);
        printf("'\n");
    }

    outlen = sizeof(out);
    memset(out, 0x6c, outlen);
    if (b.decode(out, outlen, dest, destlen) == false)
    {
        if (Base64::VERBOSE)
            printf("%d: ERROR DECODING\n", (int) b.get_variant());
        fail ++;
        return;
    }

    if (Base64::VERBOSE)
    {
        printf("out = (%d) ", outlen);
        for (i = 0; i < outlen; i++)
            printf("%02x ", out[i]);
        printf("\n");
    }

    bool ret = memcmp(_src, out, srclen) == 0;
    if (ret)
        success ++;
    else
    {
        printf("%d: ERROR MISMATCH (%d, %d)\n",
               (int) b.get_variant(), srclen, outlen);
        fail++;
    }
}

void test_encode_buf(void)
{
    int success = 0, fail = 0;
    Base64   b;
    unsigned char buf[128];

    for (int vctr = (int) BASE64_RFC4648_S4_STANDARD;
         vctr != (int) __BASE64_NUM_VARIANTS;
         vctr++)
    {
        Base64Variant v = (Base64Variant) vctr;
        b.set_variant(v);

        for (int repcnt = 0; repcnt < 1000; repcnt++)
        {
            int len = (random() % 127) + 1;
            for (int ind = 0; ind < len; ind++)
                buf[ind] = (unsigned char) (random() & 0xFF);
            _test_encode_buf(b, buf, len, success, fail);
        }
    }

    printf("newbase64 raw encode tests: %d passed, %d failed\n",
           success, fail);
}


void _test_encode_str(Base64 &b,
                      const std::string &src,
                      int &success, int &fail)
{
    std::string  dest, out;
    int i;

    if (Base64::VERBOSE)
    {
        printf("src = (%d) (%d) ", (int) b.get_variant(), (int) src.size());
        for (i = 0; i < src.size(); i++)
            printf("%02x ", (unsigned char) src[i]);
        printf("\n");
    }

    if (b.encode(dest, src) == false)
    {
        if (Base64::VERBOSE)
            printf("%d: ERROR ENCODING\n", (int) b.get_variant());
        fail ++;
        return;
    }

    if (Base64::VERBOSE)
    {
        printf("encoded = (%d) '", (int) dest.size());
        for (i = 0; i < dest.size(); i++)
            printf("%c", dest[i]);
        printf("'\n");
    }

    if (b.decode(out, dest) == false)
    {
        if (Base64::VERBOSE)
            printf("%d: ERROR DECODING\n", (int) b.get_variant());
        fail ++;
        return;
    }

    if (Base64::VERBOSE)
    {
        printf("out = (%d) ", (int) out.size());
        for (i = 0; i < out.size(); i++)
            printf("%02x ", (unsigned char) out[i]);
        printf("\n");
    }

    if (b.get_variant() == BASE64_UUENCODE)
    {
        // UUENCODE can't encode length in a smaller than
        // quantum size, so the decoder always returns extra
        // garbage if the src was not a multiple of 3 bytes long.
        if ((out.size() - src.size()) < 3)
            out.resize(src.size());
    }

    if (src == out)
    {
        success ++;
    }
    else
    {
        printf("%d: ERROR MISMATCH\n", (int) b.get_variant());
        fail++;
    }
}

void test_encode_str(void)
{
    int success = 0, fail = 0;
    Base64  b;
    std::string  buf;

    for (int vctr = (int) BASE64_RFC4648_S4_STANDARD;
         vctr != (int) __BASE64_NUM_VARIANTS;
         vctr++)
    {
        Base64Variant v = (Base64Variant) vctr;
        b.set_variant(v);

        for (int repcnt = 0; repcnt < 1000; repcnt++)
        {
            int len = (random() % 127) + 1;
            buf.resize(len);
            for (int ind = 0; ind < len; ind++)
                buf[ind] = (unsigned char) (random() & 0xFF);
            _test_encode_str(b, buf, success, fail);
        }
    }

    printf("newbase64 str encode tests: %d passed, %d failed\n",
           success, fail);
}


int main()
{
    srandom(time(NULL) * getpid());

    test_quantums();
    test_encode_buf();
    test_encode_str();

    return 0;
}
