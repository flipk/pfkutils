
#include "pk_messages.H"
#include "pk_messages_internal.H"

#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>

PK_Message_Manager * PK_Messages_global;

PK_Message_Manager :: PK_Message_Manager( int _hashsize )
{
    if ( PK_Messages_global )
    {
        fprintf( stderr, "PK_Message_Manager already exists?!\n" );
        kill(0,6);
    }

    queues = new PK_Message_Queue_List( _hashsize );
    pthread_mutex_init( &mutex, NULL );
    PK_Messages_global = this;
}

PK_Message_Manager :: ~PK_Message_Manager( void )
{
    pthread_mutex_destroy( &mutex );
    delete queues;
    PK_Messages_global = NULL;
}

int
PK_Message_Manager :: create( char * name )
{
    int qid;
    PK_Message_Queue * nmq, * mq;

    nmq = new PK_Message_Queue( name );

    _lock();
    do {
        qid = random() & 0x7fffffff;
        if ( qid == 0 || qid == -1 )
            continue;
        mq = queues->find( qid );
    } while ( mq != NULL );
    nmq->qid = qid;
    queues->add( nmq );
    _unlock();

    return qid;
}

int  // returns qid or -1
PK_Message_Manager :: lookup( char * name )
{
    PK_Message_Queue * mq;

    _lock();
    mq = queues->find( name );
    _unlock();

    if ( mq )
        return mq->qid;

    return -1;
}

bool  // returns false if qid does not exist
PK_Message_Manager :: destroy( int qid )
{
    PK_Message_Queue * mq;
    bool ret = true;

    _lock();
    mq = queues->find( qid );
    if ( mq )
        queues->remove( mq );
    else
        ret = false;
    _unlock();
    if ( mq )
    {
        pk_msg_int * m; 
        int count = 0;
        while ( m = mq->dequeue() )
        {
            // note the caveat : if you're going to destroy a 
            // msgq that isn't empty, you can only use 'new'
            // to allocate messages for it.
            delete m;
            count ++;
        }
        if ( count != 0 )
        {
            fprintf( stderr, "while destroying mq '%s': "
                     "deleted %d stale messages\n",
                     mq->name, count );
        }
        delete mq;
    }
    return ret;
}

bool // return false if error
PK_Message_Manager :: send( int qid, pk_msg_int * msg )
{
    PK_Message_Queue * mq;
    bool ret = true;
    pthread_cond_t waiter = NULL;

    _lock();
    mq = queues->find( qid );
    if ( mq )
        waiter = mq->enqueue( msg );
    else
        ret = false;
    _unlock();

    if ( waiter )
        pthread_cond_signal( &waiter );

    return ret;
}

pk_msg_int * // return NULL if timeout
PK_Message_Manager :: recv( int num_qids, int * qids,
                            int * retqidind, int ticks )
{
    pthread_cond_t       cond = NULL;
    PK_Message_Queue * mqs[ num_qids ];
    pk_msg_int * ret = NULL;
    int i;

    _lock();
    for ( i = 0; i < num_qids; i++ )
    {
        mqs[i] = queues->find( qids[i] );
        if ( !mqs[i] )
        {
            printf( "msg q %d not found!\n", qids[i] );
            kill(0,6);
        }
    }

    while ( 1 )
    {
        for ( i = 0; i < num_qids; i++ )
        {
            ret = mqs[i]->dequeue();
            if ( ret )
            {
                if ( retqidind )
                    *retqidind = i;
                break;
            }
        }

        if ( ret || ticks == 0 )
            // either one has a message, or none does and the user
            // specified no timeout.
            break;

        // none of the specified mqs has a msg, so pend.
        if ( cond == NULL )
        {
            pthread_cond_init( &cond, NULL );
            for ( i = 0; i < num_qids; i++ )
                mqs[i]->set_waiter( cond );
        }

        if ( ticks == -1 )
            pthread_cond_wait( &cond, &mutex );
        else
        {
            struct timespec abstime;

            clock_gettime( CLOCK_REALTIME, &abstime );

            abstime.tv_sec  +=  ticks / 10;
            abstime.tv_nsec += (ticks % 10) * 100000000;

            if ( abstime.tv_nsec > 1000000000 )
            {
                abstime.tv_nsec -= 1000000000;
                abstime.tv_sec ++;
            }

            if ( pthread_cond_timedwait( &cond, &mutex,
                                         &abstime ) == ETIMEDOUT )
                break;
         }
    }
    if ( cond )
        for ( i = 0; i < num_qids; i++ )
            mqs[i]->clear_waiter();
    _unlock();

    if ( cond )
        pthread_cond_destroy( &cond );

    return ret;
}

//

PK_Message_Queue_List :: ~PK_Message_Queue_List( void )
{
    PK_Message_Queue * mq;
    while ( mq = list.dequeue_head() )
    {
        fprintf( stderr, "while deleting message queue manager: "
                 "deleting mq '%s'\n", mq->name );
        id_hash.remove( mq );
        name_hash.remove( mq );
        delete mq;
    }
}

//

PK_Message_Queue :: ~PK_Message_Queue( void )
{
    pk_msg_int * m;
    int count = 0;
    while ( m = messages.dequeue_head() )
    {
        delete m;
        count++;
    }
    if ( count > 0 )
    {
        fprintf( stderr, "while deleting mq '%s': "
                 "deleting %d stale messages\n",
                  name, count );
    }
    if ( waiter )
    {
        fprintf( stderr, "deleting message queue while "
                 "thread still waiting on it!\n" );
        kill(0,6);
    }
    free( name );
}
