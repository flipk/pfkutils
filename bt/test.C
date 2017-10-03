
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#include "btree.H"

// tests:

// 1. make btree file with 10000 random entries
// 2. read that file back out
// 3. make btree file with small # entries, then delete a few

#define FILENAME "img"
#define CACHESIZE 10, 100, 500

#if TEST==1 || TEST==2
#define BTORDER 9
#elif TEST==3
#define BTORDER 5
#elif TEST==4 || TEST==5
#define BTORDER 21
#endif

#if TEST==4 || TEST==5 || TEST==6
#define KEYDATLEN 13
#define NUMKEYS   100000
#define NUMROUNDS 1000000
#endif

class myprintinfo : public btree_printinfo {
public:
    myprintinfo( void ) :
        btree_printinfo( NODE_INFO | BTREE_INFO |
                         KEY_REC_PTR | DATA_REC_PTR ) { /* nothing */ }
    /*virtual*/ ~myprintinfo( void ) { /*nothing*/ }
    /*virtual*/ char * sprint_element( int noderec,
                                       int keyrec, void * key, int keylen,
                                       int datrec, void * dat, int datlen,
                                       bool * datdirty ) {

        char ret[ 500 ];
        sprintf( ret,
                 "(%09d)key[%09d] = %10s  data[%09d] = %10s\n",
                 noderec, keyrec, key, datrec, dat );
        
        char * ret2 = new char[ strlen( ret ) + 1 ];
        strcpy( ret2, ret );
        return ret2;
    }
    /*virtual*/ void sprint_element_free( char * s ) {
        delete[] s;
    }
    /*virtual*/ void print( char * format, ... )
        __attribute__ ((format( printf, 2, 3 ))) {
        va_list ap;
        va_start( ap, format );
        vprintf( format, ap );
        va_end( ap );
    }
};


int
main( int argc, char ** argv )
{
    FileBlockNumber     * fbn;
    Btree               * bt;

#if TEST==1 || TEST==3 || TEST==4 || TEST==5
    unlink( FILENAME );
#endif

    try {
        fbn    = new FileBlockNumber( FILENAME, CACHESIZE );

#if TEST==1 || TEST==3 || TEST==4 || TEST==5
        Btree::new_file( fbn, BTORDER );
#endif
#if TEST!=7
        bt     = new Btree( fbn );
#endif
    }
    catch ( Btree::constructor_failed r ) {
        printf( "btree open failure %d\n", r.reason );
        return 1;
    }

#if 1
    int srandval = getpid() * time( NULL );
    srandom( srandval );
//    printf( "S %d\n", srandval );
#else
    srandom( (unsigned)1829423164 );
#endif

    myprintinfo pi;

#if TEST==1

    int i;
    for ( i = 0; i < 50000; i++ )
    {
        Btree::rec * r = bt->alloc_rec( 12, 12 );
        sprintf( (char*)r->key.ptr, "%d", random() );
        sprintf( (char*)r->data.ptr, "%d", random() );
        bt->put_rec( r );
    }

#elif TEST==2

    bt->dumptree( &pi );

#elif TEST==3

    Btree::rec * r;

#define INSERT(x,y)                         \
    r = bt->alloc_rec( strlen( #x ) + 1, strlen( #y ) + 1 );    \
    strcpy( (char*)r->key.ptr, #x );                \
    strcpy( (char*)r->data.ptr, #y );               \
    bt->put_rec( r )
    INSERT(01,one);
    INSERT(02,two);
    INSERT(03,three);
    INSERT(04,four);
    INSERT(05,five);
    INSERT(06,six);
    INSERT(07,seven);
    INSERT(08,eight);
    INSERT(09,nine);
    INSERT(10,ten);
    INSERT(11,eleven);
    bt->flush();
//    printf( "*** after inserts, tree state:\n" );
//    bt->dumptree( &pi );

#define DELK(k) \
  bt->delete_rec( (UCHAR*) #k, 3 )
//  printf( "*** after delete " #k "\n" )
//    bt->dumptree( &pi );

    DELK(11);
    DELK(01);
    DELK(06);
    DELK(03);
    DELK(09);
    DELK(08);
    DELK(07);
    DELK(10);
    DELK(02);
    DELK(04);
    DELK(05);
    bt->dumptree( &pi );

    bt->flush();
    {
        int nbn = bt->get_fbn()->alloc( 8000 );
        UINT32 magic;
        int size;
        UCHAR * nbnspace = bt->get_fbn()->get_block( nbn, size, magic );
        memset( nbnspace, 0xd0, size );
        bt->get_fbn()->unlock_block( magic, true );
    }
    bt->flush();

#elif TEST==4

    struct info {
        int key;
        int dat;
        bool present;
        info( void ) { present = false; }
    };
    info infos[ NUMKEYS ];
    setlinebuf( stdout );
    int i;
    for ( i = 0; i < NUMKEYS; i++ )
    {
        infos[i].key = i;
        infos[i].dat = random();
    }

#define TEST4VERBOSE 0

    int lastt = time( NULL );
    int startt = lastt;
    int put = 0;
    int got = 0;
    int del = 0;
    int delta_i = 0;
    int numindb = 0;
    int lines = 20;

    for ( i = 0; i < NUMROUNDS; i++ )
    {
        int j;
        Btree::rec * r;
        int now;

#if TEST4VERBOSE==0
        now = time( NULL );
        if ( lastt != now )
        {
            lastt = now;
            if ( lines == 20 )
            {
                printf( "secs  rounds (delta)  dbsize  "
                        "put  got  del flushed\n" );
                lines = 0;
            }
            lines++;
            printf( "%4d %7d (%5d) %7d %4d %4d %4d %5d\n",
                    now - startt,
                    i, delta_i, numindb,
                    put, got, del, bt->flush());
            put = got = del = delta_i = 0;
        }
#endif

        delta_i++;
        j = random() % NUMKEYS;
        if ( !infos[j].present )
        {
            // add it
            r = bt->alloc_rec( KEYDATLEN, KEYDATLEN );
            memset( r->key.ptr, 0, KEYDATLEN );
            memset( r->data.ptr, 0, KEYDATLEN );
            sprintf( (char*)r->key.ptr, "%d", infos[j].key );
            sprintf( (char*)r->data.ptr, "%d", infos[j].dat );
#if TEST4VERBOSE
            printf( "P %d %d\n", infos[j].key, infos[j].dat );
#endif
            Btree::put_retval x = bt->put_rec( r );
            if ( x != Btree::PUT_NEW )
                printf( " PUT RETURNS %d!\n", x );
            infos[j].present = true;
            put++;
            numindb++;
        }
        else
        {
            if (( random() % 100 ) > 50 )
            {
                // delete it
                char key[ KEYDATLEN ];
                memset( key, 0, KEYDATLEN );
                sprintf( key, "%d", infos[j].key );
#if TEST4VERBOSE
                printf( "D %d\n", infos[j].key );
#endif
                Btree::delete_retval x = bt->delete_rec( (UCHAR*)key,
                                                         KEYDATLEN );
                if ( x != Btree::DELETE_OK )
                    printf( " DELETE RETURNS %d!\n", x );
                infos[j].present = false;
                del++;
                numindb--;
            }
            else
            {
                // look it up and compare it
                char key[ KEYDATLEN ];
                memset( key, 0, KEYDATLEN );
                sprintf( key, "%d", infos[j].key );
#if TEST4VERBOSE
                printf( "G %d\n", infos[j].key );
#endif
                Btree::rec * r = bt->get_rec( (UCHAR*)key, KEYDATLEN );
                if ( !r )
                    printf( " GET returned NULL!\n" );
                else
                {
                    int v = atoi( (char*)r->data.ptr );
                    if ( v != infos[j].dat )
                        printf( " GET result didn't match (%d != %d)\n",
                                v, infos[j].dat );
                    bt->unlock_rec( r );
                }
                got++;
            }
        }
    }

#elif TEST==5

    FILE * inf;
    inf = fopen( "t4.txt", "r" );
    char l[ 80 ];
    while ( fgets( l, 80, inf ) != NULL )
    {
        Btree::rec * r;
        if ( l[0] == 'Q' )
            break;
        if ( l[1] != ' ' )
            continue;
        int i = atoi( l+2 );
        if ( l[0] == 'P' )
        {
            r = bt->alloc_rec( KEYDATLEN, KEYDATLEN );
            memset( r->key.ptr, 0, KEYDATLEN );
            memset( r->data.ptr, 0, KEYDATLEN );
            sprintf( (char*)r->key.ptr, "%d",  i );
            sprintf( (char*)r->data.ptr, "%d", random() );
//            printf( "P %d\n", i );
            Btree::put_retval x = bt->put_rec( r );
            if ( x != Btree::PUT_NEW )
                printf( " PUT RETURNS %d!\n", x );
        }
        if ( l[0] == 'D' )
        {
            char key[ KEYDATLEN ];
            memset( key, 0, KEYDATLEN );
            sprintf( key, "%d", i );
//            printf( "D %d\n", i );
            Btree::delete_retval x = bt->delete_rec( (UCHAR*)key,
                                                     KEYDATLEN );
            if ( x != Btree::DELETE_OK )
                printf( " DELETE RETURNS %d!\n", x );
        }
    }
    fclose( inf );

#elif TEST==6

    char key[ KEYDATLEN ];
    memset( key, 0, KEYDATLEN );
    sprintf( key, "%d", 15635 );
    int x = bt->delete_rec( (UCHAR*)key, KEYDATLEN );
    printf( "del returned %d\n", x );

#elif TEST==7

    int i;
    UINT32 mag;
    UCHAR * b;
    int sz, bn;

    for ( i = 0; i < 300; i++ )
    {
        bn = fbn->alloc( 300 );
        printf( "%d ", bn );
        b = fbn->get_block( bn, sz, mag );
        memset( b, i+1, sz );
        fbn->unlock_block( mag, true );
    }
    printf( "\n" );

    


#endif

#if TEST!=7
    delete bt;
#else
    delete fbn;
#endif

    return 0;
}
