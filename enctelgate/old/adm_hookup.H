
#include "fd_mgr.H"

class Adm_Hookup_Factory_iface {
public:
    virtual ~Adm_Hookup_Factory_iface( void ) { /* nothing */ }
    virtual void new_gateway( int fd_ear, int fd_outfd,
                              fd_mgr * fdmgr ) = 0;
};

class Adm_Hookup_fd : public fd_interface {
    Adm_Hookup_Factory_iface * factory;
    struct sockaddr_in * dest_sa;
public:
    Adm_Hookup_fd( Adm_Hookup_Factory_iface *,
               short port, char * dest_host, short dest_port );
    /*virtual*/ ~Adm_Hookup_fd( void );

    /*virtual*/ rw_response read ( fd_mgr * );
    /*virtual*/ rw_response write( fd_mgr * );
    /*virtual*/ void select_rw ( fd_mgr *, bool * rd, bool * wr );

    // note over_write_threshold and write_to_fd not implemented
};
