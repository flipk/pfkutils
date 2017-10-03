
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <fcntl.h>

#include "fd_mgr.H"
#include "ipipe_factories.H"
#include "ipipe_acceptor.H"
#include "ipipe_connector.H"
#include "ipipe_forwarder.H"
#include "ipipe_stats.H"
#include "ipipe_main.H"

#ifdef I2_MD5
#include "pk-md5.h"
#endif

const char * help_msg =
"i2 [-svnd] [-i infile] [-o outfile] [-z[r|t]] port      (passive)\n"
"i2 [-svnd] [-i infile] [-o outfile] [-z[r|t]] host port (active)\n"
"i2 [-d] -f port host port [port host port...]\n"
"    -i: redirect fd 0 to input file\n"
"    -o: redirect fd 1 to output file\n"
"    -n: do not read from stdin\n"
"    -s: display stats of transfer at end\n"
"    -v: verbose stats during transfer (0.5 second updates), implies -s\n"
"    -d: debug mode\n"
"   -zr: uncompress any data received from network\n"
"   -zt: compress any data transmitted to network\n"
"    -f: forward local port to remote host/port\n"
;
/*
"    -p: use ping-ack method to lower network impact (must use on both ends)\n"
"    -e: echo a hex dump of the transfer in each dir to stderr\n"
"    -d: delete 0x00's and 0x0D's from the input stream (for GDBP)\n"
*/

static void
hostname_to_ipaddr( char * host, void * addr )
{
    if ( ! (inet_aton( host, (in_addr*) addr )))
    {
        struct hostent * he;
        if (( he = gethostbyname( host )) == NULL )
        {
            fprintf( stderr, "host lookup of %s: %s\n",
                     host, strerror( errno ));
            exit( 1 );
        }
        memcpy( addr, he->h_addr, he->h_length );
    }
}

#ifdef I2_MD5
static MD5Context * md5;
static bool md5_read;
static bool md5_writ;

void
i2_add_md5_recv( char * buf, int len )
{
    if ( md5_writ )
        return;
    md5_read = true;
    MD5Update( md5, (unsigned char *)buf, len );
}

void
i2_add_md5_writ( char * buf, int len )
{
    if ( md5_read )
        return;
    md5_writ = true;
    MD5Update( md5, (unsigned char *)buf, len );
}
#else
void
i2_add_md5_recv( char * buf, int len )
{
    // nop
}

void
i2_add_md5_writ( char * buf, int len )
{
    // nop
}
#endif

extern "C"
int
i2_main( int argc,  char ** argv )
{
    bool stats    = false;
    bool verbose  = false;
    bool rcvunz   = false;
    bool txz      = false;
    bool tcpgate  = false;
    bool debug    = false;
    char * inp_file = NULL;
    char * out_file = NULL;
    char * zarg     = NULL;
    int ch;
    struct timeval tick_tv, tv;

    extern int optind;
    extern char * optarg;

#ifdef I2_MD5
    md5 = new MD5Context;
    MD5Init( md5 );
    md5_read = md5_writ = false;
#endif

    signal( SIGTTIN, SIG_IGN );

    if ( argc == 1 )
    {
        fprintf( stderr, "%s\n", help_msg );
        return 1;
    }

    while (( ch = getopt( argc, argv, "svnfdz:i:o:" )) != -1 )
    {
        switch ( ch )
        {
        case 's':  stats    = true;        break;
        case 'v':  verbose  = true;        break;
        case 'n':  inp_file = "/dev/null"; break;
        case 'f':  tcpgate  = true;        break;
        case 'd':  debug    = true;        break;
        case 'z':  zarg     = optarg;      break;
        case 'i':  inp_file = optarg;      break;
        case 'o':  out_file = optarg;      break;
        default:   fprintf( stderr, "%s\n", help_msg ); exit( 1 );
        }
    }

    argc -= optind;
    argv += optind;

    if ( tcpgate )
    {
        inp_file = "/dev/null";
        if ( stats )
        {
            fprintf( stderr,
                     "-s not useful with -f since tcpgate does not exit\n" );
            exit( 1 );
        }
        if ( zarg )
        {
            fprintf( stderr,
                     "cannot use -zX with -f\n" );
            exit( 1 );
        }
    }

    if ( zarg )
        while ( *zarg )
            switch ( *zarg++ )
            {
            case 'r':   rcvunz = true;  break;
            case 't':   txz    = true;  break;
            default:
                fprintf( stderr, "-z must be followed only by r or t\n" );
                exit( 1 );
            }

    if ( inp_file )
    {
        int cc;
        int flags = O_RDONLY;
        close( 0 );

#ifdef O_BINARY  // cygwin
        flags |= O_BINARY;
#endif
#ifdef O_LARGEFILE  // linux
        flags |= O_LARGEFILE;
#endif
        cc = open( inp_file, flags );
        if ( cc != 0 )
        {
            fprintf( stderr,
                     "unable to open '%s' as stdin (%d,%d:%s)\n",
                     inp_file, cc, errno, strerror( errno ));
            exit( 1 );
        }
    }

    if ( out_file )
    {
        int cc;
        int flags = O_WRONLY | O_CREAT;

        close( 1 );

#ifdef O_LARGEFILE // cygwin
        flags |= O_LARGEFILE;
#endif
#ifdef O_BINARY // linux
        flags |= O_BINARY;
#endif

        cc = open( out_file, flags, 0644 );
        if ( cc != 1 )
        {
            fprintf( stderr,
                     "unable to open '%s' as stdout (%d,%d:%s)\n",
                     inp_file, cc, errno, strerror( errno ));
            exit( 1 );
        }
    }

    fd_mgr         mgr( debug, 0 );
    fd_interface * fdi = NULL;

    stats_init( (!rcvunz && !txz),  // short form if no libz involved
                verbose, stats, tcpgate,
                &tick_tv );

    if ( tcpgate )
    {
        if (( argc % 3 ) != 0 )
        {
            fprintf( stderr, "incorrect number of args for -f "
                     "(not multiple of 3)\n" );
            exit( 1 );
        }
        while ( argc >= 3 )
        {
            struct sockaddr_in sa;

            int listen_port = atoi( argv[0] );
            char * host = argv[1];
            short  port = atoi( argv[2] );

            sa.sin_family = AF_INET;
            sa.sin_port = htons( port );

            hostname_to_ipaddr( host, &sa.sin_addr );

            ipipe_new_connection * inc = new ipipe_proxy_factory( &sa );
            fdi = new ipipe_acceptor( listen_port, inc );

            mgr.register_fd( fdi );

            argc -= 3;
            argv += 3;
        }
    }
    else
    {
        if ( argc == 1 )
        {
            int port = atoi( argv[0] );
            ipipe_new_connection * inc =
                new ipipe_forwarder_factory( rcvunz, txz );
            fdi = new ipipe_acceptor( port, inc );
            mgr.register_fd( fdi );
        }
        else if ( argc == 2 )
        {
            struct sockaddr_in sa;
            char * host = argv[0];
            short  port = atoi( argv[1] );

            sa.sin_family = AF_INET;
            sa.sin_port = htons( port );

            hostname_to_ipaddr( host, &sa.sin_addr );

            ipipe_new_connection * inc =
                new ipipe_forwarder_factory( rcvunz, txz );
            fdi = new ipipe_connector( &sa, inc );
            mgr.register_fd( fdi );
        }
        else
        {
            fprintf( stderr, "%s\n", help_msg );
            exit( 1 );
        }
    }

    while (1)
    {
        tv = tick_tv;
        if (mgr.loop(&tv))
            break;
        stats_tick();
    }
    stats_done();

#ifdef I2_MD5
    MD5_DIGEST  digest;
    int i;

    MD5Final( &digest, md5 );

    fprintf( stderr, "md5 digest: " );
    for ( i = 0; i < (int)sizeof(digest.digest); i++ )
        fprintf( stderr, "%02x", digest.digest[i] );
    fprintf( stderr, "\n" );
#endif

    return 0;
}
