
#ifndef __TUNNEL_H__
#define __TUNNEL_H__

#include "fd_mgr.H"
#include "adm_gate.H"

class Tunnel_fd : public fd_interface {
    Adm_Gate_fd * encoder_fd;
    char * device;
    char * devshort;
    char * my_address;
    char * other_address;
    char * netmask;
public:
    Tunnel_fd( char * device,
               char * my_address,  char * other_address, char * netmask );
    /*virtual*/ ~Tunnel_fd( void );
    /*virtual*/ rw_response read ( fd_mgr * );
    /*virtual*/ void select_rw ( fd_mgr *, bool * rd, bool * wr );
    /*virtual*/ bool write_to_fd( char * buf, int len );

    /*       */ void register_encoder_fd( Adm_Gate_fd * encoder_fd );
    /*       */ void unregister_encoder_fd( void );

    // over_write_threshold and write not implemented
};

#endif /* __TUNNEL_H__ */
