
#include "dll2/dll2.H"

enum { FDMAP_MAIN_LIST, FDMAP_DEL_LIST, FDMAP_NUM_LISTS };

struct FDMAP {
    LListLinks<FDMAP> links[FDMAP_NUM_LISTS];
    int fd;

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

class FDMAP_DATA : public FDMAP {

    FDMAP_DATA * other_fd;

    static const int BUFSIZE = 32768;
    char buf[BUFSIZE];
    int buf_in;
    int buf_out;
    int buf_free;
    int buf_used;

    bool   empty       ( void  ) { return buf_free == BUFSIZE; }
    bool   full        ( void  ) { return buf_free < 10; }
    char * write_pos   ( void  ) { return buf + buf_out; }
    char * read_pos    ( void  ) { return buf + buf_in; }

    void   record_write( int s )
        { buf_out += s; buf_out %= BUFSIZE; buf_free += s; }
    void   record_read ( int s )
        { buf_in  += s; buf_in  %= BUFSIZE; buf_free -= s; }

    int write_size( void ) {
        if ( buf_free == BUFSIZE )
            return 0;
        /* if out < in, then the space in between needs writing. */
        if ( buf_out < buf_in )
            return buf_in - buf_out;
        /* otherwise we want to write up to the end of the buffer */
        return BUFSIZE - buf_out;
    }

    int read_size( void ) {
        if ( buf_free == 0 )
            return 0;
        /* if out < in, then we can read only up to end of buffer. */
        if ( buf_out <= buf_in )
            return BUFSIZE - buf_in;
        /* otherwise the space in between is eligible */
        return buf_out - buf_in;
    }

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
