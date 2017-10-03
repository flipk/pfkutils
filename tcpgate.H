
#include "dll2/dll2.H"

extern "C" int inet_aton( register const char *cp, struct in_addr *addr );
enum { FDMAP_MAIN_LIST, FDMAP_DEL_LIST, FDMAP_NUM_LISTS };

struct FDMAP_DATA;
struct FDMAP {
    LListLinks<FDMAP> links[FDMAP_NUM_LISTS];
    int fd;
    FDMAP_DATA * other_fd; // used by FDMAP_DATA only

    FDMAP( void ) { other_fd = NULL; }
    virtual bool sel_r( void ) = 0;
    virtual bool sel_w( void ) = 0;
    virtual void handle_select_r( void ) = 0;
    virtual void handle_select_w( void ) = 0;
};

typedef LList<FDMAP,FDMAP_MAIN_LIST> FDMAP_LIST;
typedef LList<FDMAP,FDMAP_DEL_LIST > FDMAP_DELETE_LIST;

class FDMAP_LISTEN : public FDMAP {
    unsigned short my_port;
    unsigned short remote_port;
    struct in_addr remote_host;

public:
    FDMAP_LISTEN( int _fd, unsigned short _my_port,
                  unsigned short _remote_port,
                  struct in_addr _remote_host )
        {
            fd = _fd;
            my_port = _my_port;
            remote_port = _remote_port;
            remote_host = _remote_host;
        }

    /*virtual*/ bool sel_r( void ) { return true; }
    /*virtual*/ bool sel_w( void ) { return false; }
    /*virtual*/ void handle_select_r( void );
    /*virtual*/ void handle_select_w( void ) { /* noop */ }
};

#include "circular_buffer.H"

class FDMAP_DATA : public FDMAP {


    static const int BUFSIZE = 16384;

    circular_buffer buf;

    bool can_read;
    bool want_write;
    bool waitfor_connect;
    bool want_close;

    void closeit( void ) {
        close( fd );
        close( other_fd->fd );
        fd = -1;
        other_fd->fd = -1;
    }

    void _handle_select_r( void );

public:
    FDMAP_DATA( int, bool connecting );
    /*virtual*/ bool sel_r( void ) { return can_read; }
    /*virtual*/ bool sel_w( void ) { return want_write; }
    /*virtual*/ void handle_select_r( void );
    /*virtual*/ void handle_select_w( void );
    void set_other( FDMAP_DATA * other ) { other_fd = other; }
};
