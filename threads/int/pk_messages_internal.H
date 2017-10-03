
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "stringhash.H"

enum { PK_MSG_IDHASH, PK_MSG_NAMEHASH, PK_MSG_NUMLISTS };

class PK_Message_Queue {
    LList <pk_msg_int,0>  messages;
    pthread_cond_t waiter;
public:
    LListLinks <PK_Message_Queue> links[PK_MSG_NUMLISTS];
    PK_Message_Queue( char *_name ) {
        waiter = NULL; name = strdup( _name );
    }
    ~PK_Message_Queue( void ) {
        if ( waiter )
        {
            fprintf( stderr, "deleting message queue while "
                     "thread still waiting on it!\n" );
            kill(0,6);
        }
        free( name );
    }
    int    qid;
    char * name;
    // return not-null if someone waiting
    pthread_cond_t enqueue( pk_msg_int * msg ) {
        pthread_cond_t w = waiter;
        waiter = NULL;
        messages.add( msg );
        return w;
    }
    pk_msg_int * dequeue( void ) { return messages.dequeue_head(); }
    void set_waiter( pthread_cond_t w ) {
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
    LListHash <PK_Message_Queue,int,
               PK_Message_Queue_hash_1,PK_MSG_IDHASH>      id_hash;
    LListHash <PK_Message_Queue,char *,
               PK_Message_Queue_hash_2,PK_MSG_NAMEHASH>  name_hash;
public:
    PK_Message_Queue_List( int _hashsize )
        : id_hash( _hashsize ), name_hash( _hashsize ) { }

    PK_Message_Queue * find( int qid ) {
        return id_hash.find( qid );
    }
    PK_Message_Queue * find( char * n ) {
        return name_hash.find( n );
    }
    void add( PK_Message_Queue * q ) {
        id_hash.add( q ); name_hash.add( q );
    }
    void remove( PK_Message_Queue * q ) {
        id_hash.remove( q ); name_hash.remove( q );
    }
};
