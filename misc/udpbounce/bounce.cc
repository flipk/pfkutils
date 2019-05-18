
#include "posix_fe.h"

void
usage()
{
    printf("usage: bounce <port> <remote_ip> <remote_port>\n");
}

int
main(int argc, char ** argv)
{
    if (argc != 4)
    {
        usage();
        return 1;
    }

    uint16_t my_port, remote_port;
    uint32_t remote_addr;

    if (pxfe_iputils::parse_port_number (argv[1], &my_port    ) == false ||
        pxfe_iputils::hostname_to_ipaddr(argv[2], &remote_addr) == false ||
        pxfe_iputils::parse_port_number (argv[3], &remote_port) == false )
    {
        usage();
        return 1;
    }

    pxfe_errno e;

    pxfe_sockaddr_in remote;
    remote.init(remote_addr, remote_port);

    pxfe_udp_socket  s;
    if (s.init(my_port, &e) == false)
    {
        printf("socket: %s\n", e.Format().c_str());
        return 1;
    }

    pxfe_ticker   tick;
    tick.start(1,0);

    int count = 0;
    int drop = 0;

    bool done = false;
    while (!done)
    {
        pxfe_string buf;
        pxfe_sockaddr_in  addr;

        pxfe_select sel;
        sel.rfds.set(0);
        sel.rfds.set(s);
        sel.rfds.set(tick.fd());
        sel.tv.set(2,0);

        sel.select();

        if (sel.rfds.is_set(0))
        {
            char c;
            if (read(0, &c, 1) == 0)
                done = true;
            else
                drop++;
        }
        if (sel.rfds.is_set(s))
        {
            if (s.recv(buf, addr, &e))
            {
                if (drop == 0)
                {
                    if (s.send(buf, remote, &e) == false)
                    {
                        printf("send: %s\n", e.Format().c_str());
                    }
                }
                else
                    drop--;
                count++;
            }
            else
            {
                printf("recv: %s\n", e.Format().c_str());
            }
        }
        if (sel.rfds.is_set(tick.fd()))
        {
            char c;
            read(tick.fd(), &c, 1);
            if (count > 0)
                printf("%d\n", count);
            count = 0;
        }
    }

    return 0;
}
