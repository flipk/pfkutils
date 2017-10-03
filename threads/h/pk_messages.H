
#ifndef __MESSAGES_H__
#define __MESSAGES_H__

#include "dll2.H"
#include "pk_msg_int.H"
#include <pthread.h>

class PK_Message_Queue_List;

class PK_Message_Manager {
    pthread_mutex_t   mutex;
    PK_Message_Queue_List * queues;
    void   _lock( void ) { pthread_mutex_lock  ( &mutex ); }
    void _unlock( void ) { pthread_mutex_unlock( &mutex ); }
public:
    PK_Message_Manager( void );
    ~PK_Message_Manager( void );
//
    int          create( char * name );  // returns qid
    int          lookup( char * name );  // returns qid or -1
    bool         destroy( int qid ); // returns false if qid does not exist
    bool         send( int qid, pk_msg_int * msg ); // returns false if error
    pk_msg_int * recv( int num_qids, int * qids,
                       int * retqid, int ticks );
};

extern PK_Message_Manager * PK_Messages_global;

#endif /* __MESSAGES_H__ */
