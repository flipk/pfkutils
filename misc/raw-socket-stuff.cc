
#include <inttypes.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

int main()
{

#if 0
// list all the interfaces:
    {
        struct if_nameindex * i, * if_ni = if_nameindex();
        for (i = if_ni; i->if_name != NULL; i++)
            printf("%d : %s\n", i->if_index, i->if_name);
        if_freenameindex(if_ni);
    }
#endif

#if 0
    // create a socket that can pick up
    // all packets of a particular ethernet proto (or ALL):
    {
        uint16_t proto = htons(ETH_P_ALL); // or htons(0xSOMEPROTO)

        int s = socket(AF_PACKET, SOCK_RAW, proto);

    // bind only to one interface for which
    // the ifindex is known (see below to find that):
        struct sockaddr_ll sall;

        sall.sll_family = AF_PACKET;
        sall.sll_protocol = proto;
        sall.sll_ifindex = 2; //ifindex;

        if (bind(s, (struct sockaddr *)&sall, sizeof(sall)) < 0)
        {
            perror("bind");
            return 1;
        }

        while (1)
        {
            uint8_t buf[2000];
            int cc = read(s, buf, sizeof(buf));
            for (int ind = 0; ind < cc; ind++)
            {
                switch (ind) {
                case 6:  putchar(' '); break;
                case 12: putchar(' '); break;
                case 14: putchar(' '); break;
                }
                printf("%02x", buf[ind]);
            }
            printf("\n");
        }
    }
#endif

#if 0
// look up an ifindex given the interface name
    {
        int s = socket(AF_PACKET, SOCK_RAW, ETH_P_ALL);
        if (s < 0)
        {
            perror("socket");
            return 1;
        }
        struct ifreq ifr;
        strcpy(ifr.ifr_name, "enp2s0");
        if (ioctl(s, SIOCGIFINDEX, &ifr) < 0)
        {
            perror("ioctl");
            return 1;
        }
        printf("ifindex : %d\n", ifr.ifr_ifru.ifru_ivalue);
    }
#endif

#if 0
// look up the ethernet mac address of an interface
    {
        int s = socket(AF_PACKET, SOCK_RAW, ETH_P_ALL);
        if (s < 0)
        {
            perror("socket");
            return 1;
        }
        struct ifreq ifr;
        strcpy(ifr.ifr_name, "enp2s0");
        if (ioctl(s, SIOCGIFHWADDR, &ifr) < 0)
        {
            perror("ioctl");
            return 1;
        }
        uint8_t * b = (uint8_t *) &ifr.ifr_hwaddr.sa_data;
        switch (ifr.ifr_hwaddr.sa_family)
        {
        case ARPHRD_ETHER:
            printf("  hw addr: ");
            for (int ind = 0; ind < ETH_ALEN; ind++)
                printf("%02x ", b[ind]);
            printf("\n");
            break;
        }
    }
#endif

#if 0
// look up an IP address of an interface
// (not sure how to look up aliases...)
    {
        int s = socket(AF_PACKET, SOCK_RAW, ETH_P_ALL);
        if (s < 0)
        {
            perror("socket");
            return 1;
        }
        struct ifreq ifr;
        strcpy(ifr.ifr_name, "enp2s0");
        if (ioctl(s, SIOCGIFADDR, &ifr) < 0)
        {
            perror("ioctl");
            return 1;
        }
        switch (ifr.ifr_addr.sa_family)
        {
        case PF_INET:
        {
            struct sockaddr_in * sa_in = (struct sockaddr_in *) &ifr.ifr_addr;
            uint32_t  addr = ntohl(sa_in->sin_addr.s_addr);
            printf("inet %d.%d.%d.%d\n",
                   (addr >> 24) & 0xFF,
                   (addr >> 16) & 0xFF,
                   (addr >>  8) & 0xFF,
                   (addr >>  0) & 0xFF);
            break;
        }
        }
    }
#endif

    return 0;
}
