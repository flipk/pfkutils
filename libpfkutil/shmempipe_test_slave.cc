/*
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

#include <stdio.h>
#include <unistd.h>

#include "shmempipe.h"
#include "shmempipe_test_msg.h"

bool connected = false;
int count=0;
bool done = false;

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
    if (!done)
    {
        pMsg = MyTestMsg::allocSize(pPipe);
        if (pMsg)
        {
            pMsg->seqno = 8;
            pPipe->send(pMsg);
        }
    }
    count++;
    if (count >= 10000000)
        done=true;
}

int
main()
{
    shmempipeSlaveConfig CONFIG;
    shmempipe * pPipe;

    CONFIG.setPipeName( "shmempipe_test" );
    CONFIG.connectCallback = &connect;
    CONFIG.disconnectCallback = &disconnect;
    CONFIG.messageCallback = &message;
    CONFIG.arg = NULL;
    pPipe = new shmempipe( &CONFIG );
    if (!CONFIG.bInitialized)
    {
        printf("error constructing shmempipe\n");
        return 1;
    }

    while (!connected && !done)
    {
        pfk_select sel;
        sel.rfds.set(0);
        sel.tv.set(0,100000);
        if (sel.select() <= 0)
            continue;
        if (sel.rfds.isset(0))
            done = true;
    }

    if (!done)
        for (int counter=0; counter < 50; counter++)
        {
            MyTestMsg * pMsg = MyTestMsg::allocSize(pPipe);
            pMsg->seqno = 1;
            pPipe->send(pMsg);
        }

    while (connected && !done)
    {
        shmempipeStats stats;
        pPipe->getStats(&stats,true);
        printf("sb %lld sp %lld ss %lld rb %lld rp %lld rs %lld "
               "af %lld fb %lld\n",
               stats.sent_bytes, stats.sent_packets, stats.sent_signals,
               stats.rcvd_bytes, stats.rcvd_packets, stats.rcvd_signals,
               stats.alloc_fails, stats.free_buffers);

        pfk_select sel;
        sel.rfds.set(0);
        sel.tv.set(0,100000);
        if (sel.select() <= 0)
            continue;
        if (sel.rfds.isset(0))
            done = true;
    }

    delete pPipe;
    return 0;
}
