
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

#define TEST 1

#define TEST_FILE "testfile.db"
#define MAX_BYTES (16*1024*1024)
    
#if TEST==1

#include "Btree_internal.H"

#define STORE 1

class myIterator : public BtreeIterator {
public:
    /*virtual*/ bool handle_item( UCHAR * keydata, UINT32 keylen,
                                  UINT32 data_fbn ) {
        printf( "key len %d: ", keylen);
        UINT32 i;
        for (i=0; i < keylen; i++)
            printf("%02x ", keydata[i]);
        printf("\ndata fbn: %08x\n", data_fbn);
        // print contents of data?
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

    myIterator iter;
    bt->iterate( &iter );

    delete bt;
#endif
    

    delete fbi;
    delete bc;
    delete pageio;
    close(fd);

    return 0;
}
#endif
