
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
#define MAX_BYTES (16*1024*1024)

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
//        FileBlock * fb = fbi->get(data_fbn);
//        for (i=0; i < fb->get_size(); i++)
//            printf("%02x", fb->get_ptr()[i]);
//        fbi->release(fb);
        printf("\n");
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

struct fileid {
    char prefix;
    UINT32_t id;
    //
    fileid(void) { prefix = 'i'; }
    UCHAR * get_ptr (void) { return (UCHAR*) this; }
    int get_size (void) { return 5; }
    static int get_max_size (void) { return 5; }
};
struct filecount {
    char prefix;
    UINT32_t count;
    //
    filecount(void) { prefix = 'c'; }
    UCHAR * get_ptr (void) { return (UCHAR*) this; }
    int get_size (void) { return 5; }
    static int get_max_size (void) { return 5; }
};
struct fileinfo {
    UINT32_t id;
    UINT32_t count;
    UINT64_t size;
    char name[512];
    struct fileinfo * next;
    //
    UCHAR * get_ptr (void) { return (UCHAR*) this; }
    int get_size (void) {
        return  
            sizeof(fileinfo)
            - 4  // account for 'next'
            - sizeof(name) + strlen(name) // length of name.
            + 1; // trailing nul on name. 
    }
    static int get_max_size (void) {
        return
            sizeof(fileinfo)
            - 4; // account for 'next'
    }
};

struct fileinfo *
scanfiles( char * infile )
{
    fileinfo * head = NULL;
    fileinfo ** next = &head;
    fileinfo * fi;

    FILE * in = fopen(infile, "r");
    if (!in)
        return NULL;

    while (1)
    {
        fi = new fileinfo;
        unsigned int id;
        unsigned long long size;
        fi->name[0] = 'a';
        fi->name[1] = 0;
        int cc = fscanf(
            in,
            "%u %*10s %*d %*s %*s %llu %*s %*d %*s %[ -ÿ]\n",
            &id, &size, fi->name);
        if (cc != 3)
            break;
        fi->id.set( id );
        fi->size.set( size );
        fi->next = NULL;
        *next = fi;
        next = &fi->next;
    }
    delete fi;
    fclose(in);
    return head;
}

int
main( int argc, char ** argv )
{
    if (argc != 2)
    {
    bail:
        fprintf(stderr, "usage: t2 getid|getcnt|put\n");
        return 1;
    }

    enum { OP_GETID, OP_GETCNT, OP_PUT } op;

    if (strcmp(argv[1],"getid")==0)
        op = OP_GETID;
    else if (strcmp(argv[1],"getcnt")==0)
        op = OP_GETCNT;
    else if (strcmp(argv[1],"put")==0)
        op = OP_PUT;
    else
        goto bail;

    int fd, options;

    if (op == OP_PUT)
    {
        (void) unlink( TEST_FILE );
        options = O_RDWR | O_CREAT | O_LARGEFILE;
    }
    else
    {
        options = O_RDWR | O_LARGEFILE;
    }

    fd = open( TEST_FILE, options, 0644 );
    if ( fd < 0 )
    {
        fprintf(stderr, "open: %s\n", strerror(errno));
        return 1;
    }

    PageIO * pageio = new PageIOFileDescriptor(fd);
    BlockCache * bc = new BlockCache( pageio, MAX_BYTES );
    if (op == OP_PUT)
        FileBlockInterface::init_file(bc);
    FileBlockInterface * fbi = FileBlockInterface::open(bc);
    if (op == OP_PUT)
        Btree::init_file( fbi, 13 );
    Btree * bt = Btree::open( fbi );

    fileinfo * files = scanfiles( (char*)"/home/flipk/filelist.txt" );
    fileinfo * fi;
    int count = 0;
    for (fi = files; fi; fi = fi->next)
        count++;
    printf("scanned %d files\n", count);

    BTDatum <fileid>    btid(bt);
    BTDatum <filecount> btcount(bt);
    BTDatum <fileinfo>  btinfo(bt);

    if (op == OP_PUT)
    {
        btid.alloc();
        btcount.alloc();
        btinfo.alloc();

        count = 0;
        for (fi = files; fi; fi = fi->next)
        {
            count++;
            fi->count.set( count );
            btid.d->id = fi->id;
            btcount.d->count.set( count );
            *(btinfo.d) = *fi;
            if (bt->put( &btcount, &btid ) == false)
                printf("put failed on %s\n", fi->name);
            if (bt->put( &btid, &btinfo ) == false)
                printf("put failed on %s\n", fi->name);
        }

        myIterator iter(fbi);
        bt->iterate( &iter );
    }
    else if (op == OP_GETID)
    {
        btid.alloc();

        for (fi=files; fi; fi=fi->next)
        {
            btid.d->id = fi->id;
            if (bt->get( &btid, &btinfo ) == false)
            {
                printf("failed to retrieve id %d\n", fi->id.get());
            }
            else
            {
                printf("id %d -> %d %d %lld %s\n",
                       btid.d->id.get(),
                       btinfo.d->id.get(),
                       btinfo.d->count.get(),
                       btinfo.d->size.get(),
                       btinfo.d->name);
            }
        }
    }
    else if (op == OP_GETCNT)
    {
        btcount.alloc();

        count = 0;
        for (fi=files; fi; fi=fi->next)
        {
            count++;
            btcount.d->count.set(count);
            if (bt->get( &btcount, &btid ) == false)
            {
                printf("failed to retrieve count %d\n", count);
            }
            else
            {
                if (bt->get( &btid, &btinfo ) == false)
                {
                    printf("failed to retrieve id %d\n", btid.d->id.get());
                }
                else
                {
                    printf("id %d -> %d %d %lld %s\n",
                           btid.d->id.get(),
                           btinfo.d->id.get(),
                           btinfo.d->count.get(),
                           btinfo.d->size.get(),
                           btinfo.d->name);
                }
            }
        }
    }

    btid.release();
    btcount.release();
    btinfo.release();

    delete bt;
    delete fbi;
    delete bc;
    delete pageio;
    close(fd);

    return 0;

}

#endif
