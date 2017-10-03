
/** \file testBtree.C
 * \brief test harness for Btree and BtreeInternal objects
 * \author Phillip F Knaack
 */

#include "Btree.H"
// #include "Btree_internal.H"

#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

#define TEST 2

#define TEST_FILE "testfile.db"
#define MAX_BYTES (256*1024*1024)

class myIterator : public BtreeIterator {
    FileBlockInterface * fbi;
public:
    myIterator(FileBlockInterface * _fbi) { fbi = _fbi; }
    /*virtual*/ bool handle_item( UCHAR * keydata, UINT32 keylen,
                                  UINT32 data_fbn ) {
        printf( "key len %d: ", keylen);
        int i;
        for (i=0; i < (int)keylen; i++)
            printf("%02x", keydata[i]);
        printf("\ndata fbn: %08x: ", data_fbn);
#if 1 /* print data contents */
        FileBlock * fb = fbi->get(data_fbn);
        for (i=0; i < fb->get_size(); i++)
            printf("%02x", fb->get_ptr()[i]);
        fbi->release(fb);
#endif
        printf("\n");
        return true; // ok to continue iterating
    }
    /*virtual*/ void print( const char * format, ... )
        __attribute__ ((format( printf, 2, 3 ))) {
        va_list ap;
        va_start(ap,format);
        vprintf(format,ap);
        va_end(ap);
    }
};
    
#if TEST==1

struct crapkey {
    UINT32_t key;
    UCHAR * get_ptr (void) { return (UCHAR*) this; }
    int get_size (void) { return 4; }
};

struct crapdata {
    UINT32_t data;
    UCHAR * get_ptr (void) { return (UCHAR*) this; }
    int get_size (void) { return 4; }
};


struct ramcopy {
    bool infile;
    UINT32 data;
    ramcopy(void) { infile = false; }
};

int
main()
{
    int fd, options;

    unlink( TEST_FILE );
    options = O_RDWR | O_CREAT;
#ifdef O_LARGEFILE
    options |= O_LARGEFILE;
#endif
    fd = open( TEST_FILE, options, 0644 );
    if (fd < 0)
    {
        printf("file create error %d: %s\n", errno, strerror(errno));
        return 1;
    }

    PageIO * pageio = new PageIOFileDescriptor(fd);
    BlockCache * bc = new BlockCache( pageio, MAX_BYTES );
    FileBlockInterface::init_file(bc);
    FileBlockInterface * fbi = FileBlockInterface::open(bc);
    Btree::init_file( fbi, 13 );
    Btree * bt = Btree::open(fbi);

    BTDatum <crapkey>   key(bt);
    BTDatum <crapdata>  data(bt);

#define ITERATIONS 0x0100000
#define ITEMS      0x0010000

    ramcopy  ram[ITEMS];
    int iter, ind;
    int inserts = 0, queries = 0, deletes = 0;
    time_t now, last;

    time( &last );
    for (iter = 0; iter < ITERATIONS; iter++)
    {
        if (time(&now) != last)
        {
            last = now;
            fprintf(stderr, "\r %d ", iter);
        }
        ind = (random() >> 8) & (ITEMS-1);
        if (ram[ind].infile)
        {
            if ((random() & 0xff) > 0x80)
            {
                key.alloc();
                key.d->key.set(ind);
                if (bt->del( &key ) == false)
                    fprintf(stderr,"\r %d delete %d failed\n",iter,ind);
                ram[ind].infile = false;
                key.release();
                deletes++;
            }
            else
            {
                key.alloc();
                key.d->key.set(ind);
                if (bt->get( &key, &data ) == false)
                    fprintf(stderr,"\r %d query %d failed\n", iter, ind);
                else if (data.d->data.get() != ram[ind].data)
                    fprintf(stderr,"\r %d query %d data mismatch\n", iter, ind);
                key.release();
                data.release();
                queries++;
            }
        }
        else
        {
            ram[ind].data = random();
            key.alloc();
            data.alloc();
            key.d->key.set(ind);
            data.d->data.set(ram[ind].data);
            if (bt->put( &key, &data ) == false)
                fprintf(stderr,"\r %d put %d failed\n", iter, ind);
            else
                ram[ind].infile = true;
            key.release();
            data.release();
            inserts++;
        }
    }
    fprintf(stderr,
            "\ncompleted %d iterations; %d inserts, %d deletes, %d queries\n",
            ITERATIONS, inserts, deletes, queries);

    myIterator iterator(fbi);
    bt->iterate( &iterator );

    delete bt;
    delete fbi;
    delete bc;
    delete pageio;
    close(fd);

    return 0;
}

#elif TEST==2

int
main()
{
}

#endif
