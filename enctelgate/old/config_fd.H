
#include "fd_mgr.H"
#include "pipe_mgr.H"

class Config_fd : public fd_interface {
    Pipe_Mgr * pipe_mgr;
    struct sockaddr_in * last_sa;

    //xxx seq no validation not done
    static const int  seq_hist_size = 10;
    UINT32 seq_nos[ seq_hist_size ];
    int  seq_no_pos;

    // xxx must store last seq_hist_size replies too
    //     so that we can reply to repeated requests.

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
