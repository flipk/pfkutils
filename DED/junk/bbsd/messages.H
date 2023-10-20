
class bbsMessage {
    int refcount;
    int _size;
    int _sender;
    char _data[0];
public:
    static bbsMessage * newMessage( int size );
    static void deleteMessage( Message * m );
    char * data(void) { return _data; }
    int size(void) { return _size; }
    int sender(void) { return _sender; }
    friend class Messages;
};

class bbsMessageIterator {
    bbsMessageIterator * next;
    int value;
public:
    friend class bbsMessages;
};

class bbsMessages {
    int max_messages;
    int num_iters;
    int in;
    bbsMessage ** msgs;
    bbsMessageIterator * iters;
    ThreadSemaphore * sem;
    void   lock( void ) { global_th->take( sem, Threads::WAIT_FOREVER ); }
    void unlock( void ) { global_th->give( sem ); }
public:
    bbsMessages( int _max_messages );
    ~bbsMessages( void );

    // false means failure
    bool enqueue( bbsMessage * );
    bbsMessageIterator * newIterator( void );
    void deleteIterator( bbsMessageIterator * );
    bbsMessage * nextMessage( bbsMessageIterator * );
    void status_function( void );
};
