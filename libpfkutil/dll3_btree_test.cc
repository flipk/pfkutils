#if 0
set -e -x
g++ -g3 -rdynamic dll3_btree_test.cc dll3_btree.cc -lpthread -o dll3_btree_test
./dll3_btree_test
exit 0
#endif

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

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "dll3.h"
#include "dll3_btree.h"

/*
  #define BTORDER 13
  #define MAX 1000000
  with these parameters we get
   - 149000 inserts per second
   - 243902 lookups per second
*/

#define NUMS 1

#if NUMS==1
#define BTORDER 25
#define MAX     500000
#define REPS    5000000
#elif NUMS==2
#define BTORDER 5
#define MAX     500000
#define REPS    50000000
#endif

struct thing;
class thingBtreeComparator;

typedef DLL3::BTREE<thing,int,
                   thingBtreeComparator, 0, BTORDER> BT;

struct thing : public BT::Links {
    thing( int _v ) { v = _v; inlist = false; }
    int v;
    bool inlist;
};

class thingBtreeComparator {
public:
    static int key_compare( thing * item, int key ) {
        if (item->v < key) return 1;
        if (item->v > key) return -1;
        return 0;
    }
    static int key_compare( thing * item, thing * item2 ) {
        if (item->v < item2->v) return 1;
        if (item->v > item2->v) return -1;
        return 0;
    }
    static char * key_format( thing * item ) {
        static char string[20];
        sprintf(string,"%d",item->v);
        return string;
    }
};

struct memory_block;

typedef DLL3::List<memory_block,0,false,true> memory_block_list_t;

struct memory_block : public memory_block_list_t::Links {
    size_t size;
    char buf[0];

    memory_block(size_t sz) { size = sz; }
    void * operator new(size_t sz, int real_size) {
        return (void*) 
            ::malloc(real_size + sizeof(memory_block));
    }
    void operator delete(void * ptr) {
        ::free(ptr);
    }
    void * get_ptr(void) {
        return (void*)((char*)this + sizeof(memory_block_list_t::Links) + sizeof(size));
    }
    static memory_block * get_block(void * _ptr) {
        memory_block * b = (memory_block *) _ptr;
        char * ptr = (char*)_ptr;
        ptr -= (sizeof(memory_block_list_t::Links) + sizeof(b->size));
        b = (memory_block *) ptr;
        return b;
    }
};

class memory_manager {
    memory_block_list_t items;
    size_t size;
    size_t peak_size;
    int alloc_count;
public:
    memory_manager(void) {
        printf("memory manager initializing\n");
        size = 0;
        peak_size = 0;
        alloc_count = 0;
    }
    ~memory_manager(void) {
        printf("alloc_count %d, items %d, size %ld, peak %ld\n",
               alloc_count, 
               items.get_cnt(),
               (long)size, (long)peak_size);
    }
    void * alloc(size_t sz) {
        memory_block * b = new(sz) memory_block(sz);
        items.add_tail(b);
        size += sz;
        if (size > peak_size)
            peak_size = size;
        alloc_count++;
        return b->get_ptr();
    }
    void free(void * ptr) {
        memory_block * b = memory_block::get_block(ptr);
        size -= b->size;
        items.remove(b);
        delete b;
    }
};

memory_manager mgr;

void *
operator new(size_t sz)
{
    return mgr.alloc(sz);
}

void *
operator new[](size_t sz)
{
    return mgr.alloc(sz);
}

void
operator delete(void * ptr)
{
    mgr.free(ptr);
}

void
operator delete[](void * ptr)
{
    mgr.free(ptr);
}

#if 1

thing ** a;

int
main()
{
    BT * bt;
    int i, j, s;
    time_t last, now;

    s = getpid() * time(0);
    s = -2141375889;
    srandom( s );

//    printf( "S %d\n", s );

    time( &last );
    a = new thing*[ MAX ];

    for ( i = 0; i < MAX; i++ )
        a[i] = new thing( i );

    bt = new BT;

    for ( i = 0; i < REPS; i++ )
    {
        j = random() % MAX;

        time( &now );
        if ( now != last )
        {
            printf( "reps %d\n", i );
            last = now;
        }

        if ( a[j]->inlist )
        {
            if ( bt->find( j ) == 0 )
                printf( "not found 1\n" );
//            printf( "remove %d\n", j );
            bt->remove( a[j] );
        }
        else
        {
//            printf( "add %d\n", j );
            bt->add( a[j] );
            if ( bt->find( j ) == 0 )
                printf( "not found 1\n" );
        }

        a[j]->inlist = !a[j]->inlist;

//        bt->printtree();
    }

    for ( i = 0; i < MAX; i++ )
    {
        if ( a[i]->inlist )
            bt->remove( a[i] );
        delete a[i];
    }
    delete[] a;

    bt->printtree();

    delete bt;

    fflush(stdout);
    fflush(stderr);

    return 0;
}
#endif

#if 0
#define BTORDER 5
#define MAX 50
typedef DLL3::BTREE<thing,BTORDER> BT;

int vals[MAX];

int
main()
{
    BT bt;
    thing * a;
    int i;
    struct timeval tv[5];
    double tvfl[5];

    srandom( time(0) * getpid());

    gettimeofday( tv+0, 0 );
    for ( i = 0; i < MAX; i++ )
        vals[i] = i;
    for ( i = 0; i < MAX; i++ )
    {
        int j = random() % MAX;
        int t = vals[i];
        vals[i] = vals[j];
        vals[j] = t;
    }

    gettimeofday( tv+1, 0 );
    for ( i = 0; i < MAX; i++ )
        bt.add( new thing( vals[i] ));
    gettimeofday( tv+2, 0 );
    for ( i = 0; i < MAX; i++ )
    {
        int j = random() % MAX;
        int t = vals[i];
        vals[i] = vals[j];
        vals[j] = t;
    }
    gettimeofday( tv+3, 0 );
    for ( i = 0; i < MAX; i++ )
    {
        a = bt.find( vals[i] );
        if ( !a || a->v != vals[i] )
            printf( "not found\n" );
    }
    gettimeofday( tv+4, 0 );

    for ( i = 0; i < 5; i++ )
        tvfl[i] =
            (double)(tv[i].tv_sec) + 
            ((double)(tv[i].tv_usec) / (double)1000000);

    for ( i = 1; i < 5; i++ )
        printf( "%5.2f\n", tvfl[i] - tvfl[i-1]);

    return 0;
}
#endif