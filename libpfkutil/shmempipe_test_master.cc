#if 0
set -e -x
opt=-O3
g++ -Wall $opt -c shmempipe.cc
g++ -Wall $opt -c shmempipe_test_master.cc
g++ -Wall $opt -c shmempipe_test_slave.cc
g++ $opt shmempipe_test_master.o shmempipe.o -o tm -lpthread
g++ $opt shmempipe_test_slave.o shmempipe.o -o ts -lpthread
exit 0
#endif

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
#include <unistd.h>

#include "shmempipe.H"
#include "shmempipe_test_msg.H"

bool connected = false;
int count = 0;

void connect(shmempipe *pPipe, void *arg)
{
    printf("got connect\n");
    connected = true;
}

void disconnect(shmempipe *pPipe, void *arg)
{
    printf("got disconnect\n");
    connected = false;
}

void message(shmempipe * pPipe, void *arg, shmempipeMessage * _pMsg)
{
    MyTestMsg * pMsg = (MyTestMsg *) _pMsg;
    pPipe->release(pMsg);
    pMsg = MyTestMsg::allocSize(pPipe);
    if (pMsg)
    {
        pMsg->seqno = 8;
        pPipe->send(pMsg);
    }
    count ++;
}

int
main()
{
    shmempipeMasterConfig  CONFIG;
    shmempipe * pPipe;
    CONFIG.file.setPipeName( "shmempipe_test" );
    CONFIG.poolInfo.addPool(500, sizeof(MyTestMsg));
    CONFIG.callbacks.connectCallback = &connect;
    CONFIG.callbacks.disconnectCallback = &disconnect;
    CONFIG.callbacks.messageCallback = &message;
    CONFIG.callbacks.arg = NULL;
    pPipe = new shmempipe( &CONFIG );
    if (!CONFIG.bInitialized)
    {
        printf("error constructing shmempipe\n");
        return 1;
    }
    while (!connected)
        usleep(1);
    do {
        shmempipeStats stats;
        pPipe->getStats(&stats,true);
        printf("sb %lld sp %lld ss %lld rb %lld rp %lld rs %lld "
               "af %lld fb %lld\n",
               stats.sent_bytes, stats.sent_packets, stats.sent_signals,
               stats.rcvd_bytes, stats.rcvd_packets, stats.rcvd_signals,
               stats.alloc_fails, stats.free_buffers);
        usleep(100000);
    } while (connected);
    delete pPipe;
    return 0;
}
