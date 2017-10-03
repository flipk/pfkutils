
// xxx whole file !

#include "tunnel.H"

Tunnel :: Tunnel( char * device,
                  char * my_address,  char * other_address )
{
}

//virtual
Tunnel :: ~Tunnel( void )
{
}

//virtual
fd_interface :: rw_response
Tunnel :: read ( fd_mgr * )
{
}

//virtual
fd_interface :: rw_response
Tunnel :: write( fd_mgr * )
{
}

//virtual
void
Tunnel :: select_rw ( fd_mgr *, bool * rd, bool * wr )
{
}

//virtual
bool
Tunnel :: over_write_threshold( void )
{
}

//virtual
bool
Tunnel :: write_to_fd( char * buf, int len )
{
}
