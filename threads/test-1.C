
#include "pk_threads.H"

#include <stdio.h>
#include <unistd.h>

class test1 : public PK_Thread {
    void entry( void );
public:
    test1( void ) { set_name( "test1" ); resume(); }
};

class test2 : public PK_Thread {
    void entry( void );
public:
    test2( void ) { set_name( "test2" ); resume(); }
};

PkMsgIntDef( testmsg, 4,
             int a;
    );

void
test1 :: entry( void )
{
    int qids[2], qi;
    qids[0] = msg_lookup( "q1" );
    qids[1] = msg_lookup( "q2" );

    pk_msg_int * pmi;
    pmi = msg_recv( 2, qids, &qi, -1 );
    printf( "test1 got msg on q %d : type = %d\n",
            qi, pmi->type );
    delete pmi;
    pmi = msg_recv( 2, qids, &qi, -1 );
    printf( "test1 got msg on q %d : type = %d\n",
            qi, pmi->type );
    delete pmi;
    pmi = msg_recv( 2, qids, &qi, 5 );
}

void
test2 :: entry( void )
{
    int qids[2];
    qids[0] = msg_lookup( "q1" );
    qids[1] = msg_lookup( "q2" );

    testmsg * m;
    m = new testmsg;
    m->a = 5;
    sleep( 1 );
    timer_create( 5, qids[0], m );
    m = new testmsg;
    m->a = 5;
    sleep( 1 );
    msg_send( qids[0], m );
}

class test3 : public PK_Thread {
    void entry( void );
public:
    test3( void ) { resume(); }
};

void
test3 :: entry( void )
{
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

int
main()
{
    new PK_Threads( 10 );

    int qid1 = PK_Thread::msg_create( "q1" );
    int qid2 = PK_Thread::msg_create( "q2" );

    new test3;
    th->run();

    PK_Thread::msg_destroy( qid1 );
    PK_Thread::msg_destroy( qid2 );

    delete th;
    return 0;
}
