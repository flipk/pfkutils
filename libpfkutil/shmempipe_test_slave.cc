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
            pPipe->enqueue(pMsg);
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

    CONFIG.file.setPipeName( "shmempipe_test" );
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

    for (int counter=0; counter < 50; counter++)
    {
        MyTestMsg * pMsg = MyTestMsg::allocSize(pPipe);
        pMsg->seqno = 1;
        pPipe->enqueue(pMsg);
    }

    while (connected && !done)
    {
        printf("\r count = %d    ", count);
        fflush(stdout);
        usleep(100000);
    }

    delete pPipe;
    return 0;
}
