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

#include "pk_threads.h"
#include "pk_filedescs_internal.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

PK_File_Descriptor_Manager * PK_File_Descriptors_global;

/** \cond INTERNAL */
PK_File_Descriptor_List :: ~PK_File_Descriptor_List( void )
{
    //? destroy pending data on the list
}

PK_File_Descriptor_Manager :: PK_File_Descriptor_Manager( void )
{
    if ( PK_File_Descriptors_global )
    {
        fprintf( stderr, "PK_File_Descriptor_Manager already exists?!\n" );
        kill(0,6);
    }

    descs = new PK_File_Descriptor_List;
    pthread_mutex_init( &mutex, NULL );
    PK_File_Descriptors_global = this;
    if (pipe(wakeup_pipe) < 0)
        fprintf(stderr, "PK_File_Descriptor_Manager: pipe failed\n");
    thread = new PK_File_Descriptor_Thread(this);
}

PK_File_Descriptor_Manager :: ~PK_File_Descriptor_Manager( void )
{
    delete descs;
    pthread_mutex_destroy( &mutex );
    PK_File_Descriptors_global = NULL;
    close(wakeup_pipe[0]);
    close(wakeup_pipe[1]);
}

void
PK_File_Descriptor_Manager :: stop( void )
{
    thread->stop();
}

int // pkfdid
PK_File_Descriptor_Manager :: register_fd( int fd, PK_FD_RW rw,
                                           int qid, void *obj )
{
    int pkfdid;

    PK_File_Descriptor * pkfd = new PK_File_Descriptor;
    pkfd->fd = fd;
    pkfd->rw = rw;
    pkfd->qid = qid;
    pkfd->obj = obj;

    // find a unique id
    _lock();
    do {
        do {
            pkfdid = random();
        } while (pkfdid == -1  ||  pkfdid == 0); // never use -1 or 0
    } while (descs->find(pkfdid) != NULL);
    pkfd->pkfdid = pkfdid;
    descs->add(pkfd);
    _unlock();

    // awaken the thread
    char c = 1;
    if (write(wakeup_pipe[1], &c, 1) < 0)
        fprintf(stderr, "PK_File_Descriptor_Manager: write failed\n");

    return pkfdid;
}

void *
PK_File_Descriptor_Manager :: unregister_fd( int pkfdid )
{
    PK_File_Descriptor * pkfd;
    void * obj = NULL;

    _lock();
    pkfd = descs->find(pkfdid);
    if (pkfd)
    {
        descs->remove(pkfd);
        _unlock();
        obj = pkfd->obj;
        delete pkfd;
    }
    else
        _unlock();

    if (pkfd) // don't dereference, just check nonnull
    {
        // awaken the thread
        char c = 1;
        if (write(wakeup_pipe[1], &c, 1) < 0)
            fprintf(stderr, "PK_File_Descriptor_Manager: write failed\n");
    }
    return obj;
}
/** \endcond */
