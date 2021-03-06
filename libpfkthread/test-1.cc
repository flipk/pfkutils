
#include <stdio.h>
#include <unistd.h>
#include "pk_threads.h"
PkMsgIntDef( testmsg, 4,
             int a;
    );
class test1 : public PK_Thread {
    void entry( void ) {
        int qids[2], qi;
        qids[0] = msg_lookup( "q %d", 1 );
        qids[1] = msg_lookup( "q %d", 2 );
        union {
            pk_msg_int * m;
            testmsg * tm;
        } m;
        m.m = msg_recv( 2, qids, &qi, -1 );
        printf( "test1 got msg on q %d : type = %d a = %d (expected q=1 a=4)\n",
                qi, m.m->type, m.tm->a );
        delete m.m;
        m.m = msg_recv( 2, qids, &qi, -1 );
        printf( "test1 got msg on q %d : type = %d a = %d (expected q=0 a=5)\n",
                qi, m.m->type, m.tm->a );
        delete m.m;
        m.m = msg_recv( 2, qids, &qi, 5 );
        printf( "received ptr = %#lx (expected null)\n", (unsigned long)m.m);
    }
public:
    test1( void ) {
        set_name( "test %d", 1 ); resume();
    }
};
class test2 : public PK_Thread {
    void entry( void ) {
        int qids[2];
        qids[0] = msg_lookup( "q %d", 1 );
        qids[1] = msg_lookup( "q %d", 2 );
        testmsg * m;
        m = new testmsg;
        m->a = 5;
        sleep( 1 );
        timer_create( 5, qids[0], m );
        m = new testmsg;
        m->a = 4;
        sleep( 1 );
        msg_send( qids[1], m );
    }
public:
    test2( void ) {
        set_name( "test %d", 2 );
        resume();
    }
};
class test3 : public PK_Thread {
    void entry( void ) {
        int i;
        printf( "about to sleep\n" );
        for ( i = 10; i >= 0; i-- )
        {
            printf( "%d ", i );
            fflush( stdout );
            sleep( 1 );
        }
        printf( "\ndone sleeping, creating 2 test threads\n" );
        new test1;
        new test2;
    }
public:
    test3( void ) { resume(); }
};
int main() {
    new PK_Threads( 10 );
    int qid1 = PK_Thread::msg_create( "q %d", 1 );
    int qid2 = PK_Thread::msg_create( "q %d", 2 );
    new test3;
    th->run();
    PK_Thread::msg_destroy( qid1 );
    PK_Thread::msg_destroy( qid2 );
    delete th;
    return 0;
}
