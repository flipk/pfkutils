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

#include "thread_slinger.H"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

struct myMessage : public thread_slinger_message
{
    int a; // some field
    int b; // some other field
};

thread_slinger_pool<myMessage,2>  p;

thread_slinger_queue<myMessage>   q;

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

int
main()
{
    pthread_t id;
    pthread_create( &id, NULL, t1, (void*) 0 );
    pthread_create( &id, NULL, t1, (void*) 1 );
    pthread_create( &id, NULL, t2, (void*) 0 );
    pthread_create( &id, NULL, t2, (void*) 1 );
    pthread_join(id,NULL);
    return 0;
}
