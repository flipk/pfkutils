/*
    This file is part of the "pkutils" tools written by Phil Knaack
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

#include "pk_threads.H"

#include <stdio.h>
#include <unistd.h>

class test1 : public PK_Thread {
    void entry( void );
    int set_timer( void );
    int qid;
public:
    test1( void ) { set_name( (char*)"test1" ); resume(); }
};

PkMsgIntDef( tst_timr, 0x1234,
             int a;
    );

int
test1 :: set_timer( void )
{
    tst_timr * t;
    t = new tst_timr;
    t->src_q = (unsigned int)-1;
    t->dest_q = qid;
    return timer_create(5,qid,t);
}

void
test1 :: entry( void )
{
    union {
        pk_msg_int * m;
        PK_FD_Activity * fdact;
        tst_timr * tmr;
    } m;
    int timer_id;

    qid = msg_create( (char*)"q1" );
    timer_id = set_timer();
    register_fd( 0, PK_FD_Read, qid, NULL );

    bool done = false;
    while (!done)
    {
        m.m = msg_recv( 1, &qid, NULL, -1 );
        switch (m.m->type)
        {
        case PK_FD_Activity::TYPE:
        {
            char buf[100];
            int cc = read(0,buf,100);
            printf("test1 : read returned %d\n", cc);
            if (cc <= 0)
                done = true;
            else
                // only re-register if we got an indication!
                register_fd( 0, PK_FD_Read, qid, NULL );
            break;
        }
        case tst_timr::TYPE:
            printf("timer!\n");
            timer_id = set_timer();
            break;
        default:
            printf("unknown message type %#x\n", m.m->type);
        }

        delete m.m;
    }

    msg_destroy(qid);
    timer_cancel(timer_id);
}

int
main()
{
    new PK_Threads( 10 );

    new test1;

    th->run();

    delete th;
    return 0;
}
