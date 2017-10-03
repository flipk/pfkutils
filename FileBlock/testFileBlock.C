
/** \file testFileBlock.C
 * \brief Test harness for FileBlock
 * \author Phillip F Knaack
 *
 * This file is the test harness for the components of FileBlock.
 * It is used to test anything and everything during development. */

#include "FileBlockLocal.H"

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/** unit of test data stored in the file to test the API. */
struct TestData {
    UINT32_t  id;     /**< unique identifier for a block */
    UINT32_t  idxor;  /**< alternate encoding of id, to consume space */
    UINT32_t  seq;    /**< sequence number to validate updates */
};

/** in-memory copy of data used to validate storage in file. */
struct info {
    UINT32 id;  /**< unique identifier for a block */
    int seq;    /**< sequence number, basically the loop count */
    int extra;  /**< how many extra bytes of space was allocated */
    bool inuse; /**< whether this slot in the array is in use or not */
    info(void) { inuse = false; }
};

#define VERBOSE 0
#define XOR_CONSTANT 0x12345678

int
main(int argc, char ** argv)
{
    int           fd;
    PageIO      * io;
    BlockCache  * bc;
    FileBlockInterface * fbi;

    srandom( getpid() * time(NULL) );

    (void) unlink( "testfile.db" );
    fd = open("testfile.db", O_RDWR | O_CREAT | O_LARGEFILE, 0644);
    if (fd < 0)
    {
        fprintf(stderr, "unable to open file\n");
        exit( 1 );
    }

    io = new PageIOFileDescriptor(fd);
    bc = new BlockCache(io, 256*1024*1024);
    FileBlockLocal::init_file( bc );
    fbi = new FileBlockLocal( bc );

#define ITEMS 1000000
#define LOOPS 100000000

    info * infos = new info[ITEMS];

    int count_delete = 0;
    int count_read = 0;
    int count_create = 0;
    int count_flush = 0;

    time_t last, now;
    time(&last);

    int loop;
    for (loop = 0; loop < LOOPS; loop++)
    {
        int index = random() % ITEMS;
        info * i = &infos[index];

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
                   td.data->id.get(),
                   td.data->idxor.get(),
                   td.data->seq.get());
#endif
                if (td.data->id.get() != i->id)
                    printf("%d: ERROR id mismatch\n", loop);
                else if (td.data->idxor.get() != (i->id ^ XOR_CONSTANT))
                    printf("%d: ERROR idxor mismatch\n", loop);
                else if (td.data->seq.get() != (UINT32)i->seq)
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
                td.data->seq.set( loop );
                td.mark_dirty();
            }
        }
        else
        {
            int extra = (random() & 0x7F);

            // create
            UINT32 id = fbi->alloc(sizeof(TestData) + extra);
            i->id = id;
            i->seq = loop;
            i->inuse = true;
            i->extra = extra;

            FileBlockT <TestData>  td(fbi);
            td.get(id,true);
            td.data->id.set( id );
            td.data->idxor.set( id ^ XOR_CONSTANT );
            td.data->seq.set( loop );
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

    fprintf(stderr, "\n");

    delete[] infos;

    delete fbi;
    delete bc;
    delete io;
    close(fd);

    return 0;
}
