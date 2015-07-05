/*
    This file is part of the "pfkutils" tools written by Phil Knaack
    (pknaack1@netscape.net).
    Copyright (C) 2008  Phillip F Knaack

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <fcntl.h>

#include "fd_mgr.h"
#include "m.h"
#include "ipipe_factories.h"
#include "ipipe_acceptor.h"
#include "ipipe_connector.h"
#include "ipipe_forwarder.h"
#include "ipipe_stats.h"
#include "ipipe_main.h"

#ifdef I2_MD5
#include "pk-md5.h"
#endif

const char * help_msg =
"i2 [-svnd] [-I[r|z]] [-i file] [-O] [-o file] [-m size] [-z[r|t]] port\n"
"i2 [-svnd] [-I[r|z]] [-i file] [-O] [-o file] [-m size] [-z[r|t]] host port\n"
"    -i: redirect fd 0 to input file\n"
"    -o: redirect fd 1 to output file\n"
"   -Ir: input is random data (implies -n)\n"
"   -Iz: input is zero data (implies -n)\n"
"    -O: discard output data\n"
"    -m: max output file size in bytes\n"
"    -n: do not read from stdin\n"
"    -s: display stats of transfer at end\n"
"    -v: verbose stats during transfer (0.5 second updates), implies -s\n"
"   -zr: uncompress any data received from network\n"
"   -zt: compress any data transmitted to network\n"
"i2 [-d] -f port host port [port host port...]\n"
"    -d: debug mode\n"
"    -D: log data transferred\n"
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

bool ipipe2_debug_log = false;

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
    bool inrand   = false;
    bool outdisc  = false;
    char * inp_file = NULL;
    char * out_file = NULL;
    char * zarg     = NULL;
    char * Iarg     = NULL;
    int ch;
    int flags;
    M_INT64 max_size = 0;
    struct timeval tick_tv, tv;
    ipipe_rollover * rollover = NULL;

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

    while (( ch = getopt( argc, argv, "svnfdDOz:i:I:o:m:" )) != -1 )
    {
        switch ( ch )
        {
        case 's':  stats    = true;        break;
        case 'v':  verbose  = true;        break;
        case 'n':  inp_file = (char*)"/dev/null"; break;
        case 'f':  tcpgate  = true;        break;
        case 'd':  debug    = true;        break;
        case 'D':  ipipe2_debug_log = true; break;
        case 'z':  zarg     = optarg;      break;
        case 'i':  inp_file = optarg;      break;
        case 'I':  Iarg     = optarg;      break;
        case 'o':  out_file = optarg;      break;
        case 'O':  outdisc  = true;        break;
        case 'm':
        {
            int result;
            if (optarg[0] == '0' && optarg[1] == 'x')
                result = m_parse_number( &max_size, optarg+2,
                                         strlen(optarg)-2, 16 );
            else
                result = m_parse_number( &max_size, optarg,
                                         strlen(optarg), 10 );
            if (result != 0)
            {
                fprintf( stderr,
                         "error: -m expects either decimal or hex number\n");
                return 1;
            }
            break;
        }
        default:   fprintf( stderr, "%s\n", help_msg ); exit( 1 );
        }
    }

    argc -= optind;
    argv += optind;

    if ( out_file && outdisc )
    {
        fprintf( stderr, 
                 "-O and -o are mutually exclusive\n" );
        exit( 1 );
    }

    if ( Iarg )
    {
        if ( inp_file )
        {
            fprintf( stderr,
                     "-I and -i are mutually exclusive\n" );
            exit( 1 );
        }
        inp_file = (char*)"/dev/zero";
        switch ( *Iarg )
        {
        case 'r':  inrand = true; break;
        case 'z':  /* nothing */  break;
        default:
            fprintf( stderr, "-I should only be followed by z or r\n" );
            exit( 1 );
        }
    }

    if ( tcpgate )
    {
        inp_file = (char*)"/dev/null";
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
        flags = O_RDONLY;
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

    flags = O_WRONLY | O_CREAT;
#ifdef O_LARGEFILE // cygwin
    flags |= O_LARGEFILE;
#endif
#ifdef O_BINARY // linux
    flags |= O_BINARY;
#endif

    if (max_size != 0)
    {
        if (!out_file)
        {
            fprintf(stderr, "error: -m only useful with -o\n");
            exit( 1 );
        }
        rollover = new ipipe_rollover( (int) max_size, out_file, flags );
        out_file = rollover->get_next_filename();
    }

    if ( out_file )
    {
        int cc;

        close( 1 );

        flags |= O_TRUNC;

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
                new ipipe_forwarder_factory( rcvunz, txz, rollover,
                                             outdisc, inrand );
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
                new ipipe_forwarder_factory( rcvunz, txz, rollover,
                                             outdisc, inrand );
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
