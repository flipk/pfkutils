
#include <sys/socket.h>
#include <netinet/in.h>

#include "fd_mgr.H"

#include "ipipe_factories.H"
#include "ipipe_acceptor.H"
#include "ipipe_connector.H"
#include "ipipe_forwarder.H"
#include "ipipe_tick.H"

const char * help_msg =
"ipipe [-psve] [-i infile] [-o outfile] [-z[r|t]] port      (passive)\n"
"ipipe [-psve] [-i infile] [-o outfile] [-z[r|t]] host port (active)\n"
"ipipe -f port host port [port host port...]\n"
"    -i: redirect fd 0 to input file\n"
"    -o: redirect fd 1 to output file\n"
"    -s: display stats of transfer at end\n"
"    -v: display stats live, during transfer (1 second updates), implies -s\n"
"    -e: echo a hex dump of the transfer in each dir to stderr\n"
"    -d: delete 0x00's and 0x0D's from the input stream (GDBP)\n"
"    -n: do not read from stdin\n"
"    -p: use ping-ack method to lower network impact (must use on both ends)\n"
"   -zr: uncompress any data received from network\n"
"   -zt: compress any data transmitted to network\n"
"    -f: forward local port to remote host/port\n"
;

//static
void
tick_func( void * )
{
}

int
main( int argc,  char ** argv )
{
    fd_mgr         mgr( true, 0 );
    fd_interface * fdi;

//    tick_fd * tick_fd_ptr = new tick_fd( 10 );
//    mgr.register_fd( tick_fd_ptr );
//    tick_fd_ptr->register_tick( tick_func, 0 );

#if 0
    /* tcpgate mode */
    struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_port = htons( 23 );
    sa.sin_addr.s_addr = htonl( 0x7f000001 );
    ipipe_new_connection * inc = new ipipe_proxy_factory( &sa );
    fdi = new ipipe_acceptor( 2500, inc );
#endif

#if 1
    /* ipipe passive mode */
    ipipe_new_connection * inc = new ipipe_forwarder_factory( false, false );
    fdi = new ipipe_acceptor( 2500, inc );
#endif

#if 0
    /* ipipe active mode */
    struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_port = htons( 2500 );
    sa.sin_addr.s_addr = htonl( 0x7f000001 );
#if 0
    /* compress */
    ipipe_new_connection * inc = new ipipe_forwarder_factory( false, true  );
#elif 1
    /* decompress */
    ipipe_new_connection * inc = new ipipe_forwarder_factory( true,  false );
#else
    /* raw */
    ipipe_new_connection * inc = new ipipe_forwarder_factory( false, false );
#endif
    fdi = new ipipe_connector( &sa, inc );
#endif

    mgr.register_fd( fdi );
    mgr.loop();

    return 0;
}
