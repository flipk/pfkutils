
#include <string.h>

class ThreadSemaphore;
struct semWaiter;

typedef LList<ThreadSemaphore,0> ThreadSemaphoreList;

class ThreadSemaphores {
    ThreadSemaphoreList  sem_list;
public:
    ThreadSemaphores( void );
    ~ThreadSemaphores( void );
    ThreadSemaphore * seminit( char * name, int v );
    void semdelete( ThreadSemaphore * );
    bool take( ThreadSemaphore *s, int timeout );
    void give( ThreadSemaphore *s );
    void printsems( FILE * );
};

typedef LList<semWaiter,0> semWaiterList;

class ThreadSemaphore {
public:
    LListLinks<ThreadSemaphore> links[1];
private:
    semWaiterList  waitlist;
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
