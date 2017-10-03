
#include "tunnel.H"

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#ifdef linux
#include <linux/if.h>
#include <linux/if_tun.h>
#endif

Tunnel_fd :: Tunnel_fd( char * _my_address,
                        char * _other_address,
                        char * _netmask )
{
    encoder_fd    = NULL;
    device        = (char*)"/dev/net/tun";
    my_address    = _my_address;
    other_address = _other_address;
    netmask       = _netmask;

    devshort = strrchr( device, '/' );
    if ( !devshort )
    {
        fprintf( stderr, "improperly formed device path '%s'?\n", device );
        exit( 1 );
    }
    devshort++;

    fd = open( device, O_RDWR );
    if ( fd < 0 )
    {
        fprintf( stderr, "open tunnel device %s : %s\n",
                 device, strerror( errno ));
        exit( 1 );
    }

    printf( "opened tunnel device '%s'\n", device );
#ifdef linux
    struct ifreq ifr;
    memset(&ifr,0,sizeof(ifr));
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
    if (ioctl( fd, TUNSETIFF, &ifr ) < 0)
    {
        fprintf(stderr, "tun ioctl failed: %s\n", strerror(errno));
        exit( 1 );
    }
    printf("tunnel device %s activated\n", ifr.ifr_name);
    devshort = strdup(ifr.ifr_name);
#endif
}

//virtual
Tunnel_fd :: ~Tunnel_fd( void )
{
    unregister_encoder_fd();
    close( fd );
    printf( "closed tunnel device '%s'\n", device );
}

void
Tunnel_fd :: register_encoder_fd( Adm_Gate_fd * _encoder_fd )
{
    encoder_fd = _encoder_fd;

    char ifconfig_cmd[ 100 ];
    sprintf( ifconfig_cmd,
             "sudo ifconfig %s inet %s netmask %s",
             devshort, my_address, netmask );
    system( ifconfig_cmd );
}

void
Tunnel_fd :: unregister_encoder_fd( void )
{
    encoder_fd = NULL;
}

//virtual
fd_interface :: rw_response
Tunnel_fd :: read ( fd_mgr * )
{
    unsigned char buf[ 5000 ];
    int   len;

    len = ::read( fd, buf, sizeof(buf) );
    if ( len < 0 )
        return DEL;
    if ( len == 0 )
        return OK;

    encoder_fd->write_packet_to_fd( buf, len );

    return OK;
}

//virtual
void
Tunnel_fd :: select_rw ( fd_mgr *, bool * rd, bool * wr )
{
    *wr = false;
    *rd = true;
}

//virtual
bool
Tunnel_fd :: write_to_fd( char * buf, int len )
{
    if ( ::write( fd, buf, len ) != len )
        return false;
    return true;
}
