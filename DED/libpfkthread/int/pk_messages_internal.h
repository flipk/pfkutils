/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */
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

#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "stringhash.h"

/** \cond INTERNAL */

enum { PK_MSG_LIST, PK_MSG_IDHASH, PK_MSG_NAMEHASH, PK_MSG_NUMLISTS };

class PK_Message_Queue {
    LList <pk_msg_int,0>  messages;
    pthread_cond_t * waiter;
public:
    LListLinks <PK_Message_Queue> links[PK_MSG_NUMLISTS];
    PK_Message_Queue( char *_name ) {
        waiter = NULL; name = strdup( _name );
    }
    ~PK_Message_Queue( void );
    int    qid;
    char * name;
    // return not-null if someone waiting
    pthread_cond_t * enqueue( pk_msg_int * msg ) {
        pthread_cond_t * w = waiter;
        waiter = NULL;
        messages.add( msg );
        return w;
    }
    pk_msg_int * dequeue( void ) { return messages.dequeue_head(); }
    void set_waiter( pthread_cond_t * w ) {
        if ( waiter != NULL )
        {
            fprintf( stderr, "more than one thread waiting "
                     "on a message queue!\n" );
            kill(0,6);
        }
        waiter = w;
    }
    void clear_waiter( void ) { waiter = NULL; }
};

class PK_Message_Queue_hash_1 {
public:
    static int hash_key( PK_Message_Queue * item ) { return item->qid; }
    static int hash_key( int key ) { return key; }
    static bool hash_key_compare( PK_Message_Queue * item, int key )
    { return ( item->qid == key ); }
};

class PK_Message_Queue_hash_2 {
public:
    static int hash_key( PK_Message_Queue * item ) {
        return hash_key( item->name );
    }
    static int hash_key( char * key ) { return string_hash( key ); }
    static bool hash_key_compare( PK_Message_Queue * item, char * key ) {
        return ( strcmp( item->name, key ) == 0 );
    }
};

class PK_Message_Queue_List {
    LList     <PK_Message_Queue,       PK_MSG_LIST>           list;
    LListHash <PK_Message_Queue,int,
               PK_Message_Queue_hash_1,PK_MSG_IDHASH>      id_hash;
    LListHash <PK_Message_Queue,char *,
               PK_Message_Queue_hash_2,PK_MSG_NAMEHASH>  name_hash;
public:
    PK_Message_Queue_List( void ) { }
    ~PK_Message_Queue_List( void );

    PK_Message_Queue * find( int qid ) {
        return id_hash.find( qid );
    }
    PK_Message_Queue * find( char * n ) {
        return name_hash.find( n );
    }
    void add( PK_Message_Queue * q ) {
        list.add( q ); id_hash.add( q ); name_hash.add( q );
    }
    void remove( PK_Message_Queue * q ) {
        list.remove( q ); id_hash.remove( q ); name_hash.remove( q );
    }
};
/** \endcond */
