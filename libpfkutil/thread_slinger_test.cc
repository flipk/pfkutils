#if 0
set -e -x
g++ -Wall -O6 -c thread_slinger.cc
g++ -Wall -O6 -c thread_slinger_test.cc
g++ thread_slinger_test.o thread_slinger.o -o tts -lpthread -lrt
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

// TEST==1 is for testing single-queue dequeue()
// TEST==2 is for testing multi-queue dequeue()

#define TEST 2

#include "thread_slinger.H"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

struct myMessage : public thread_slinger_message
{
    int a; // some field
    int b; // some other field
};

#if TEST==1
thread_slinger_pool<myMessage,2>  p;
#elif TEST==2
thread_slinger_pool<myMessage,5>  p;
#endif

typedef thread_slinger_queue<myMessage> myMsgQ;

myMsgQ q;
myMsgQ q2;

void * t1( void * dummy )
{
    uintptr_t  val = (uintptr_t) dummy;
    while (1)
    {
        myMessage * m = p.alloc(random()%5000);
        if (m)
        {
            q.enqueue(m);
            if (val == 0)
                printf("+");
            else
                printf("=");
        }
        else
        {
            if (val == 0)
                printf("-");
            else
                printf("_");
        }
        fflush(stdout);
        usleep(random()%10000);
    }
    return NULL;
}

void * t2( void * dummy )
{
    uintptr_t  val = (uintptr_t) dummy;
    while (1)
    {
        myMessage * m = q.dequeue(random()%10000);
        if (m)
        {
            if (val == 0)
                printf(".");
            else
                printf(",");
            p.release(m);
        }
        else
        {
            if (val == 0)
                printf("!");
            else
                printf("?");
        }
        fflush(stdout);
        usleep(random()%10000);
    }
    return NULL;
}

void * t3( void * dummy )
{
    uintptr_t  val = (uintptr_t) dummy;
    while (1)
    {
        myMessage * m = p.alloc(random()%5000);
        if (m)
        {
            if (val == 0)
                q.enqueue(m);
            else
                q2.enqueue(m);
            if (val == 0)
                printf("+");
            else
                printf("=");
        }
        else
        {
            if (val == 0)
                printf("-");
            else
                printf("_");
        }
        fflush(stdout);
        usleep(random()%30000);
    }
    return NULL;
}

void * t4(void * dummy)
{
    _thread_slinger_queue * qs[2];

    qs[0] = &q;
    qs[1] = &q2;

    while (1)
    {
        int which;
        myMessage * m = myMsgQ::dequeue((_thread_slinger_queue**)&qs,
                                         2,(int)(random()%1000),&which);
        if (m)
        {
            printf(".");
            p.release(m);
        }
        else
        {
            printf("!");
        }
        fflush(stdout);
        usleep(random()%10000);
    }

    return NULL;
}

int
main()
{
    pthread_t id;
#if TEST==1
    pthread_create( &id, NULL, t1, (void*) 0 );
    pthread_create( &id, NULL, t1, (void*) 1 );
    pthread_create( &id, NULL, t2, (void*) 0 );
    pthread_create( &id, NULL, t2, (void*) 1 );
    pthread_join(id,NULL);
#elif TEST==2
    pthread_create( &id, NULL, t3, (void*) 0 );
    pthread_create( &id, NULL, t3, (void*) 1 );
    pthread_create( &id, NULL, t4, (void*) 0 );
    pthread_join(id,NULL);
#endif
    return 0;
}
