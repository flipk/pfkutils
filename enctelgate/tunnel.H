
#include "fd_mgr.H"

class Tunnel : public fd_interface {
public:
    Tunnel( char * device,   char * my_address,  char * other_address );
    /*virtual*/ ~Tunnel( void );
    /*virtual*/ rw_response read ( fd_mgr * );
    /*virtual*/ rw_response write( fd_mgr * );
    /*virtual*/ void select_rw ( fd_mgr *, bool * rd, bool * wr );
    /*virtual*/ bool over_write_threshold( void );
    /*virtual*/ bool write_to_fd( char * buf, int len );
};
