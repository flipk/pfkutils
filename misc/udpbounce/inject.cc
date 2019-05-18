
#include "posix_fe.h"

void
usage()
{
    printf("usage: inject <ip> <port>\n");
}

int
main(int argc, char ** argv)
{
    if (argc != 3)
    {
        usage();
        return 1;
    }

    uint16_t remote_port;
    uint32_t remote_addr;

    if (pxfe_iputils::hostname_to_ipaddr(argv[1], &remote_addr) == false ||
        pxfe_iputils::parse_port_number (argv[2], &remote_port) == false )
    {
        usage();
        return 1;
    }

    pxfe_errno e;

    pxfe_sockaddr_in remote;
    remote.init(remote_addr, remote_port);

    pxfe_udp_socket s;
    s.init();

    pxfe_string  buf = "THIS IS A TEST MESSAGE WHICH IS FAIRLY SHORT";

    if (s.send(buf, remote, &e) == false)
    {
        printf("send: %s\n", e.Format().c_str());
    }

    return 0;
}
