
#include "pk_threads.h"

#include <stdio.h>
#include <unistd.h>


class TestMsg1BST : public BST {
public:
    TestMsg1BST(BST * parent)
        : BST(parent), one(this), two(this) { }
    BST_UINT32_t one;
    BST_UINT16_t two;
};
PkMsgExtDef( TestMsg1, 0x1234, TestMsg1BST );

class TestMsg2BST : public BST {
public:
    TestMsg2BST(BST * parent)
        : BST(parent), one(this), two(this) { }
    BST_UINT32_t one;
    BST_UINT16_t two;
};
PkMsgExtDef( TestMsg2, 0x1235, TestMsg2BST );


class test1 : public PK_Thread, PK_Message_Ext_Handler {
    void entry( void );
    /*virtual*/ pk_msg_ext * make_msg( uint16_t type ) {
        switch (type)
        {
        case TestMsg1::TYPE: return new TestMsg1;
        case TestMsg2::TYPE: return new TestMsg2;
        default:
            printf("test1 unknown msg type %#x received\n", type);
        }
        return NULL;
    }
public:
    test1( void ) { set_name( (char*)"test1" ); resume(); }
};

void
test1 :: entry( void )
{
    printf("test1 thread starting\n");

    sleep(5);

    PK_Message_Ext_Link_TCP * tcp =
        new PK_Message_Ext_Link_TCP(this, (char*) "127.0.0.1", 2005);

    sleep(5);

    TestMsg1 * tm1;
    tm1 = new TestMsg1;
    tm1->body.one() = 1;
    tm1->body.two() = 2;
    if (!tcp->send(tm1))
        printf("test1 send failed\n");

    TestMsg2 * tm2;
    tm2 = new TestMsg2;
    tm2->body.one() = 3;
    tm2->body.two() = 4;
    if (!tcp->send(tm2))
        printf("test1 send failed\n");

    tm1 = new TestMsg1;
    tm1->body.one() = 5;
    tm1->body.two() = 6;
    if (!tcp->send(tm1))
        printf("test1 send failed\n");

    sleep(5);
    delete tcp;
    printf("test1 thread dying\n");
}

class test2 : public PK_Thread, PK_Message_Ext_Handler {
    void entry( void );
    /*virtual*/ pk_msg_ext * make_msg( uint16_t type ) {
        switch (type)
        {
        case TestMsg1::TYPE: return new TestMsg1;
        case TestMsg2::TYPE: return new TestMsg2;
        default:
            printf("test2 unknown msg type %#x received\n", type);
        }
        return NULL;
    }
public:
    test2( void ) { set_name( (char*)"test2" ); resume(); }
};

void
test2 :: entry( void )
{
    printf("test2 thread starting\n");

    PK_Message_Ext_Link_TCP * tcp =
        new PK_Message_Ext_Link_TCP(this, 2005);

    union {
        pk_msg_ext * msg;
        TestMsg1 * tm1;
        TestMsg2 * tm2;
    } u;

    while (true)
    {
        u.msg = tcp->recv(10);

        if (!u.msg)
        {
            printf("test2 recv got null msg\n");
            break;
        }

        switch (u.msg->hdr.get_type())
        {
        case TestMsg1::TYPE:
            printf("test2 got TestMsg1, one=%d two=%d\n",
                   u.tm1->body.one(), u.tm1->body.two());
            break;

        case TestMsg2::TYPE:
            printf("test2 got TestMsg2, one=%d two=%d\n",
                   u.tm2->body.one(), u.tm2->body.two());
            break;

        default:
            printf("test2 unknown msg of type %#x received\n",
                   u.msg->hdr.get_type());
        }

        delete u.msg;
    }

    delete tcp;
    printf("test2 thread dying\n");
}

int
main()
{
    new PK_Threads( 10 );
    new test1;
    new test2;

    th->run();

    delete th;
    return 0;
}
