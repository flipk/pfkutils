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

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>
#include <inttypes.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/time.h>
#include <signal.h>

#include "shmempipe.H"

static void dummyCallback(shmempipe *, void *arg) { /* nothing */ }

shmempipe :: shmempipe( shmempipeMasterConfig * pConfig )
{
    m_bMaster = true;
    m_bConnected = m_bCloserRunning = false;
    pConfig->bInitialized = false;
    m_shmemFd = m_myPipeFd = m_otherPipeFd = -1;
    m_shmemPtr = 0;
    m_filename = pConfig->file;
    m_callbacks = pConfig->callbacks;
    if (pConfig->poolInfo.numPools == 0)
    {
        fprintf(stderr, "no buffer pools defined\n");
        return;
    }
    (void) unlink(m_filename.filename);
    m_shmemFd = open(m_filename.filename, O_RDWR | O_CREAT, 0600);
    if (m_shmemFd < 0)
    {
        fprintf(stderr, "unable to create file %s: %s\n",
                m_filename.filename, strerror(errno));
        return;
    }
    (void) unlink(m_filename.m2sname);
    if (mkfifo(m_filename.m2sname, 0600) < 0)
    {
        fprintf(stderr, "unable to create pipe %s: %s\n",
                m_filename.m2sname, strerror(errno));
        return;
    }
    (void) unlink(m_filename.s2mname);
    if (mkfifo(m_filename.s2mname, 0600) < 0)
    {
        fprintf(stderr, "unable to create pipe %s: %s\n",
                m_filename.s2mname, strerror(errno));
        return;
    }
    m_fileSize = sizeof(shmempipeHeader);
    int poolInd;
    uint32_t poolOffset = m_fileSize;
    for (poolInd = 0;
         poolInd < pConfig->poolInfo.numPools;
         poolInd++)
    {
        m_fileSize +=
            (pConfig->poolInfo.bufSizes[poolInd] +
             sizeof(shmempipeMessage)) *
            pConfig->poolInfo.numBufs[poolInd];
    }

    ftruncate(m_shmemFd, (off_t) m_fileSize);
    m_shmemPtr = (uintptr_t)
        mmap(NULL, m_fileSize, PROT_READ | PROT_WRITE,
             MAP_SHARED, m_shmemFd, 0);
    if (m_shmemPtr == (uintptr_t)MAP_FAILED)
    {
        m_shmemPtr = 0;
        fprintf(stderr, "unable to mmap file: %s: %s\n",
                m_filename.filename, strerror(errno));
        return;
    }

    m_shmemLimit = m_shmemPtr + m_fileSize;
    m_pHeader = (shmempipeHeader *) m_shmemPtr;

    m_pHeader->poolInfo = pConfig->poolInfo;

    m_pHeader->master2slave.init();
    m_pHeader->slave2master.init();

    uintptr_t  buf = m_shmemPtr + poolOffset;

    for (poolInd = 0;
         poolInd < pConfig->poolInfo.numPools;
         poolInd++)
    {
        m_pHeader->pools[poolInd].init();
        for (int bufnum=0;
             bufnum < pConfig->poolInfo.numBufs[poolInd];
             bufnum++)
        {
            m_pHeader->pools[poolInd].enqueue(
                m_shmemPtr,
                (shmempipeMessage *)buf,
                NULL,
                false /*for speed*/);
            // add buf to pool
            buf +=
                pConfig->poolInfo.bufSizes[poolInd] +
                sizeof(shmempipeMessage);
        }
    }

    m_myBufferList = &m_pHeader->slave2master;
    m_otherBufferList = &m_pHeader->master2slave;

    pthread_mutexattr_t mattr;
    pthread_mutexattr_init( &mattr );
    pthread_mutex_init( &m_statsMutex, &mattr );
    pthread_mutexattr_destroy( &mattr );

    startCloserThread();

    pConfig->bInitialized = true;
}

shmempipe :: shmempipe( shmempipeSlaveConfig * pConfig )
{
    m_bMaster = false;
    m_bConnected = m_bCloserRunning = false;
    m_shmemFd = m_myPipeFd = m_otherPipeFd = -1;
    m_shmemPtr = 0;
    m_filename = pConfig->file;
    m_callbacks = pConfig->callbacks;
    m_shmemFd = open(m_filename.filename, O_RDWR);
    if (m_shmemFd < 0)
    {
        fprintf(stderr, "unable to open file %s: %s\n",
                m_filename.filename, strerror(errno));
        return;
    }
    struct stat sb;
    fstat(m_shmemFd,&sb);
    m_fileSize = sb.st_size;
    m_shmemPtr = (uintptr_t)
        mmap(NULL, m_fileSize, PROT_READ | PROT_WRITE,
             MAP_SHARED, m_shmemFd, 0);
    if (m_shmemPtr == (uintptr_t)MAP_FAILED)
    {
        m_shmemPtr = 0;
        fprintf(stderr, "unable to mmap file: %s: %s\n",
                m_filename.filename, strerror(errno));
        return;
    }
    m_shmemLimit = m_shmemPtr + m_fileSize;
    m_pHeader = (shmempipeHeader *) m_shmemPtr;

    m_myBufferList = &m_pHeader->master2slave;
    m_otherBufferList = &m_pHeader->slave2master;

    pthread_mutexattr_t mattr;
    pthread_mutexattr_init( &mattr );
    pthread_mutex_init( &m_statsMutex, &mattr );
    pthread_mutexattr_destroy( &mattr );

    startCloserThread();

    pConfig->bInitialized = true;
}

shmempipe :: ~shmempipe(void)
{

    // todo: call all shmempipeBufferList :: cleanup methods

    if (m_bReaderRunning)
        stopReaderThread();
    if (m_bCloserRunning)
        stopCloserThread();
    if (m_shmemFd > 0)
        close(m_shmemFd);
    if (m_myPipeFd > 0)
        close(m_myPipeFd);
    if (m_otherPipeFd > 0)
        close(m_otherPipeFd);
    if (m_shmemPtr != 0)
        munmap((void*)m_shmemPtr, m_fileSize);
    if (m_bMaster)
    {
        (void) unlink(m_filename.filename);
        (void) unlink(m_filename.m2sname);
        (void) unlink(m_filename.s2mname);
    }
}

void
shmempipeBufferList :: init(void)
{
    head = tail = 0;
    count = 0;
    bNeedsPoke = false;
    bIsWaiting = false;
    pthread_mutexattr_t mattr;
    pthread_condattr_t  cattr;
    pthread_mutexattr_init( &mattr );
    pthread_condattr_init( &cattr );
    pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
    pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init( &mutex, &mattr );
    pthread_cond_init( &empty_cond, &cattr );
    pthread_mutexattr_destroy( &mattr );
    pthread_condattr_destroy( &cattr );
}

void
shmempipe  :: startCloserThread(void)
{
    pthread_attr_t  attr;
    pthread_t id;

    pipe(m_closerPipe);

    pthread_attr_init( &attr );
    pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );
    pthread_create(&id, &attr, &_closerThreadEntry, (void*) this);
    pthread_attr_destroy( &attr );

    while (m_bCloserRunning == false)
        usleep(1);
}

// todo : instead of bCloserRunning,  have state enum

void
shmempipe  :: stopCloserThread(void)
{

    // todo : use pthread cancel if in the open-pipes state.

    if (m_bCloserRunning)
    {
        close(m_closerPipe[1]);
        while (m_bCloserRunning)
            usleep(1);
        close(m_closerPipe[0]);
    }
}

//static
void *
shmempipe :: _closerThreadEntry(void * arg)
{
    shmempipe * pPipe = (shmempipe *) arg;
    pPipe->closerThread();
    return NULL;
}

void
shmempipe :: closerThread(void)
{
    m_bCloserRunning = true;

    // todo : install pthread cleanup handler to reset closer state.

    if (m_bMaster)
    {
        // open my pipe, wait for connect indication.
        // when i get one, open other pipe, write connect indication.
        m_myPipeFd = open(m_filename.s2mname, O_RDONLY);
        if (m_myPipeFd < 0)
        {
            printf("crap 1\n");
            return;
        }
        char c;
        read(m_myPipeFd, &c, 1);
        m_otherPipeFd = open(m_filename.m2sname, O_WRONLY);
        if (m_otherPipeFd < 0)
        {
            printf("crap 2\n");
            return;
        }
        write(m_otherPipeFd, &c, 1);
    }
    else
    {
        // open other pipe, write connect indication,
        // then try to open my pipe, wait for connect indication.
        char c = 1;
        m_otherPipeFd = open(m_filename.s2mname, O_WRONLY);
        if (m_otherPipeFd < 0)
        {
            printf("crap 2\n");
            return;
        }
        write(m_otherPipeFd, &c, 1);
        m_myPipeFd = open(m_filename.m2sname, O_RDONLY);
        if (m_myPipeFd < 0)
        {
            printf("crap 1\n");
            return;
        }
        read(m_myPipeFd, &c, 1);
    }

    m_callbacks.connectCallback(this, m_callbacks.arg);
    m_callbacks.connectCallback = &dummyCallback;
    m_bConnected = true;
    startReaderThread();

    while (1)
    {
        fd_set rfds;
        int maxfd;

        FD_ZERO(&rfds);
        FD_SET(m_myPipeFd, &rfds);
        FD_SET(m_closerPipe[0], &rfds);
        if (m_myPipeFd > m_closerPipe[0])
            maxfd = m_myPipeFd + 1;
        else
            maxfd = m_closerPipe[0] + 1;
        select(maxfd, &rfds, NULL, NULL, NULL);
        if (FD_ISSET(m_closerPipe[0], &rfds))
            break;
        if (FD_ISSET(m_myPipeFd, &rfds))
        {
            char buf[10];
            int cc = read(m_myPipeFd, buf, sizeof(buf));
            if (cc <= 0)
                break;
        }
    }

    m_bConnected = false;
    stopReaderThread();
    m_callbacks.disconnectCallback(this, m_callbacks.arg);
    m_callbacks.disconnectCallback = &dummyCallback;

    if (m_myPipeFd != -1)
    {
        close(m_myPipeFd);
        m_myPipeFd = -1;
    }
    if (m_otherPipeFd != -1)
    {
        close(m_otherPipeFd);
        m_otherPipeFd = -1;
    }

    m_bCloserRunning = false;
}

void
shmempipe :: startReaderThread(void)
{
    pthread_attr_t  attr;
    pthread_t id;

    m_bReaderStop = false;
    pthread_attr_init( &attr );
    pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );
    pthread_create(&id, &attr, &_readerThreadEntry, (void*) this);
    pthread_attr_destroy( &attr );

    while (m_bReaderRunning == false)
        usleep(1);
}

void
shmempipe :: stopReaderThread(void)
{
    if (m_bReaderRunning)
    {
        m_bReaderStop = true;
        m_myBufferList->poke();
        while (m_bReaderRunning)
            usleep(1);
    }
}

//static
void *
shmempipe :: _readerThreadEntry(void *arg)
{
    shmempipe * pPipe = (shmempipe *) arg;
    pPipe->readerThread();
    return NULL;
}

void
shmempipe :: readerThread(void)
{
    m_bReaderRunning = true;

    while (m_bReaderStop == false)
    {
        bool signalled = false;
        shmempipeMessage * pMsg = m_myBufferList->dequeue(m_shmemPtr,
                                                          &signalled, true);
        if (pMsg)
        {
            lockStats();
            m_stats.rcvd_packets ++;
            m_stats.rcvd_bytes += pMsg->messageSize;
            if (signalled)
                m_stats.rcvd_signals ++;
            unlockStats();
            m_callbacks.messageCallback(this, m_callbacks.arg, pMsg);
        }
    }

    m_bReaderRunning = false;
}

void
shmempipe :: getStats(shmempipeStats * pStats, bool zero)
{
    lockStats();
    *pStats = m_stats;
    if (zero)
        m_stats.init();
    uint64_t free_buffers = 0;
    for (int poolInd = 0;
         poolInd < m_pHeader->poolInfo.numPools;
         poolInd++)
    {
        free_buffers += m_pHeader->pools[poolInd].get_count();
    }
    pStats->free_buffers = free_buffers;
    unlockStats();
}
