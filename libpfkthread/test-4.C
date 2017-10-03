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

#include "pk_threads.H"

#include <stdio.h>
#include <unistd.h>


class MyType2 : public BST {
public:
	MyType2(BST * parent)
        : BST(parent), four(this), five(this) { }
	BST_BINARY     four;
	BST_STRING     five;
};

class MyType3 : public BST_UNION {
public:
	enum { EIGHT, NINE, MAX };
	MyType3(BST * parent)
        : BST_UNION(parent, MAX), eight(this), nine(this) { }
	BST_STRING  eight;
	BST_UINT16_t nine;
};

class MyType : public BST {
public:
	MyType(BST * parent)
        : BST(parent),
          one(this), two(this), three(this), six(this), seven(this) { }
	BST_UINT64_t  one;
	BST_UINT16_t  two;
	BST_POINTER<MyType2>  three;
	BST_ARRAY<BST_UINT16_t> six;
	MyType3  seven;
};

PkMsgExtDef( MyMessage, 0x1234, MyType );

class test1 : public PK_Thread, PK_Message_Ext_Handler {
    void entry( void );
    /*virtual*/ pk_msg_ext * make_msg( UINT16 type ) {
        switch (type)
        {
        case MyMessage::TYPE: return new MyMessage;
        default:
            printf("test1 unknown msg type %#x received\n", type);
        }
        return NULL;
    }
public:
    test1( void ) { set_name( (char*)"test1" ); resume(); }
};

void
fill_mm(MyMessage * mm)
{
    mm->body.one.v = 0x123456789abcdef0ULL;
    mm->body.two.v = 0x5531U;
    mm->body.three.pointer = new MyType2(NULL);
    mm->body.three.pointer->four.alloc(4);
    mm->body.three.pointer->four.binary[0] = 1;
    mm->body.three.pointer->four.binary[0] = 2;
    mm->body.three.pointer->four.binary[0] = 3;
    mm->body.three.pointer->four.binary[0] = 4;
    mm->body.three.pointer->five.set((char*)"this is a test string");
    mm->body.six.alloc(4);
    mm->body.six.array[0]->v = 0x8765U;
    mm->body.six.array[1]->v = 0x2344U;
    mm->body.six.array[2]->v = 0x5674U;
    mm->body.six.array[3]->v = 0x9898U;

#if 1
    mm->body.seven.which.v = MyType3::EIGHT;
    mm->body.seven.eight.set((char*)"this is another test string");
#else
    mm->body.seven.which.v = MyType3::NINE;
    mm->body.seven.nine.v = 0x7676U;
#endif
}

void
test1 :: entry( void )
{
    printf("test1 thread starting\n");

    PK_Message_Ext_Link_TCP * tcp =
        new PK_Message_Ext_Link_TCP(this, (char*) "10.0.0.2", 2005);

    if (!tcp->Connected())
    {
        printf("NOT CONNECTED\n");
    }
    else
    {
        MyMessage * mm;

        mm = new MyMessage;
        fill_mm(mm);

        if (!tcp->send(mm))
            printf("test1 send failed\n");

        mm = new MyMessage;
        fill_mm(mm);

        if (!tcp->send(mm))
            printf("test1 send failed\n");

        mm = new MyMessage;
        fill_mm(mm);

        if (!tcp->send(mm))
            printf("test1 send failed\n");

        sleep(5);
    }

    delete tcp;
    printf("test1 thread dying\n");
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
