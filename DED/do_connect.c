
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#if defined(SUNOS) || defined(SOLARIS)
#define socklen_t int
#define setsockoptcast char*
#else
#define setsockoptcast void*
#endif

int
do_connect( char * host, int port )
{
    int sock, siz;
    struct hostent *he;
    struct sockaddr_in sa;
    int salen;

    sock = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);

    if (sock < 0)
    {
        fprintf( stderr, "socket: %s\n", strerror( errno ));
        exit( 1 );
    }

    siz = 64 * 1024;
    (void)setsockopt(sock, SOL_SOCKET, SO_RCVBUF,
                     (setsockoptcast) &siz, sizeof(siz));
    (void)setsockopt(sock, SOL_SOCKET, SO_SNDBUF,
                     (setsockoptcast) &siz, sizeof(siz));
    siz = 1;
    (void)setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
                     (setsockoptcast) &siz, sizeof(siz));

    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);

    if (host == NULL)
    {
        int ear;
        sa.sin_addr.s_addr = INADDR_ANY;

        if (bind(sock, (struct sockaddr *)&sa, sizeof(sa)) < 0) 
        {
            fprintf( stderr,
                     "bind: %s\n", strerror( errno ));
            exit( 1 );
        }

        listen(sock, 1);
        salen = sizeof(struct sockaddr_in);
        ear = accept4(sock, (struct sockaddr *)&sa, &salen, SOCK_CLOEXEC);

        if (ear < 0)
        {
            fprintf( stderr,
                     "accept: %s\n", strerror( errno ));
            exit( 1 );
        }

        close(sock);
        sock = ear;
    }
    else
    {
        if (!(inet_aton(host, &sa.sin_addr)))
        {
            if ((he = gethostbyname(host)) == NULL)
            {
                fprintf( stderr,
                         "gethostbyname: %s\n", strerror( errno ));
                exit( 1 );
            }
            bcopy((char *)he->h_addr, (char*)&sa.sin_addr,
                  he->h_length);
        }

        if ((connect(sock, (struct sockaddr *)&sa, sizeof sa)) < 0)
        {
            fprintf( stderr,
                     "connect: %s\n", strerror( errno ));
            exit( 1 );
        }
    }

    return sock;
}
