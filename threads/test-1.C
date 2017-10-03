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
public:
    test1( void ) { set_name( (char*)"test %d", 1 ); resume(); }
};

class test2 : public PK_Thread {
    void entry( void );
public:
    test2( void ) { set_name( (char*)"test %d", 2 ); resume(); }
};

PkMsgIntDef( testmsg, 4,
             int a;
    );

void
test1 :: entry( void )
{
    int qids[2], qi;
    qids[0] = msg_lookup( (char*)"q %d", 1 );
    qids[1] = msg_lookup( (char*)"q %d", 2 );

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

void
test2 :: entry( void )
{
    int qids[2];
    qids[0] = msg_lookup( (char*)"q %d", 1 );
    qids[1] = msg_lookup( (char*)"q %d", 2 );

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

    int qid1 = PK_Thread::msg_create( (char*)"q %d", 1 );
    int qid2 = PK_Thread::msg_create( (char*)"q %d", 2 );

    new test3;
    th->run();

    PK_Thread::msg_destroy( qid1 );
    PK_Thread::msg_destroy( qid2 );

    delete th;
    return 0;
}
