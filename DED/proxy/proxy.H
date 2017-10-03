
#include "../threads/h/threads.H"
#include "connlog.H"
#include "linked_list.H"

class inputbuf;
class mime_headers;

class proxyThread : public Thread {
    static const int myprio = 9;
    static const int mystacksize = 65536;

    static const int mybufsize = 8192;
    static const int maxurlsize = 2000;

    void entry( void );
    static int inbufreadfunc( int fd, char * buf, int len );
    static int outbufreadfunc( int fd, char * buf, int len );

    int mywrite( int fd, char * buf, int len );

    // debug data
    connLog * log;

    // connection data
    int fd;
    inputbuf * inbuf;
    inputbuf * outbuf;
    int outfd;

    // connection functions
    int get_request( void );
    int get_mimehdrs( inputbuf *, mime_headers * );
    int parse_req( void );
    int open_proxy_conn( void );
    int send_request( void );
    int parse_outmime( void );
    int pass_data( void );
    void req_cleanup( void );
    void send_mime( int myfd, mime_headers * );

    // request data
    char * inrequest;
    char * outrequest;
    mime_headers *  inmh;
    mime_headers * outmh;
    int in_content_length;
    int out_content_length;
    bool keepalive;
    char * remotehost;
    int remoteport;
    bool death_requested;

public:
    proxyThread( int fd );
    ~proxyThread( void );

    proxyThread * next;
    proxyThread * prev;

    char * my_url;
    int get_id( void ) { return tid; }
    int get_lognumber( void ) { return log->get_lognumber(); }
    void die( void ) { death_requested = true; resume( tid ); }
};

extern linked_list<proxyThread> * proxys;
