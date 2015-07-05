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

#include "shmempipe.H"
#include "shmempipe_test_msg.H"

class myHandler : public shmempipeHandler {
public:
    bool done;
    time_t last_print;
    time_t start;
#ifdef SHMEMPIPE_STATS
    shmempipeStats stats;
#endif
    myHandler(void) { 
        done = false;
        last_print = start = 0;
    }
    /*virtual*/ void messageHandler(shmempipeMessage * _pMsg) {
        done = true;
    }
    /*virtual*/ void connectHandler(void) {
        printf("connected\n");
    }
    /*virtual*/ void disconnectHandler(void) {
        printf("disconnected\n");
        done = true;
    }
};

int
main()
{
    myHandler handler;
    shmempipeSlaveConfig CONFIG;
    shmempipe * pPipe;
    int seqno = 1;
    CONFIG.setPipeName( "shmempipe_test" );
    CONFIG.flushUsecs = 10000;
    CONFIG.pHandler = &handler;
    pPipe = new shmempipe( &CONFIG );
    if (!CONFIG.bInitialized)
    {
        printf("error constructing shmempipe\n");
        return 1;
    }
    MyTestMsg * pMsg = NULL;
    while (!handler.done)
    {
        pMsg = MyTestMsg::allocSize(pPipe);
        if (pMsg)
        {
            int cc = read(0, &pMsg->data, MyTestMsg::max_size);
            if (cc <= 0)
                break;
            pMsg->messageSize = cc;
            pMsg->seqno = seqno++;
            pPipe->send(pMsg, false);
        }
        else
        {
            printf("out of buffers\n");
            break;
        }
#ifdef SHMEMPIPE_STATS
        time_t now = time(0);
        int delta = (int)(now - info.start);
        if (now != info.last_print)
        {
            info.pPipe->getStats(&info.stats, true);

            fprintf(stderr,
                    " sb %ld sp %ld rb %ld rp %ld"
                    " sm %ld rm %ld br %ld bf %ld aw %ld         \r",
                    info.stats.sent_bytes,
                    info.stats.sent_packets,
                    info.stats.rcvd_bytes,
                    info.stats.rcvd_packets,
                    info.stats.sent_messages,
                    info.stats.rcvd_messages,
                    info.stats.buffers_read,
                    info.stats.buffers_flushed,
                    info.stats.alloc_waits );
            info.last_print = now;
        }
#endif
    }
    pMsg->seqno = seqno++;
    pMsg->messageSize = 0;
    pPipe->send(pMsg, true);
    while (!handler.done)
    {
        usleep(10000);
    }
    delete pPipe;
    return 0;
}
