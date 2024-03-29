#if 0
set -e -x
g++ -Wall -g3 -c thread_slinger.cc
g++ -Wall -g3 -c thread_slinger_test.cc
g++ -Wall -g3 -c signal_backtrace.cc
g++ -g3 thread_slinger_test.o thread_slinger.o signal_backtrace.o -o tts -lpthread -lrt
exit 0
#endif
/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
*/

#include "thread_slinger.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

struct myMessage : public ThreadSlinger::thread_slinger_message
{
    int a; // some field
    int b; // some other field
    void init(int _a, int _b) { a=_a; b=_b; }
    void cleanup(void) { a=0; b=0; }
};

typedef ThreadSlinger::thread_slinger_queue<myMessage> myMsgQ;
typedef ThreadSlinger::thread_slinger_pool <myMessage,int,int> myMsgPool;

myMsgPool *p  = NULL;
myMsgQ    *q  = NULL;
myMsgQ    *q2 = NULL;

void * t1( void * dummy )
{
    uintptr_t  val = (uintptr_t) dummy;
    while (1)
    {
        myMessage * m = p->alloc(random()%5000,false,1,2);
        if (m)
        {
            m->ref();
            q->enqueue(m);
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
        myMessage * m = q->dequeue(random()%10000);
        if (m)
        {
            if (val == 0)
                printf(".");
            else
                printf(",");
            m->deref(); //p.release(m);
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
        myMessage * m = p->alloc(random()%5000,false,3,4);
        if (m)
        {
            m->ref();
            if (val == 0)
                q->enqueue(m);
            else
                q2->enqueue(m);
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
    myMsgQ * qs[2];

    qs[0] = q;
    qs[1] = q2;

    while (1)
    {
        int which;
        myMessage * m = myMsgQ::dequeue(qs,2,(int)(random()%1000),&which);
        if (m)
        {
            if (which == 0)
                printf(".");
            else
                printf(",");
            m->deref(); //p.release(m);
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

int main(int argc, char ** argv)
{
    if (argc != 2)
    {
        printf("usage: thread_slinger_test <test#>\n");
        return 1;
    }
    int test = atoi(argv[1]);

    pthread_t id;

    p = new myMsgPool;

    if (test == 1)
    {
        printf("for this test you should see all eight of these chars: +=-_.,!?\n");
        q = new myMsgQ;
        p->add(2);
        pthread_create( &id, NULL, t1, (void*) 0 );
        pthread_create( &id, NULL, t1, (void*) 1 );
        pthread_create( &id, NULL, t2, (void*) 0 );
        pthread_create( &id, NULL, t2, (void*) 1 );
        pthread_join(id,NULL);
        delete q;
    }
    else if (test == 2)
    {
        printf("for this test you should see all seven of these chars: +=-_.,!\n");
        q  = new myMsgQ;
        q2 = new myMsgQ;
        p->add(1);
        pthread_create( &id, NULL, t3, (void*) 0 );
        pthread_create( &id, NULL, t3, (void*) 1 );
        pthread_create( &id, NULL, t4, (void*) 0 );
        pthread_join(id,NULL);
        delete q;
        delete q2;
    }

    delete p;
    return 0;
}
