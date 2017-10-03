
/*
    This file is part of the "pfkutils" tools written by Phil Knaack
    (pknaack1@netscape.net).
    Copyright (C) 2008  Phillip F Knaack

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

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

#define TEST 1

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
    /*virtual*/ void print( const char * format, ... )
        __attribute__ ((format( printf, 2, 3 ))) {
        va_list ap;
        va_start(ap,format);
        vprintf(format,ap);
        va_end(ap);
    }
};
    
#if TEST==1

struct crapkey : public BST {
    ~crapkey(void) { bst_free(); }
    BST_UINT32_t key;
    /*virtual*/ bool bst_op( BST_STREAM * str ) {
        BST * fields[] = { &key, NULL };
        return bst_do_fields( str, fields );
    }
};

struct crapdata : public FileBlockBST {
    crapdata(FileBlockInterface * _fbi) : FileBlockBST(_fbi) { }
    ~crapdata(void) { bst_free(); }
    BST_UINT32_t data;
    /*virtual*/ bool bst_op( BST_STREAM * str ) {
        BST * fields[] = { &data, NULL };
        return bst_do_fields( str, fields );
    }
};

struct ramcopy {
    bool infile;
    UINT32 data;
    ramcopy(void) { infile = false; }
};

int
main()
{
    unlink( TEST_FILE );
    Btree * bt = Btree::createFile( TEST_FILE, MAX_BYTES, 0644, 13 );

    crapkey    key;
    crapdata   data(bt->get_fbi());
    UINT32     data_fbn;

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
                key.key.v = ind;
                if (bt->del( &key, &data_fbn ) == false)
                    fprintf(stderr,"\r %d delete %d failed\n",iter,ind);
                bt->get_fbi()->free(data_fbn);
                ram[ind].infile = false;
                deletes++;
            }
            else
            {
                key.key.v = ind;
                if (bt->get( &key, &data_fbn ) == false)
                    fprintf(stderr,"\r %d query %d failed\n", iter, ind);
                else
                {
                    if (!data.get(data_fbn))
                        fprintf(stderr,"\r %d data retrieval %d failed\n",
                                iter, ind);
                    else if (data.data.v != ram[ind].data)
                        fprintf(stderr,"\r %d query %d data mismatch\n",
                                iter, ind);
                }
                queries++;
            }
        }
        else
        {
            ram[ind].data = random();
            key.key.v = ind;
            data.data.v = ram[ind].data;
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
    ~crapkey(void) { bst_free(); }
    BST_UINT32_t key;
    /*virtual*/ bool bst_op( BST_STREAM * str ) {
        BST * fields[] = { &key, NULL };
        return bst_do_fields( str, fields );
    }
};

struct ramcopy {
    bool infile;
    UINT32 data;
    ramcopy(void) { infile = false; }
};

int
main()
{
    unlink( TEST_FILE );
    Btree * bt = Btree::createFile( TEST_FILE, MAX_BYTES, 0644, 13 );

    crapkey    key;
    UINT32     data;

#define ITERATIONS 0x100000
#define ITEMS      0x010000

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
                key.key.v = ind;
                if (bt->del( &key, &data ) == false)
                    fprintf(stderr,"\r %d delete %d failed\n",iter,ind);
                ram[ind].infile = false;
                deletes++;
            }
            else
            {
                key.key.v = ind;
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
            key.key.v = ind;
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

#if 1  /* uncomment to enable dumping the database at the end. */
    myIterator iterator(bt->get_fbi());
    bt->iterate( &iterator );
#endif

    delete bt;

    return 0;
}

#endif
