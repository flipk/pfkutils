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

#include <stdio.h>
#include <stdarg.h>

#include "Btree.h"

class myIterator : public BtreeIterator {
    FileBlockInterface * fbi;
public:
    myIterator(FileBlockInterface * _fbi) {
        fbi = _fbi;
        wantPrinting = true;
    }
    /*virtual*/ ~myIterator(void) { }
    /*virtual*/ bool handle_item( uint8_t * keydata, uint32_t keylen,
                                  FB_AUID_T data_fbn );
    /*virtual*/ void print( const char * format, ... )
        __attribute__ ((format( printf, 2, 3 )));
};

static void
display_hex(unsigned char * ptr, int size, bool prefix)
{
    for (int pos = 0; pos < size; pos++)
    {
        if (prefix)
            if ((pos & 31) == 0)
                printf("    ");
        printf("%02x", ptr[pos]);
        if ((pos & 3) == 3)
            printf(" ");
        if ((pos & 7) == 7)
            printf(" ");
        if ((pos & 31) == 31)
            printf("\n");
    }
    printf("\n");
}

bool
myIterator :: handle_item( uint8_t * keydata, uint32_t keylen,
                           FB_AUID_T data_fbn )
{
    printf("data @ %#x  key:", data_fbn);
    display_hex(keydata, keylen, false);

    FileBlock * fb = fbi->get(data_fbn);
    if (fb)
    {
        display_hex(fb->get_ptr(), fb->get_size(), true);
        fbi->release(fb);
    }

    return true;
}

void
myIterator :: print( const char * format, ... )
{
    va_list ap;
    va_start(ap,format);
    vprintf(format,ap);
    va_end(ap);
    printf("\n");
}

extern "C" int
btdump_main(int argc, char ** argv)
{
    if (argc != 2)
    {
        printf("usage: fbdump <dbfile>\n");
        return 1;
    }

    Btree * bt;

    bt = Btree::openFile(argv[1], 128 * 1024);

    if (!bt)
    {
        printf("unable to open file %s\n", argv[1]);
        return 1;
    }

    myIterator  myi(bt->get_fbi());

    bt->iterate( &myi );

    delete bt;

    return 0;
}
