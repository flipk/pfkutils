
#include "../threads/h/threads.H"

#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

class connThread : public Thread {
    void entry( void );
    static const int myprio = 10;
    static const int mystack = 16384;
    static const int myport = 1080;
    int fd;
    struct sockaddr_in sa;
    bool death_requested;
public:
    connThread( void );
    ~connThread( void );
    void die( void ) { death_requested = true; resume( tid ); }
};
