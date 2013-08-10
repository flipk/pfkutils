
#include "main.H"
#include "messages.H"

bbsMessage *
bbsMessage :: newMessage( int size )
{
    bbsMessage * ret;
    char * dat;
    dat = new char[ sizeof( bbsMessage ) + size ];
    ret = (bbsMessage *)dat;
    ret->refcount = 0;
    ret->_size = size;
    return ret;
}

void
Message :: deleteMessage( Message * m )
{
    char * dat = (char*) m;
    delete[] dat;
}

Messages :: Messages( int _max_messages )
{
    int i;
    max_messages = _max_messages;
    msgs = new Message*[max_messages];
    for ( i = 0; i < max_messages; i++ )
        msgs[i] = NULL;
    in = 0;
    iters = NULL;
    num_iters = 0;
    sem = global_th->seminit( 1 );
}

Messages :: ~Messages( void )
{
    for ( int i = 0; i < max_messages; i++ )
        if ( msgs[i] != NULL )
            delete msgs[i];

    MessageIterator *x, *nx;
    for ( nx = NULL, x = iters; x != NULL; x = nx )
    {
        nx = x->next;
        delete x;
    }

    global_th->semdelete( sem );
}

bool
Messages :: enqueue( Message * m )
{
    lock();

    int inp1 = (in + 1) % max_messages;
    if ( msgs[inp1] != NULL && msgs[inp1]->refcount > 0 )
    {
        unlock();
        return false;
    }

    if ( num_iters == 0 )
    {
        unlock();
        // nobody to send the message to, 
        // delete the message now.
        Message::deleteMessage( m );
        return true;
    }

    if ( msgs[in] != NULL )
        Message::deleteMessage( msgs[in] );

    msgs[in] = m;
    m->refcount = num_iters;
    m->_sender = global_th->tid();

    if ( ++in == max_messages )
        in = 0;

    unlock();
    return true;
}

MessageIterator *
Messages :: newIterator( void )
{
    MessageIterator * ret;

    ret = new MessageIterator;

    lock();
    ret->next = iters;
    ret->value = in;
    iters = ret;
    num_iters++;
    unlock();

    return ret;
}

void
Messages :: deleteIterator( MessageIterator * it )
{
    int i;
    lock();

    for ( i = it->value; i != in; i = ((i+1)%max_messages) )
        if ( msgs[i] != NULL )
            msgs[i]->refcount--;

    MessageIterator *x, *ox;
    for ( ox = NULL, x = iters; x != NULL; ox = x, x = x->next )
        if ( x == it )
            break;

    if ( x == it )
    {
        num_iters--;
        if ( ox == NULL )
            iters = x->next;
        else
            ox->next = x->next;
    }       
    unlock();
    delete it;
}

Message *
Messages :: nextMessage( MessageIterator * it )
{
    lock();

    int i = it->value;
    if ( i == in )
    {
        unlock();
        return NULL;
    }

    Message * ret = msgs[i];
    ret->refcount--;
    it->value = (i+1) % max_messages;

    unlock();
    return ret;
}

void
Messages :: status_function( void )
{
    int i;
    printf( "Messages queue: max=%d in=%d\n", max_messages, in );
    for ( i = 0; i < max_messages; i++ )
    {
        printf( "slot %2d : ", i );
        if ( msgs[i] != NULL )
            printf( "size=%4d sender=%2d refcount=%2d\n",
                    msgs[i]->_size,
                    msgs[i]->_sender,
                    msgs[i]->refcount );
        else
            printf( "NULL\n" );
    }
    printf( "iterators: num=%d\n", num_iters );
    MessageIterator * x = iters;
    for ( i = 0; i < num_iters; i++ )
    {
        printf( "slot %d : value=%2d\n", i, x->value );
        x = x->next;
    }
}
