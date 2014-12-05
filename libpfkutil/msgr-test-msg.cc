
#include "msgr.h"
#include <stdio.h>

int
main()
{
    uint8_t  bigbuf[0x100];
    uint32_t bigbuflen = 0;
    uint32_t len;
    uint32_t consumed;
    uint8_t * ptr;
    PfkMsg  m;

//msg one
    m.init(0x0103, bigbuf + bigbuflen, sizeof(bigbuf) - bigbuflen);
    m.add_val((uint8_t)  1);
    m.add_val((uint16_t) 2);
    m.add_val((uint32_t) 3);
    m.add_val((uint64_t) 4);
    bigbuflen += m.finish();

//msg two
    m.init(0x0107, bigbuf + bigbuflen, sizeof(bigbuf) - bigbuflen);
    m.add_val((uint16_t) 0xfedc);
    m.add_val((uint16_t) 0x1234);
    m.add_val((uint32_t) 0x56788765);
    bigbuflen += m.finish();

//msg three
    m.init(0x010f, bigbuf + bigbuflen, sizeof(bigbuf) - bigbuflen);
    m.add_val((uint8_t) 0xdc);
    m.add_field(16, (void*)"this is a test");
    bigbuflen += m.finish();

//debug dump
    printf("encoded: ");
    for (uint32_t i = 0; i < bigbuflen; i++)
        printf("%02x ", bigbuf[i]);
    printf("\n");

//init for decode
    ptr = bigbuf;
    len = bigbuflen;

    while (len > 0)
    {
        if (m.parse(ptr, len, &consumed) == true)
        {
            printf("\ngot msg with %u fields\n", m.get_num_fields());
            for (uint32_t fld = 0; fld < m.get_num_fields(); fld++)
            {
                uint16_t fldlen;
                uint8_t * f = (uint8_t *) m.get_field(&fldlen, fld);
                if (f == NULL)
                    printf("can't fetch field %u\n", fld);
                else {
                    printf("field %u: ", fld);
                    for (uint32_t fldpos = 0; fldpos < fldlen; fldpos++)
                        printf("%02x ", f[fldpos]);
                    printf("\n");
                }
            }
        }
        else
            printf("parse returned false\n");
        len -= consumed;
        ptr += consumed;
        printf("consumed = %d remaining len = %d\n", consumed, len);
    }

    printf("\n");

    return 0;
}
