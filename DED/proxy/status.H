
#include "../threads/h/threads.H"

class statusThread : public Thread {
    void entry( void );
    static const int myprio = 4;
    static const int mystack = 16384;
    bool death_requested;
public:
    statusThread( void );
    ~statusThread( void );
    void die( void ) {
        if ( !death_requested )
        {
            death_requested = true;
            resume( tid );
        }
    }
};
