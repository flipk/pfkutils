
/** \file main.C
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

int
main(int argc, char ** argv)
{
    int           fd;
    PageIO      * io;
    BlockCache  * bc;
    FileBlockInterface * fbi;

    if (strcmp(argv[1], "init") == 0)
    {
        (void) unlink( "testfile.db" );
        fd = open("testfile.db", O_RDWR | O_CREAT | O_LARGEFILE, 0644);
        if (fd < 0)
        {
            fprintf(stderr, "unable to open file\n");
            exit( 1 );
        }

        io = new PageIOFileDescriptor(fd);
        bc = new BlockCache(io, 4*1024*1024);

        FileBlockLocal::init_file( bc );
        delete bc;
        delete io;
        close(fd);
        return 0;
    }

    fd = open("testfile.db", O_RDWR | O_LARGEFILE, 0644);
    if (fd < 0)
    {
        fprintf(stderr, "unable to open file\n");
        exit( 1 );
    }

    io = new PageIOFileDescriptor(fd);
    bc = new BlockCache(io, 256*1024*1024);

    fbi = new FileBlockLocal( bc );
    FileBlock * fb;
    UINT32 id;

    if (strcmp(argv[1], "put") == 0)
    {
        int i, last_i;
        time_t last, now;
        FILE * f = fopen( "ids", "w");
        time( &last );
        last_i = 0;
        for (i = 0; i < 1000000; i++)
        {
            UINT32 size = (random() % 5120) + 1;
            UINT32 id;

            id = fbi->alloc(size);
            fprintf(f, "%d %d %d\n", i, id, size);
            fb = fbi->get( id );
            memset( fb->get_ptr(), i & 0xFF, size );
            fbi->release( fb, true );
            if (time( &now ) != last)
            {
                printf(" %d (%d)\r", i, i-last_i);
                fflush(stdout);
                last = now;
                last_i = i;
            }
        }
        fclose(f);
    }

    if (strcmp(argv[1], "get") == 0)
    {
        UINT32 i, last_i;
        UINT32 size;
        UCHAR * buf;
        time_t last, now;
        FILE * f = fopen( "ids", "r" );

        time( &last );
        last_i=0;
        while (fscanf(f, "%d %d %d\n", &i, &id, &size) == 3)
        {
            fb = fbi->get( id );
            if (fb)
            {
                UINT32 j;
                buf = fb->get_ptr();
                for (j=0; j < size; j++)
                    if (buf[j] != (i & 0xFF))
                    {
                        fprintf(stderr, "ERROR MISMATCH\n");
                        printf("ERROR MISMATCH\n");
                    }
                fbi->release(fb);
            }
            else
            {
                fprintf( stderr, "block id not found!\n" );
                printf( "block id not found!\n" );
            }
            if (time( &now ) != last)
            {
                printf(" %d (%d)\r", i, i-last_i);
                fflush(stdout);
                last = now;
                last_i = i;
            }
        }

        fclose(f);
    }

    delete fbi;
    delete bc;
    delete io;
    close(fd);

    return 0;
}
