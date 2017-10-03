
#include "sw.H"
#include "test.H"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

net_thread :: net_thread( short local_port,
                          char * remote_addr, short remote_port )
{
    set_name( "net" );
    net_sw_qid = msg_create( "net sw" );

    remote_sa.sin_family = PF_INET;
    remote_sa.sin_port = htons(remote_port);

    if (!inet_aton(remote_addr, &remote_sa.sin_addr))
    {
        struct hostent _he;
        struct hostent * he;
        int h_errno;
        char he_buf[1000];

    // POSIX.1-2001 marks gethostbyaddr() and gethostbyname() obsolescent,
    // should eventually switch to getaddrinfo.

        if (gethostbyname_r(remote_addr, &_he,
                            he_buf, sizeof(he_buf),
                            &he, &h_errno) == 0)
        {
            if ( he )
                memcpy( &remote_sa.sin_addr, he->h_addr, he->h_length );
            else
            {
                printf("unable to resolve hostname\n");
                exit(1);
            }
        }
        else
        {
            fprintf(stderr, "unable to resolve hostname '%s'\n",
                    remote_addr);
            exit(1);
        }
    }

    fd = socket(PF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
    {
        perror("socket");
        exit(1);
    }
    struct sockaddr_in sa;
    sa.sin_family = PF_INET;
    sa.sin_port = htons(local_port);
    sa.sin_addr.s_addr = INADDR_ANY;
    if (bind(fd, (struct sockaddr *)&sa, sizeof(sa)) < 0)
    {
        perror("bind");
        exit(1);
    }

    pkfdid = -1;

    resume();
}

net_thread :: ~net_thread( void )
{
    msg_destroy( net_sw_qid );
    close(fd);
}

void
net_thread :: register_my_fd(void)
{
    if (pkfdid == -1)
        pkfdid = register_fd(fd, PK_FD_Read, net_sw_qid, this);
}

void
net_thread :: handle_net_rx(void)
{
    // xxx
}

// virtual
void
net_thread :: entry( void )
{
    printf( "%s: created qid %d; connected to %d %d\n",
            get_name(), net_sw_qid,
            sw_tx_qid, sw_rx_qid );

    union {
        pk_msg_int * m;
        PK_FD_Activity * pkfda;
        //
    } m;

    register_my_fd();

    while (1)
    {
        m.m = msg_recv( 1, &net_sw_qid, NULL, -1 );
        if (!m.m)
            continue;

        switch (m.m->type)
        {
            // xxx

        case PK_FD_Activity::TYPE:
            pkfdid = -1;
            handle_net_rx();
            register_my_fd();
            break;

        default:
            printf("%s: unknown msg type %#x received!\n", m.m->type);
            break;
        }

        delete m.m;
    }
}
