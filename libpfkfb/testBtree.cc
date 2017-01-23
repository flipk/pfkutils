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

/** \file testBtree.cc
 * \brief test harness for Btree and BtreeInternal objects
 */

#include "Btree.h"
// #include "Btree_internal.h"

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
#define MAX_BYTES (1*1024*1024)

class myIterator : public BtreeIterator {
    FileBlockInterface * fbi;
public:
    myIterator(FileBlockInterface * _fbi) {
        fbi = _fbi;
        wantPrinting = true;
    }
    /*virtual*/ bool handle_item( uint8_t * keydata, uint32_t keylen,
                                  uint32_t data_fbn ) {
        printf( "key len %d: ", keylen);
        int i;
        for (i=0; i < (int)keylen; i++)
            printf("%02x", keydata[i]);
        printf("\ndata fbn: %08x: ", data_fbn);
#if 1 /* print data contents */
#if TEST==1
        FileBlock * fb = fbi->get(data_fbn);
        for (i=0; i < fb->get_size(); i++)
            printf("%02x", fb->get_ptr()[i]);
        fbi->release(fb);
#endif
#endif
        printf("\n");
        return true; // ok to continue iterating
    }
    /*virtual*/ void print( const char * format, ... ) {
        va_list ap;
        va_start(ap,format);
        vprintf(format,ap);
        va_end(ap);
    }
};
    
#if TEST==1

struct crapkey : public BST {
    crapkey(void) : BST(NULL), key(this) { }
    ~crapkey(void) { bst_free(); }
    BST_UINT32_t key;
};

struct crapdata : public FileBlockBST {
    crapdata(FileBlockInterface * _fbi) :
        FileBlockBST(NULL,_fbi), data(this) { }
    ~crapdata(void) { bst_free(); }
    BST_UINT32_t data;
};

struct ramcopy {
    bool infile;
    uint32_t data;
    ramcopy(void) { infile = false; }
};

int
main()
{
    unlink( TEST_FILE );
    Btree * bt = Btree::createFile( TEST_FILE, MAX_BYTES, 0644, 13 );

    crapkey    key;
    crapdata   data(bt->get_fbi());
    uint32_t     data_fbn;

#define ITERATIONS 0x10000
#define ITEMS      0x01000

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
                key.key() = ind;
                if (bt->del( &key, &data_fbn ) == false)
                    fprintf(stderr,"\r %d delete %d failed\n",iter,ind);
                bt->get_fbi()->free(data_fbn);
                ram[ind].infile = false;
                deletes++;
            }
            else
            {
                key.key() = ind;
                if (bt->get( &key, &data_fbn ) == false)
                    fprintf(stderr,"\r %d query %d failed\n", iter, ind);
                else
                {
                    if (!data.get(data_fbn))
                        fprintf(stderr,"\r %d data retrieval %d failed\n",
                                iter, ind);
                    else if (data.data() != ram[ind].data)
                        fprintf(stderr,"\r %d query %d data mismatch\n",
                                iter, ind);
                }
                queries++;
            }
        }
        else
        {
            ram[ind].data = random();
            key.key() = ind;
            data.data() = ram[ind].data;
            if (!data.putnew( &data_fbn ))
                fprintf(stderr,"\r %d data put %d failed\n", iter, ind);
            if (bt->put( &key, data_fbn ) == false)
                fprintf(stderr,"\r %d put %d failed\n", iter, ind);
            else
                ram[ind].infile = true;
            inserts++;
        }
    }
    fprintf(stderr,
            "\ncompleted %d iterations; %d inserts, %d deletes, %d queries\n",
            ITERATIONS, inserts, deletes, queries);

#if 1  /* uncomment to enable dumping the database at the end. */
    myIterator iterator(bt->get_fbi());
    bt->iterate( &iterator );
#endif

    delete bt;

    return 0;
}

#elif TEST==2

struct crapkey : public BST {
    crapkey(BST *parent) :
        BST(parent), key(this) { }
    BST_UINT32_t key;
};

struct ramcopy {
    bool infile;
    uint32_t data;
    ramcopy(void) { infile = false; }
};

#define ITERATIONS 0x1000000
#define ITEMS      0x0100000

ramcopy  ram[ITEMS];

int
main()
{
    unlink( TEST_FILE );
    Btree * bt = Btree::createFile( TEST_FILE, MAX_BYTES, 0644, 13 );

    crapkey    key(NULL);
    uint32_t     data;

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
                key.key() = ind;
                if (bt->del( &key, &data ) == false)
                    fprintf(stderr,"\r %d delete %d failed\n",iter,ind);
                ram[ind].infile = false;
                deletes++;
            }
            else
            {
                key.key() = ind;
                if (bt->get( &key, &data ) == false)
                    fprintf(stderr,"\r %d query %d failed\n", iter, ind);
                else
                {
                    if (data != ram[ind].data)
                        fprintf(stderr,"\r %d query %d data mismatch\n",
                                iter, ind);
                }
                queries++;
            }
        }
        else
        {
            ram[ind].data = random();
            data = ram[ind].data;
            key.key() = ind;
            if (bt->put( &key, data ) == false)
                fprintf(stderr,"\r %d put %d failed\n", iter, ind);
            else
                ram[ind].infile = true;
            inserts++;
        }
    }
    fprintf(stderr,
            "\ncompleted %d iterations; %d inserts, %d deletes, %d queries\n",
            ITERATIONS, inserts, deletes, queries);

#if 0  /* uncomment to enable dumping the database at the end. */
    myIterator iterator(bt->get_fbi());
    bt->iterate( &iterator );
#endif

    delete bt;

    return 0;
}

#endif
