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

#include <inttypes.h>
#include <limits.h>
#include <pthread.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>

#include "pfkutils_config.h"

struct shmempipePoolInfo {
    static const int maxPools = 64;
    uint8_t numPools;
    uint32_t bufSizes[maxPools]; // specify in increasing order.
    uint32_t numBufs[maxPools];
    inline shmempipePoolInfo(void);
    inline bool addPool(uint32_t numBuf, uint32_t bufSize);
};

#define SHMEMPIPE_FILEPATH_FMT "/tmp/p2p.%s.shmem"
#define SHMEMPIPE_M2SPATH_FMT "/tmp/p2p.%s.m2s"
#define SHMEMPIPE_S2MPATH_FMT "/tmp/p2p.%s.s2m"

struct shmempipeFilename {
    static const int FILENAMEMAX = 512;
    char filename[FILENAMEMAX];
    char m2sname[FILENAMEMAX];
    char s2mname[FILENAMEMAX];
    inline shmempipeFilename(void);
    inline void setPipeName(const char * _pipename);
};

class shmempipe;
struct shmempipeMessage;

typedef void (*shmempipeConnectionHandlerFunc)(shmempipe *, void *arg);
typedef void (*shmempipeMessageHandlerFunc)(shmempipe *, void *arg,
                                         shmempipeMessage * pMsg);

struct shmempipeCallbacks {
    shmempipeConnectionHandlerFunc connectCallback;
    shmempipeConnectionHandlerFunc disconnectCallback;
    shmempipeMessageHandlerFunc messageCallback;
    void * arg;
};

struct shmempipeMasterConfig :
    public shmempipeFilename, shmempipeCallbacks, shmempipePoolInfo
{
    bool bInitialized;
};

struct shmempipeSlaveConfig :
    public shmempipeFilename, shmempipeCallbacks
{
    bool bInitialized;
};

struct shmempipeMessage {
    friend class shmempipeBufferList;
private:
    uint32_t next; // offset
    uint8_t bMaster;
    uint8_t poolInd;
    uint32_t bufInd;
    uint32_t bufferSize;
public:
    uint32_t messageSize;
private:
    enum { FREE=1, APP, QUEUE } owner;
    friend class shmempipe;
};

struct shmempipeStats {
    uint64_t sent_bytes;
    uint64_t sent_packets;
    uint64_t sent_signals;
    uint64_t rcvd_bytes;
    uint64_t rcvd_packets;
    uint64_t rcvd_signals;
    uint64_t alloc_fails;
    uint64_t free_buffers;
    inline shmempipeStats(void);
    inline void init(void);
};

class shmempipeBufferList {
private:
    volatile uint32_t head; // dequeue from head
    volatile uint32_t tail; // enqueue to tail
    volatile uint32_t count;
    // shmempipeMessage::next points towards tail
    pthread_mutex_t mutex;
    pthread_cond_t  empty_cond;
    volatile bool needspoke;
    volatile bool bIsWaiting;
    inline bool   lock(void);
    inline void unlock(void);
public:
    void init(void);
    void cleanup(void);
    inline bool empty(void);
    inline void poke(void);
    inline uint32_t get_count(void);
    // the only time needlock=false is at init time when
    // building the freelists initially.
    inline bool enqueue( uintptr_t base, shmempipeMessage * msg,
                         bool *pSignalSent, bool debug,
                         bool needlock=true );
    inline shmempipeMessage * dequeue(uintptr_t base,
                                      bool *pSignalled,
                                      bool bWait, bool debug);
};

struct shmempipeHeader {
    shmempipePoolInfo    poolInfo;
    shmempipeBufferList  pools[shmempipePoolInfo::maxPools];
    shmempipeBufferList  master2slave;
    shmempipeBufferList  slave2master;
    bool attachedFlag;
    // pool buffers start here
};

class shmempipe {
    bool m_bMaster;
    bool m_bConnected;
    bool m_bufferListsInitialized;
    shmempipeFilename m_filename;
    shmempipeCallbacks m_callbacks;
    shmempipeStats m_stats;
    pthread_mutex_t m_statsMutex;
    inline void   lockStats(void);
    inline void unlockStats(void);
    int m_shmemFd;
    int m_myPipeFd;
    int m_otherPipeFd;
    int m_closerPipe[2];
    uint32_t m_fileSize;
    uintptr_t m_shmemPtr;
    uintptr_t m_shmemLimit;
    shmempipeHeader * m_pHeader;
    shmempipeBufferList * m_myBufferList;
    shmempipeBufferList * m_otherBufferList;

    enum {
        CLOSER_NOT_EXIST,
        CLOSER_STARTING,
        CLOSER_RUNNING,
        CLOSER_EXITING,
        CLOSER_DEAD
    } m_closerState;
    pthread_t m_closerId;
    void startCloserThread(void);
    void stopCloserThread(void);
    static void * _closerThreadEntry(void *arg);
    static void closerThreadCleanup(void *arg);
    void closerThread(void);

    bool m_bReaderRunning;
    bool m_bReaderStop;
    void startReaderThread(void);
    void stopReaderThread(void);
    static void * _readerThreadEntry(void *arg);
    void readerThread(void);

public:
    shmempipe( shmempipeMasterConfig * pConfig );
    shmempipe( shmempipeSlaveConfig * pConfig );
    ~shmempipe( void );
    inline shmempipeMessage * allocSize(uint32_t size, bool bWait=false);
    inline shmempipeMessage * allocPool(uint32_t poolInd, bool bWait=false);
    inline bool release(shmempipeMessage * pBuf);
    inline bool send(shmempipeMessage * pBuf);
    void getStats(shmempipeStats * pStats, bool zero=false);
};

// inline implementations below

inline
shmempipePoolInfo :: shmempipePoolInfo(void)
{
    numPools = 0;
}

inline
bool shmempipePoolInfo :: addPool(uint32_t numBuf, uint32_t bufSize)
{
    if (numPools == maxPools)
        return false;
    bufSizes[numPools] = bufSize;
    numBufs[numPools] = numBuf;
    numPools++;
    return true;
}

inline
shmempipeFilename :: shmempipeFilename(void)
{
    filename[0] = 0;
    m2sname[0] = 0;
    s2mname[0] = 0;
}

inline void
shmempipeFilename :: setPipeName(const char * _pipename)
{
    snprintf(filename,sizeof(filename),
             SHMEMPIPE_FILEPATH_FMT, _pipename);
    filename[FILENAMEMAX-1] = 0;
    snprintf(m2sname,sizeof(m2sname),
             SHMEMPIPE_M2SPATH_FMT, _pipename);
    m2sname[FILENAMEMAX-1] = 0;
    snprintf(s2mname,sizeof(s2mname),
             SHMEMPIPE_S2MPATH_FMT, _pipename);
    s2mname[FILENAMEMAX-1] = 0;
}

inline void
shmempipeStats :: init(void)
{
    memset(this, 0, sizeof(*this));
}

inline
shmempipeStats :: shmempipeStats(void)
{
    init();
}

inline bool
shmempipeBufferList :: lock(void)
{
    int ret;
    ret = pthread_mutex_lock( &mutex );
    if (ret < 0)
    {
#if HAVE_PTHREAD_MUTEX_CONSISTENT_NP
        if (ret == EOWNERDEAD)
        {
            pthread_mutex_consistent_np(&mutex);
            pthread_mutex_unlock( &mutex );
        }
#endif
        return false;
    }
    return true;
}

inline void
shmempipeBufferList :: unlock(void)
{
    pthread_mutex_unlock( &mutex );
}

inline bool
shmempipeBufferList :: empty(void)
{
    return head == 0;
}

inline void
shmempipeBufferList :: poke(void)
{
    lock();
    needspoke = true;
    unlock();
    pthread_cond_signal( &empty_cond );
}

inline uint32_t
shmempipeBufferList :: get_count(void)
{
    return count;
}

inline bool
shmempipeBufferList :: enqueue( uintptr_t base,
                               shmempipeMessage * msg,
                               bool *pSignalSent,
                               bool debug,
                               bool needlock /*=true*/ )
{
    uint32_t msgoff = ((uintptr_t)msg) - base;
    bool signal = false;
    msg->next = 0;
    if (needlock)
        if (lock() == false)
            return false;
    if (tail != 0)
    {
        shmempipeMessage * tailmsg = (shmempipeMessage *)(tail + base);
        tailmsg->next = msgoff;
        tail = msgoff;
    }
    else
    {
        head = tail = msgoff;
    }
    signal = bIsWaiting;
    bIsWaiting = false;
    count++;
    if (needlock)
        unlock();
    if (signal)
    {
        pthread_cond_broadcast(&empty_cond);
        if (pSignalSent)
            *pSignalSent = true;
    }
    return true;
}

inline shmempipeMessage *
shmempipeBufferList :: dequeue(uintptr_t base, bool *pSignalled,
                              bool bWait, bool debug)
{
    uint32_t msgoff;
    shmempipeMessage * pMsg = NULL;
    if (lock() == false)
        return NULL;
    if (head == 0 && !needspoke)
    {
        if (bWait)
        {
            bIsWaiting = true;
            pthread_cond_wait(&empty_cond, &mutex);
            bIsWaiting = false;
            if (pSignalled)
                *pSignalled = true;
        }
    }
    if (head != 0 && !needspoke)
    {
        msgoff = head;
        pMsg = (shmempipeMessage *)(msgoff + base);
        head = pMsg->next;
        if (head == 0)
            tail = 0;
        count--;
    }
    if (pMsg == NULL)
        needspoke = false;
    unlock();
    return pMsg;
}

inline void
shmempipe :: lockStats(void)
{
    pthread_mutex_lock  ( &m_statsMutex );
}

inline void
shmempipe :: unlockStats(void)
{
    pthread_mutex_unlock( &m_statsMutex );
}

inline shmempipeMessage *
shmempipe :: allocSize(uint32_t size, bool bWait /*=false */)
{
    int poolInd;
    for (poolInd = 0; poolInd < m_pHeader->poolInfo.numPools; poolInd++)
    {
        if (m_pHeader->poolInfo.bufSizes[poolInd] >= size)
            break;
    }
    if (poolInd == m_pHeader->poolInfo.numPools)
    {
        lockStats();
        m_stats.alloc_fails++;
        unlockStats();
        return NULL;
    }
    shmempipeMessage * ret = 
        m_pHeader->pools[poolInd].dequeue(m_shmemPtr, NULL, bWait, false);
    if (ret == NULL)
    {
        lockStats();
        m_stats.alloc_fails++;
        unlockStats();
    }
    return ret;
}

inline shmempipeMessage *
shmempipe :: allocPool(uint32_t poolInd, bool bWait /*=false*/ )
{
    if (poolInd < 0 || poolInd >= m_pHeader->poolInfo.numPools)
    {
        lockStats();
        m_stats.alloc_fails++;
        unlockStats();
        return NULL;
    }
    shmempipeMessage * ret = 
        m_pHeader->pools[poolInd].dequeue(m_shmemPtr, NULL, bWait, false);
    if (ret == NULL)
    {
        lockStats();
        m_stats.alloc_fails++;
        unlockStats();
    }
    return ret;
}

inline bool
shmempipe :: release(shmempipeMessage * pBuf)
{
    return 
        m_pHeader->pools[pBuf->poolInd].enqueue(m_shmemPtr,pBuf,NULL,false);
}

inline bool
shmempipe :: send(shmempipeMessage * pBuf)
{
    if (m_bConnected == false)
        return false;
    bool signalSent = false;
    if (m_otherBufferList->enqueue(m_shmemPtr,pBuf,&signalSent,true) == false)
        return false;
    lockStats();
    m_stats.sent_bytes += pBuf->messageSize;
    m_stats.sent_packets ++;
    if (signalSent)
        m_stats.sent_signals ++;
    unlockStats();
    return true;
}
