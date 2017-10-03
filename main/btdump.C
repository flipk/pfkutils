
#include <stdio.h>
#include <stdarg.h>

#include "Btree.H"

class myIterator : public BtreeIterator {
    FileBlockInterface * fbi;
public:
    myIterator(FileBlockInterface * _fbi) { fbi = _fbi; }
    /*virtual*/ ~myIterator(void) { }
    /*virtual*/ bool handle_item( UCHAR * keydata, UINT32 keylen,
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
myIterator :: handle_item( UCHAR * keydata, UINT32 keylen,
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
