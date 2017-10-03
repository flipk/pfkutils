
#include <string.h>

class ThreadSemaphore;
struct semWaiter;

class ThreadSemaphores {
    ThreadSemaphore * first;
    ThreadSemaphore * last;
public:
    ThreadSemaphores( void );
    ~ThreadSemaphores( void );
    ThreadSemaphore * seminit( char * name, int v );
    void semdelete( ThreadSemaphore * );
    bool take( ThreadSemaphore *s, int timeout );
    void give( ThreadSemaphore *s );
    void printsems( FILE * );
};

class ThreadSemaphore {
    semWaiter * waitlist;
    semWaiter * waitlist_end;
    ThreadSemaphore * next;
    ThreadSemaphore * prev;
    char * name;
    int v;
    void enqueue( semWaiter * );
    void unenqueue( semWaiter * );
    semWaiter * dequeue( void );
    // constructor and destructor are private.
    // use ThreadSemaphores object within Threads object
    // to create and destroy these.
    ThreadSemaphore( char * _name, int _v ) {
        v = _v;
        next = NULL;
        prev = NULL;
        waitlist = NULL;
        waitlist_end = NULL;
        int name_l = strlen( _name ) + 1;
        name = new char[ name_l ];
        memcpy( name, _name, name_l );
    }
    ~ThreadSemaphore( void ) {
        delete[] name;
    }
public:
    void give( void ) { th->sems->give( this ); }
    bool take( int timeout ) {
        return th->sems->take( this, timeout );
    }
    bool take( void ) {
        return th->sems->take( this,
                               Threads::WAIT_FOREVER );
    }
    char * get_name( void ) {
        return name;
    }
    friend class ThreadSemaphores;
};
