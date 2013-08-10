
#include <sys/types.h>

class inputbuf {
    int fd;
    int size;
    int (*readfunc)(int fd, char * buf, int size);
    int curdat;
    char data[0];
    int readmore( void );
public:
    void * operator new( size_t dummysize, int size );
    void operator delete( void * );
    inputbuf( int _fd, int (*readfunc)(int,char*,int) );

    int data_left( void ) { return curdat; }
    char * getline( void );
    int read( char * buf, int len );
};
