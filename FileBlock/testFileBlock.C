
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

/** \file testFileBlock.C
 * \brief Test harness for FileBlock
 * \author Phillip F Knaack
 *
 * This file is the test harness for the components of FileBlock.
 * It is used to test anything and everything during development. */

#include "FileBlock_iface.H"

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>

/** unit of test data stored in the file to test the API. */
struct TestData {
    FB_AUID_t id;     /**< unique identifier for a block */
    UINT32_t  idxor;  /**< alternate encoding of id, to consume space */
    UINT32_t  seq;    /**< sequence number to validate updates */
};

/** in-memory copy of data used to validate storage in file. */
struct info {
    FB_AUID_T id;  /**< unique identifier for a block */
    int seq;    /**< sequence number, basically the loop count */
    int extra;  /**< how many extra bytes of space was allocated */
    bool inuse; /**< whether this slot in the array is in use or not */
    info(void) { inuse = false; }
};

#define TESTFILE "testfile.db"

#define VERBOSE 0
#define XOR_CONSTANT 0x12345678

#define TEST 1

#if TEST==1
int testFileBlockSignalOccurred = 0;

static void
sighandler( int sig, siginfo_t * info, void * extra )
{
    testFileBlockSignalOccurred = 1;
}

#include "FileBlockLocal.H"

int
main(int argc, char ** argv)
{
    FileBlockInterface * _fbi;
    FileBlockLocal * fbi;

    srandom( getpid() * time(NULL) );

    (void) unlink( TESTFILE );

    _fbi = FileBlockInterface::createFile( TESTFILE, 65536 * 4096, 0644 );

    fbi = (FileBlockLocal*) _fbi;

#if 1
#define ITEMS 1000000
#define LOOPS 10000000
#else
#define ITEMS 100
#define LOOPS 1000
#endif

    info * infos = new info[ITEMS];

    int count_delete = 0;
    int count_read = 0;
    int count_create = 0;
    int count_flush = 0;

    time_t last, now;
    time(&last);

    struct sigaction act;

    act.sa_sigaction = sighandler;
    sigfillset( &act.sa_mask );
    act.sa_flags = SA_RESTART | SA_SIGINFO;

    sigaction( SIGINT, &act, NULL );

    int loop;
    for (loop = 0; loop < LOOPS; loop++)
    {
        int idx = random() % ITEMS;
        info * i = &infos[idx];

        if (testFileBlockSignalOccurred)
            break;

        if (i->inuse)
        {
            FileBlockT <TestData>  td(fbi);
            if (td.get(i->id) == false)
                printf("%d: ERROR failed to get id 0x%x\n", loop, i->id);
            else {
#if VERBOSE
                printf("%d: getting id 0x%x extra %d -> "
                       "id 0x%x idxor 0x%x seq 0x%x\n",
                       loop, i->id, i->extra,
                       td.d->id.get(),
                       td.d->idxor.get(),
                       td.d->seq.get());
#endif
                if (td.d->id.get() != i->id)
                    printf("%d: ERROR id mismatch\n", loop);
                else if (td.d->idxor.get() != (i->id ^ XOR_CONSTANT))
                    printf("%d: ERROR idxor mismatch\n", loop);
                else if (td.d->seq.get() != (UINT32)i->seq)
                    printf("%d: ERROR seq mismatch\n", loop);
            }
            count_read ++;
            if ((random() & 0x7F) > 0x40)
            {
                // delete
                td.release();
                fbi->free(i->id);
                i->inuse = false;
                count_delete ++;
#if VERBOSE
                printf("%d: deleted id 0x%x seq 0x%x\n", loop, i->id, i->seq);
#endif
            }
            else
            {
#if VERBOSE
                printf("%d: updated id 0x%x from seq 0x%x to seq 0x%x\n",
                       loop, i->id, i->seq, loop);
#endif
                i->seq = loop;
                td.d->seq.set( loop );
                td.mark_dirty();
            }
        }
        else
        {
            int extra = (random() & 0x7F);

            // create
            FB_AUID_T id = fbi->alloc(sizeof(TestData) + extra);
            i->id = id;
            i->seq = loop;
            i->inuse = true;
            i->extra = extra;

            FileBlockT <TestData>  td(fbi);
            td.get(id,true);
            td.d->id.set( id );
            td.d->idxor.set( id ^ XOR_CONSTANT );
            td.d->seq.set( loop );
            td.mark_dirty();

#if VERBOSE
            printf("%d: add id 0x%x seq 0x%x extra %d\n",
                   loop, id, loop, extra);
#endif

            count_create ++;
        }

        if ((loop % ITEMS) == 0)
        {
            fbi->flush();
            count_flush++;
        }
        if (time(&now) != last)
        {
            fprintf( stderr,
                     " %d : %d creates, %d reads, %d deletes, %d flushes  \r",
                     LOOPS - loop, count_create, count_read,
                     count_delete, count_flush );
            last = now;
        }
    }

    fprintf( stderr,
             " %d : %d creates, %d reads, %d deletes, %d flushes  \n",
             LOOPS - loop, count_create, count_read,
             count_delete, count_flush );

    delete[] infos;

    fbi->validate(false);
    delete fbi;

    (void) unlink( TESTFILE );

    return 0;
}

#elif TEST==2

struct CRAP {
    UINT32_t   junk;
};

int
main( int argc, char ** argv )
{
    int           fd;
    int           options;
    PageIO      * io;
    BlockCache  * bc;
    FileBlockInterface * fbi;

    options = O_RDWR;
#ifdef O_LARGEFILE
    options |= O_LARGEFILE;
#endif
    if (strcmp(argv[1], "init") == 0)
    {
        options |= O_CREAT;
        (void) unlink( TESTFILE );
    }
    fd = open(TESTFILE, options, 0644);

    if (fd < 0)
    {
        fprintf(stderr, "unable to open file\n");
        exit( 1 );
    }

    io = new PageIOFileDescriptor(fd);
    bc = new BlockCache(io, 256*1024*1024);

    if (strcmp(argv[1], "init") == 0)
    {
        FileBlockLocal::init_file( bc );
        goto out;
    }

    fbi = new FileBlockLocal( bc );

    if (strcmp(argv[1], "1") == 0)
    {
        UINT32 id;
        FileBlockT <CRAP>  crap(fbi);

        id = fbi->alloc(sizeof(CRAP));
        crap.get(id,true);
        crap.d->junk.set( 0x12345678 );
        fbi->set_data_info_block(id,(char*)"CRAP");

        id = fbi->alloc(sizeof(CRAP));
        crap.get(id,true);
        crap.d->junk.set( 0x12345679 );
        fbi->set_data_info_block(id,(char*)"CRAP2");

        id = fbi->alloc(sizeof(CRAP));
        crap.get(id,true);
        crap.d->junk.set( 0x82345679 );
        fbi->set_data_info_block(id,(char*)"CRAP3");
    }

    if (strcmp(argv[1], "2") == 0)
    {
        UINT32 id;

        id = fbi->get_data_info_block((char*)"CRAP");
        if (id > 0)
        {
            FileBlockT <CRAP>  crap(fbi);
            if (crap.get(id) == true)
                printf("got junk %#x\n", crap.d->junk.get());
        }

        id = fbi->get_data_info_block((char*)"CRAP2");
        if (id > 0)
        {
            FileBlockT <CRAP>  crap(fbi);
            if (crap.get(id) == true)
                printf("got junk %#x\n", crap.d->junk.get());
        }

        id = fbi->get_data_info_block((char*)"CRAP3");
        if (id > 0)
        {
            FileBlockT <CRAP>  crap(fbi);
            if (crap.get(id) == true)
                printf("got junk %#x\n", crap.d->junk.get());
        }
    }

    if (strcmp(argv[1], "3") == 0)
    {
        fbi->del_data_info_block((char*)"CRAP");
        fbi->del_data_info_block((char*)"CRAP2");
        fbi->del_data_info_block((char*)"CRAP3");
    }

    delete fbi;
out:
    delete bc;
    delete io;
    close(fd);

    return 0;
}

#elif TEST==3

#include "bst.H"

class CrapUnion : public BST_UNION {
public:
    enum { ONE, TWO, MAX };
    CrapUnion(void) : BST_UNION(MAX) { }
    ~CrapUnion(void) { bst_free(); }
    BST_UINT32_t   one;
    BST_STRING     two;
    /*virtual*/ bool bst_op( BST_STREAM *str ) {
        BST * fields[] = { &one, &two };
        return bst_do_union(str, fields);
    }
};

class CrapType : public FileBlockBST {
public:
    CrapType(FileBlockInterface * _fbi) : FileBlockBST(_fbi) { }
    ~CrapType(void) { bst_free(); }
    BST_UINT32_t one;
    BST_UINT32_t two;
    CrapUnion un;
    /*virtual*/ bool bst_op( BST_STREAM *str ) {
        BST * fields[] = { &one, &two, &un, NULL };
        return bst_do_fields( str, fields );
    }
};

struct JunkType : public BST {
    ~JunkType(void) { bst_free(); }
    BST_UINT32_t one;
    BST_STRING   two;
    /*virtual*/ bool bst_op( BST_STREAM *str ) {
        BST * fields[] = { &one, &two, NULL };
        return bst_do_fields( str, fields );
    }
};

int
main()
{
#if 0
    int fd;
    (void) unlink( TESTFILE );
    fd = open(TESTFILE, O_RDWR | O_CREAT, 0644);
    if (fd < 0)
    {
        fprintf(stderr, "unable to open file\n");
        exit( 1 );
    }
    PageIO      * io = new PageIOFileDescriptor(fd);
    BlockCache  * bc = new BlockCache(io, 256*1024*1024);
    FileBlockInterface::init_file( bc );
    FileBlockInterface * fbi = FileBlockInterface::open( bc );
#else
    (void) unlink( TESTFILE );
    FileBlockInterface * fbi = 
        FileBlockInterface::createFile( TESTFILE, 256*1024*1024, 0644 );
#endif

    {
        JunkType  j, k;
        j.one.v = 24;
        j.two.strdup((char*)"this is a stupid test");
        UCHAR * jptr;
        int jlen = 0;
        jptr = j.bst_encode( &jlen );
        if (jptr)
        {
            for (int i = 0; i < jlen; i++)
                printf("%02x", jptr[i]);
            printf("\n");

            if (!k.bst_decode(jptr,jlen))
                printf("decode failure\n");
            else
            {
                printf("%d %s\n", k.one.v, k.two.string);
            }

            delete[] jptr;
        }
        else
            printf("jptr is null\n");
    }

    UINT32 newid = 0;
    CrapType c(fbi);
    CrapType d(fbi);
    CrapType e(fbi);

    c.one.v = 4;
    c.two.v = 5;
    c.un.which.v = CrapUnion::ONE;
    c.un.one.v = 6;

    if (c.putnew( &newid ) == false)
        printf("putnew failed\n");
    else
        printf("allocated new block %d\n", newid);

    if (d.get(newid) == false)
        printf("get failed\n");
    else
    {
        printf("get d succeeded: %d %d ", d.one.v, d.two.v);
        if (d.un.which.v != CrapUnion::ONE)
            printf("incorrect d union\n");
        else
            printf("%d\n", d.un.one.v);
        d.un.which.v = CrapUnion::TWO;
        d.un.two.strdup((char*)"this is a test");
        d.one.v++;
        d.two.v--;
        if (d.put() == false)
            printf("put failed\n");
    }

    if (e.get(newid) == false)
        printf("get e failed\n");
    else
    {
        printf("get e succeeded: %d %d ", e.one.v, e.two.v);
        if (e.un.which.v != CrapUnion::TWO)
            printf("incorrect e union\n");
        else
            printf("%s\n", e.un.two.string);
    }

    delete fbi;
    return 0;
}

#endif
