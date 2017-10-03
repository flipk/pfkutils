
#include "fd_mgr.H"
#include "pipe_mgr.H"

class Config_fd : public fd_interface {
    Pipe_Mgr * pipe_mgr;
    struct sockaddr_in * last_sa;

    UINT32 last_seq_no;
    char * last_response_pkt;
    int    last_response_len;

    // send response from the last* buffer
    void   send_response( void );
    // call before send_response if there is a new one
    void   fill_response( char * buf, int len );

    // return true if the seqno is new
    bool   check_last_seqno( UINT32 seq );

public:
    Config_fd( Pipe_Mgr * _pipe_mgr, short udp_port_no );
    ~Config_fd( void );

    // true means it was ok, false means delete this object.
    bool read ( fd_mgr * );
    bool write( fd_mgr * );

    // true means select, false means dont
    bool select_for_read( fd_mgr * );
    bool select_for_write( fd_mgr * );

    // true means low on write buffer space, should probably
    // try to quench your source if you need to write to this fd.
    bool over_write_threshold( void );

    // return false if it was not ok
    bool write_to_fd( char * buf, int len );
};
