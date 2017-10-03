
#include "config_messages.H"
#include "config_port.H"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>


// return false if failure
bool
fetch_host( char * host_str, UINT32 * host_ip )
{
    struct sockaddr_in sa;
    struct hostent * he;

    if ( ! inet_aton( host_str, &sa.sin_addr ))
    {
        if (( he = gethostbyname( host_str )) == NULL )
            return false;
        bcopy( (char*) he->h_addr, (char*) &sa.sin_addr,
               he->h_length );
    }

    *host_ip = ntohl( sa.sin_addr.s_addr );

    return true;
}


int
main( int argc, char ** argv )
{

    if ( argc == 1 || argc > 5 )
    {
    usage:
        fprintf( stderr,
                 "etg_config -al port remote_host remote_port \n"
                 "etg_config -dl port \n"
                 "etg_config -Dl \n"
                 "etg_config -Dp \n" );
        return 1;
    }

    bool               done = false;
    int                fd, port;
    UINT32             seq = arc4random();
    pk_tcp_msg       * msg;
    struct sockaddr_in  sa;

    sa.sin_family = AF_INET;
    sa.sin_port = htons( CONFIG_UDP_PORT_NO );
    sa.sin_addr.s_addr = INADDR_ANY;

    fd = socket( AF_INET, SOCK_DGRAM, 0 );
    if ( fd < 0 )
    {
        fprintf( stderr, "socket: %s\n", strerror( errno ));
        return 1;
    }

    if ( strcmp( argv[1], "-al" ) == 0 )
    {
        if ( argc != 5 )
            goto usage;

        port = atoi(argv[2]);
        if ( port < 1024 || port > 32767 )
        {
            fprintf( stderr, "bogus port number '%d'\n", port );
            return 1;
        }

        UINT32 rhost;
        if ( fetch_host( argv[3], &rhost ) == false )
        {
            fprintf( stderr, "unknown host %s\n", argv[3] );
            return 1;
        }

        int rport = atoi(argv[4]);
        if ( port < 20 || port > 32767 )
        {
            fprintf( stderr, "bogus port number '%d'\n", port );
            return 1;
        }

        msg = new Config_Add_Listen_Port( seq, port, rhost, rport );
    }
    else if ( strcmp( argv[1], "-dl" ) == 0 )
    {
        if ( argc != 3 )
            goto usage;
        port = atoi(argv[2]);
        if ( port < 1024 || port > 32767 )
        {
            fprintf( stderr, "bogus port number '%d'\n", port );
            return 1;
        }

        msg = new Config_Delete_Listen_Port( seq, port );
    }
    else if ( strcmp( argv[1], "-Dl" ) == 0 )
    {
        if ( argc != 2 )
            goto usage;

        msg = new Config_Display_Listen_Ports( seq );
        done = true; // no response
    }
    else if ( strcmp( argv[1], "-Dp" ) == 0 )
    {
        if ( argc != 2 )
            goto usage;

        msg = new Config_Display_Proxy_Ports( seq );
        done = true; // no response
    }
    else 
        goto usage;

    int retries = 0;
    while ( 1 )
    {
        msg->set_checksum();
        sendto( fd, msg->get_ptr(), msg->get_len(), 0,
                (struct sockaddr *)&sa, sizeof( sa ));

        if ( done )
        {
            delete msg;
            return 0;
        }

        fd_set rfds;
        struct timeval tv = { 0, 100000 };

        FD_ZERO( &rfds );
        FD_SET( fd, &rfds );

        if ( select( fd+1, &rfds, NULL, NULL, &tv ) == 1 )
            break;

        printf( "timeout waiting for response!\n" );
        if ( ++retries >= 3 )
        {
            printf( "giving up on response\n" );
            return 1;
        }
    }
    delete msg;


    MaxPkMsgType                          gen;
    union {
        char                            * buf;
        MaxPkMsgType                    * gen;
        Config_Add_Listen_Port_Reply    * calpr;
        Config_Delete_Listen_Port_Reply * cdlpr;
    } u;
    u.gen = &gen;

    recvfrom( fd, u.buf, sizeof(gen), 0, NULL, NULL );

    if ( ! u.gen->verif_magic() )
    {
        printf( "bogus magic\n" );
        return 1;
    }
    if ( ! u.gen->verif_checksum() )
    {
        printf( "bogus checksum\n" );
        return 1;
    }

    switch ( u.gen->get_type() )
    {
    case Config_Add_Listen_Port_Reply::TYPE:
        if ( u.calpr->sequence_number.get() != seq )
        {
            printf( "seq error\n" );
        }
        else
        {
            printf( "added listen port\n", port );
        }
        break;

    case Config_Delete_Listen_Port_Reply::TYPE:
        if ( u.cdlpr->sequence_number.get() != seq )
        {
            printf( "seq error2\n" );
        }
        else
        {
            printf( "deleted port\n", port );
        }
        break;
    }

    return 0;
}
