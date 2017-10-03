#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

struct sendpingbuf {
    struct icmp icmp;
    struct timeval tv;
    char string[ 32 ];
};
struct recvpingbuf {
    struct ip ip;
    struct icmp icmp;
    struct timeval tv;
    char string[32];
};

int
in_cksum(unsigned short *buf, int sz)
{
    int nleft = sz;
    int sum = 0;
    unsigned short *w = buf;
    unsigned short ans = 0;

    while (nleft > 1) {
        sum += *w++;
        nleft -= 2;
    }

    if (nleft == 1) {
        *(unsigned char *) (&ans) = *(unsigned char *) w;
        sum += ans;
    }

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    ans = ~sum;
    return (ans);
}

/* calculate *a - *b and place the result in *result. */
void
subtract_timevals( struct timeval * result,
                   struct timeval * a,
                   struct timeval * b )
{
    struct timeval r;

    r.tv_sec  = a->tv_sec  - b->tv_sec;
    r.tv_usec = a->tv_usec - b->tv_usec;

    if (r.tv_usec < 0)
    {
        /* borrow from seconds */
        r.tv_usec += 1000000;
        r.tv_sec  -= 1;
    }

    *result = r;
}

/* return 1 if *a > *b, return -1 if *a < *b, return 0 if *a == *b */
int
compare_timevals( struct timeval * a,
                  struct timeval * b )
{
    if (a->tv_sec < b->tv_sec)
        return -1;
    if (a->tv_sec > b->tv_sec)
        return 1;
    if (a->tv_usec < b->tv_usec)
        return -1;
    if (a->tv_usec > b->tv_usec)
        return 1;
    return 0;
}

void
send_ping(int sock, struct sockaddr_in dest, short identifier, short sequence)
{
    int cc;
    struct sendpingbuf pb;
    char * string;

    memset(&pb, 0, sizeof(pb));
    pb.icmp.icmp_type = ICMP_ECHO;
    pb.icmp.icmp_code = 0;
    pb.icmp.icmp_seq = sequence;
    pb.icmp.icmp_id = identifier;
    gettimeofday(&pb.tv, NULL);
    sprintf(pb.string, "CPE router health check");
    pb.icmp.icmp_cksum = in_cksum((unsigned short *)&pb, sizeof(pb));

    fprintf(stderr, "INFO: sending ping with id %d seq %d\n",
            identifier, sequence);

    cc = sendto( sock, &pb, sizeof(pb), 0, 
                 (struct sockaddr *) &dest, sizeof(dest));

    if (cc != sizeof(pb))
    {
        fprintf(stderr,
                "ERR: sendto returns %d (errno %d:%s)\n",
                cc, errno, strerror(errno));
        /* xxx recovery? */
        exit(1);
    }
}

/* return 1 if response was received
 * return 0 if no response received 
 */

int
get_response(int sock, short identifier)
{
    struct recvpingbuf pb;
    struct timeval now, diff;
    int cc;
    struct sockaddr_in source;
    socklen_t source_len = sizeof(source);

    cc = recvfrom(sock, &pb, sizeof(pb), 0,
                  (struct sockaddr *) &source, &source_len);
    gettimeofday( &now, NULL);
    if (pb.icmp.icmp_type != ICMP_ECHOREPLY)
    {
        if (pb.icmp.icmp_type == ICMP_ECHO)
            fprintf(stderr,
                    "INFO: ping for someone else received\n");
        else
            fprintf(stderr,
                    "INFO: unknown icmp type %d received\n",
                    pb.icmp.icmp_type);
        return 0;
    }
    if (pb.icmp.icmp_id != identifier)
    {
        fprintf(stderr,
                "INFO: icmp ping response received for unknown id %d\n",
                pb.icmp.icmp_id);
        return 0;
    }

    subtract_timevals(&diff, &now, &pb.tv);

    fprintf(stderr,
            "NOTE: icmp ping response seq %d received (rtd = %d.%06d)\n",
            pb.icmp.icmp_seq, diff.tv_sec, diff.tv_usec);
    return 1;
}

void
init(char *addr, struct sockaddr_in *dest, int *sock)
{
    struct hostent * he;
    struct protoent * protoe;
    int proto, s;

    if (!(inet_aton(addr, &dest->sin_addr)))
    {
        if ((he = gethostbyname(addr)) == NULL)
        {
            fprintf(stderr,
                    "ERR: unknown host '%s'!\n", addr);
            exit(1);
        }
        memcpy(&dest->sin_addr, he->h_addr, he->h_length);
    }

    dest->sin_family = AF_INET;
    protoe = getprotobyname("icmp");

    if (protoe)
        proto = protoe->p_proto;
    else
        proto = 1; /* hardcode ICMP */

    s = socket(AF_INET, SOCK_RAW, proto);

    if (s < 0)
    {
        fprintf(stderr, "ERR: unable to make icmp socket! %s\n",
                strerror(errno));
        exit(1);
    }
    *sock = s;
}

void
send_success_message_to_wcm()
{
    fprintf(stderr, "INFO: sending success message to WCM\n");
    /* xxx needs implement */
}

void
send_fail_message_to_wcm()
{
    fprintf(stderr, "INFO: sending fail message to WCM\n");
    /* xxx needs implement */
}

/*
// while (1)
//   sleep poll_interval
//   set attempt=0
//   identifier=random
//   while (1)
//     send ping
//     attempt++
//     wait wait_interval for response to arrive
//     if response arrived
//        send success to wcm
//        break inner loop, continue outer loop
//     else
//        if attempt >= max_attempts
//           send failure to wcm
//           break inner loop, continue outer loop
*/

int
main()
{

// xxx the following four should come from the command line
//     rather than being hardcoded.

    char * addr = "10.19.173.30";
    int  poll_interval = 15;  // eventually, 60
    int  wait_interval =  3;
    int  max_attempts = 5;


    struct sockaddr_in dest;
    int sock;
    short sequence;

    init(addr, &dest, &sock);

    sequence = 1;

    /* while while-loop is invoked once per polling cycle. */
    while (1)
    {
        struct timeval tv, now, expire;
        int attempt = 0;
        short identifier = random() & 0x7fff;
        int response = 0;
        fd_set  rfds;

        /* each polling interval, send pings until a response
           is heard or the maximum number of pings are sent. */
        do {
            send_ping(sock,dest,identifier,sequence);

            sequence++;
            if (sequence < 0)
                sequence = 1;
            attempt++;

            gettimeofday(&now,NULL);
            expire = now;
            expire.tv_sec += wait_interval;

            /* after sending each ping, wait a certain amount of
               time for a response before sending the next request.
               this do-while loop ensures that extraneous messages
               for other ongoing pings will not disrupt the timeout
               interval for our desired response. */
            do {
                subtract_timevals(&tv, &expire, &now);

                FD_ZERO(&rfds);
                FD_SET(sock, &rfds);

                fprintf(stderr, "INFO: select for %d.%06d\n",
                        tv.tv_sec, tv.tv_usec);

                if (tv.tv_sec >= 0)
                    if (select(sock+1, &rfds, NULL, NULL, &tv) > 0)
                    {
                        fprintf(stderr, "INFO: select awoke!\n");
                        if (get_response(sock, identifier))
                        {
                            fprintf(stderr, "INFO: got expected response!\n");
                            response = 1;
                        }
                    }

                gettimeofday(&now, NULL);

            } while (!response &&
                     compare_timevals(&now, &expire) < 0);

        } while (!response && attempt < max_attempts);

        /* inform wcm of our success or failure. */
        if (response)
            send_success_message_to_wcm();
        else
            send_fail_message_to_wcm();

        gettimeofday(&now,NULL);
        expire = now;
        expire.tv_sec += poll_interval;

        /* now sleep for the desired poll interval; 
           during the poll interval, absorb and ignore
           all icmp messages, to ensure the queue is empty
           next time we send a ping and expect a response.
           also ensure these messages do not disturb our
           timeout interval. */
        do {
            subtract_timevals(&tv, &expire, &now);

            FD_ZERO(&rfds);
            FD_SET(sock, &rfds);
            fprintf(stderr, "INFO: sleeping for %d.%06d\n", 
                    tv.tv_sec, tv.tv_usec);

            if (tv.tv_sec >= 0)
                if (select(sock+1, &rfds, NULL, NULL, &tv) > 0)
                    get_response(sock, identifier); // ignore

            gettimeofday(&now, NULL);

        } while (compare_timevals(&now, &expire) < 0);
    }

    /* NOT REACHED */

    return 0;
}
