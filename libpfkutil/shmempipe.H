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

struct shmempipePoolInfo {
    static const int maxPools = 64;
    uint8_t numPools;
    uint16_t bufSizes[maxPools]; // specify in increasing order.
    uint16_t numBufs[maxPools];
    shmempipePoolInfo(void) { numPools = 0; }
    bool addPool(uint16_t numBuf, uint16_t bufSize) {
        if (numPools == maxPools)
            return false;
        bufSizes[numPools] = bufSize;
        numBufs[numPools] = numBuf;
        numPools++;
        return true;
    }
};

#define SHMEMPIPE_FILEPATH_FMT "/tmp/p2p.%s.shmem"
#define SHMEMPIPE_M2SPATH_FMT "/tmp/p2p.%s.m2s"
#define SHMEMPIPE_S2MPATH_FMT "/tmp/p2p.%s.s2m"

struct shmempipeFilename {
    static const int FILENAMEMAX = 512;
    char filename[FILENAMEMAX];
    char m2sname[FILENAMEMAX];
    char s2mname[FILENAMEMAX];
    shmempipeFilename(void) {
        filename[0] = 0;
        m2sname[0] = 0;
        s2mname[0] = 0;
    }
    void setPipeName(const char * _pipename) {
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
};

class shmempipe;
struct shmempipeMessage;

typedef void (*shmempipeConnectCallback)(shmempipe *, void *arg);
typedef void (*shmempipeMessageCallback)(shmempipe *, void *arg,
                                         shmempipeMessage * pMsg);

struct shmempipeCallbacks {
    shmempipeConnectCallback connectCallback;
    shmempipeConnectCallback disconnectCallback;
    shmempipeMessageCallback messageCallback;
    void * arg;
};

struct shmempipeMasterConfig {
    shmempipeFilename file;
    shmempipeCallbacks callbacks;
    shmempipePoolInfo poolInfo;
    bool bInitialized;
    shmempipeMasterConfig(void) {
        bInitialized = false;
    }
};

struct shmempipeSlaveConfig {
    shmempipeFilename file;
    shmempipeCallbacks callbacks;
    bool bInitialized;
    shmempipeSlaveConfig(void) {
        bInitialized = false;
    }
};

struct shmempipeMessage {
    friend class shmempipeBufferList;
private:
    uint32_t next; // offset
    uint8_t bMaster;
    uint8_t poolInd;
    uint16_t bufInd;
    uint16_t bufferSize;
public:
    uint16_t messageSize;
private:
    enum { FREE=1, APP, QUEUE } owner;
    friend class shmempipe;
};

struct shmempipeBufferList {
private:
    uint32_t head; // dequeue from head
    uint32_t tail; // enqueue to tail
    // shmempipeMessage::next points towards tail
    pthread_mutex_t  mutex;
    pthread_cond_t   empty_cond;
    bool needspoke;
    void lock(void) { pthread_mutex_lock( &mutex ); }
    void unlock(void) { pthread_mutex_unlock( &mutex ); }
public:
    void init(void);
    bool empty(void) { return head == 0; }
    void poke(void) { lock(); needspoke = true; unlock(); pthread_cond_signal( &empty_cond ); }
    void enqueue( uintptr_t base, shmempipeMessage * msg,
                  bool needlock=true ) {
        uint32_t msgoff = ((uintptr_t)msg) - base;
        bool signal = false;
        msg->next = 0;
        if (needlock)
            lock();
        if (tail != 0) {
            shmempipeMessage * tailmsg = (shmempipeMessage *)(tail + base);
            tailmsg->next = msgoff;
            tail = msgoff;
        } else {
            head = tail = msgoff;
            signal = true;
        }
        if (needlock)
            unlock();
        if (signal)
            pthread_cond_signal(&empty_cond);
    }
    shmempipeMessage * dequeue(uintptr_t base,
                               bool needlock=true) {
        uint32_t msgoff;
        shmempipeMessage * pMsg = NULL;
        if (needlock)
            lock();
        if (head == 0 && !needspoke) {
            pthread_cond_wait(&empty_cond, &mutex);
        }
        if (head != 0 && !needspoke) {
            msgoff = head;
            pMsg = (shmempipeMessage *)(msgoff + base);
            head = pMsg->next;
            if (head == 0)
                tail = 0;
        }
        if (pMsg == NULL)
            needspoke = false;
        if (needlock)
            unlock();
        return pMsg;
    }
};

struct shmempipeHeader {
    shmempipePoolInfo    poolInfo;
    shmempipeBufferList  pools[shmempipePoolInfo::maxPools];
    shmempipeBufferList  master2slave;
    shmempipeBufferList  slave2master;
    // pool buffers start here
};

class shmempipe {
    bool m_bMaster;
    bool m_bConnected;
    shmempipeFilename m_filename;
    shmempipeCallbacks m_callbacks;
//    shmempipeStats m_stats;
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
    bool m_bCloserRunning;
    void startCloserThread(void);
    void stopCloserThread(void);
    static void * _closerThreadEntry(void *arg);
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
    shmempipeMessage * allocSize(int size, int uSecs=0)
    {
        int poolInd;
        for (poolInd = 0;
             poolInd < m_pHeader->poolInfo.numPools;
             poolInd++)
        {
            if (m_pHeader->poolInfo.bufSizes[poolInd] >= size)
                break;
        }
        if (poolInd == m_pHeader->poolInfo.numPools)
            return NULL;
        return m_pHeader->pools[poolInd].dequeue(m_shmemPtr, uSecs);
    }
    shmempipeMessage * allocPool(int poolInd, int uSecs=0)
    {
        if (poolInd < 0 || poolInd >= m_pHeader->poolInfo.numPools)
            return NULL;
        return m_pHeader->pools[poolInd].dequeue(m_shmemPtr, uSecs);
    }
    void release(shmempipeMessage * pBuf)
    {
        m_pHeader->pools[pBuf->poolInd].enqueue(m_shmemPtr,pBuf);
    }
    bool enqueue(shmempipeMessage * pBuf)
    {
        if (m_bConnected == false)
            return false;
        m_otherBufferList->enqueue(m_shmemPtr,pBuf);
        return true;
    }
// xxx   void getStats(shmempipeStats * pStats, bool zero=false);
};
