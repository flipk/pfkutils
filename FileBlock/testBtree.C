
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

#include "Btree_internal.H"

#define STORE 1

int
main()
{
    int fd, options;

#if STORE
    (void) unlink( TEST_FILE );
#endif

    options = O_RDWR
#if STORE
        | O_CREAT
#endif
        ;
#ifdef O_LARGEFILE
    options |= O_LARGEFILE;
#endif
    fd = open( TEST_FILE, options, 0644 );
    if ( fd < 0 )
    {
        fprintf(stderr, "open: %s\n", strerror(errno));
        return 1;
    }

    PageIO * pageio = new PageIOFileDescriptor(fd);
    BlockCache * bc = new BlockCache( pageio, MAX_BYTES );
#if STORE
    FileBlockInterface::init_file(bc);
#endif
    FileBlockInterface * fbi = FileBlockInterface::open(bc);
    Btree::init_file( fbi, 5 );
    if (Btree::valid_file( fbi ))
        printf("file is a valid btree file\n");
    else
        printf("file is NOT a valid btree file\n");

#if STORE
    BTNode * n = new BTNode( fbi, 5, 0 );

    n->numitems = 4;
    n->ptrs[0] = 1;
    n->ptrs[1] = 2;
    n->ptrs[2] = 3;
    n->ptrs[3] = 4;
    n->ptrs[4] = 5;
    n->datas[0] = 6;
    n->datas[1] = 7;
    n->datas[2] = 8;
    n->datas[3] = 9;
    n->keys[0] = new(32) BTKey(32);
    memset( n->keys[0]->data, 10, 32 );
    n->keys[1] = new(32) BTKey(32);
    memset( n->keys[1]->data, 11, 32 );
    n->keys[2] = new(32) BTKey(32);
    memset( n->keys[2]->data, 12, 32 );
    n->keys[3] = new(39) BTKey(39);
    memset( n->keys[3]->data, 13, 39 );
    n->mark_dirty();

    n->store();

    printf( "block id for new node is %08x\n", n->get_fbn() );

    delete n;
#else
    BTNode * n = new BTNode( fbi, 5, 0x19495cff );

    int i, j;
    printf( "numitems = %d\n", n->numitems );
    for (i=0; i < n->numitems+1; i++)
        printf("ptr %d = %d\n", i, n->ptrs[i]);
    for (i=0; i < n->numitems; i++)
        printf("datas %d = %d\n", i, n->datas[i]);
    for (i=0; i < n->numitems; i++)
    {
        printf("key %d = len %d : ", i, n->keys[i]->keylen);
        for (j=0; j < (int)n->keys[i]->keylen; j++)
            printf("%02x ", n->keys[i]->data[j]);
        printf("\n");
    }

    delete n;
#endif    


#if 1
    Btree * bt = Btree::open( fbi );

    myIterator iter(fbi);
    bt->iterate( &iter );

    delete bt;
#endif
    

    delete fbi;
    delete bc;
    delete pageio;
    close(fd);

    return 0;
}

#elif TEST==2

struct crapkey {
    UINT32_t key;
    UCHAR * get_ptr (void) { return (UCHAR*) this; }
    int get_size (void) { return 4; }
    static int get_max_size (void) { return 4; }
};

struct crapdata {
    UINT32_t data;
    UCHAR * get_ptr (void) { return (UCHAR*) this; }
    int get_size (void) { return 4; }
    static int get_max_size (void) { return 4; }
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

#endif
