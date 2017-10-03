
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
    /*virtual*/ ~Config_fd( void );

    /*virtual*/ rw_response read ( fd_mgr * );
    /*virtual*/ rw_response write( fd_mgr * );
    /*virtual*/ void select_rw ( fd_mgr *, bool * rd, bool * wr );
    /*virtual*/ bool write_to_fd( char * buf, int len );

    // note over_write_threshold not implemented, using base class version
};
