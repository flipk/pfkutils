
/** \file pk_filedescs.cc
 * \brief implementation of file descriptor manager
 * \author Phillip F Knaack <pfk@pfk.org>

    This file is part of the "pfkutils" tools written by Phil Knaack
    (pfk@pfk.org).
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
    pipe(wakeup_pipe);
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
    write(wakeup_pipe[1], &c, 1);

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
        write(wakeup_pipe[1], &c, 1);
    }
    return obj;
}
/** \endcond */
