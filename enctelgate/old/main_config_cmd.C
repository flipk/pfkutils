
#include "config_messages.H"

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

const int udp_port_no = 2100;

int
main()
{
    int fd;
    struct sockaddr_in sa;

    fd = socket( AF_INET, SOCK_DGRAM, 0 );
    if ( fd < 0 )
    {
        fprintf( stderr, "socket: %s\n", strerror( errno ));
        return 1;
    }

    sa.sin_family = AF_INET;
    sa.sin_port = htons( udp_port_no );
    sa.sin_addr.s_addr = INADDR_ANY;

    



}
