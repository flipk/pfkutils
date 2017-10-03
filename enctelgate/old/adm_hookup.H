
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
    ~Adm_Hookup_fd( void );
    bool read ( fd_mgr * );
    bool write( fd_mgr * );
    bool select_for_read( fd_mgr * );
    bool select_for_write( fd_mgr * );
    bool over_write_threshold( void );
    bool write_to_fd( char * buf, int len );
};
