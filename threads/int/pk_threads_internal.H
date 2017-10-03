/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

#include "stringhash.H"

class PK_Thread_hash_1 {
public:
    static int hash_key( PK_Thread * item ) { 
        return hash_key( item->get_id() );
    }
    static int hash_key( pthread_t key ) {
        // this hash algorithm assumes that a pthread_t is a pointer
        // value that happens to be 4-byte aligned -- meaning that the
        // bottom two bits are zero and thus not useful in the hash.
        return (int) ((unsigned long) key >> 2);
    }
    static bool hash_key_compare( PK_Thread * item, pthread_t key ) {
        return ( item->get_id() == key );
    }
};

class PK_Thread_hash_2 {
public:
    static int hash_key( PK_Thread * item ) { 
        return hash_key( item->get_name() );
    }
    static int hash_key( char * key ) { return string_hash( key ); }
    static bool hash_key_compare( PK_Thread * item, char * key ) {
        return ( strcmp( item->get_name(), key ) == 0 );
    }
};

class PK_Thread_List {
    LList     <PK_Thread,        PK_THREAD_LIST    >  list;
    LListHash <PK_Thread,  pthread_t,
               PK_Thread_hash_1, PK_THREAD_IDHASH  >  idhash;
    LListHash <PK_Thread,  char *,
               PK_Thread_hash_2, PK_THREAD_NAMEHASH>  namehash;
    pthread_mutex_t  mutex;
    void   _lock( void ) { pthread_mutex_lock  ( &mutex ); }
    void _unlock( void ) { pthread_mutex_unlock( &mutex ); }
public:
    PK_Thread_List( void ) {
        pthread_mutex_init( &mutex, NULL );
    }
    ~PK_Thread_List( void ) {
        pthread_mutex_destroy( &mutex );
    }
    void add( PK_Thread * t ) {
        _lock();
        list.add( t );
        idhash.add( t );
        namehash.add( t ); 
        _unlock();
    }
    void remove( PK_Thread * t ) {
        _lock();
        list.remove( t );
        idhash.remove( t );
        namehash.remove( t );
        _unlock();
    }
    void unhash_name( PK_Thread * t ) {
        _lock(); namehash.remove( t );
    }
    void rehash_name( PK_Thread * t ) {
        namehash.add( t ); _unlock();
    }
    PK_Thread * find_id( pthread_t id ) {
        PK_Thread * ret;
        _lock();
        ret = idhash.find( id );
        _unlock();
        return ret;
    }
    PK_Thread * find_name( char * n ) {
        PK_Thread * ret;
        _lock();
        ret = namehash.find( n );
        _unlock();
        return ret;
    }
    int get_cnt( void ) {
        return list.get_cnt();
    }
    PK_Thread * get_head( void ) {
        return list.get_head();
    }
    // xxx : iteration over this list needs to be 
    //       atomic for the whole list, not just at each step
    PK_Thread * get_next( PK_Thread * t ) {
        PK_Thread * ret;
        _lock();
        ret = list.get_next( t );
        _unlock();
        return ret;
    }
};
