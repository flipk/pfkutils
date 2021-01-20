#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip_var.h>
#include <arpa/inet.h>
#include <signal.h>

#include "sig_handler.h"
#include "debug.h"

/*
   send pings until they come back (up to 6 pings with ten second waits
   for each ping). if after a minute of this there is no ping, returns
   the time as -1, else returns time in ms
*/

#define DATALEN (64 - 8);

int in_cksum(u_short *, int);
void pinger(int);
int calc_ping_time(char *, int);

u_char *ping_packet;
u_char outpack[sizeof(struct icmp)];
int ping_packlen;
struct sockaddr_in to;
int ping_sock, ping_ident;
int icmp_proto = -1;

/*
   look up the protocol number for icmp; likes to fail randomly for
   some unknown reason, so we only do it once or until needed at
   the start of the program
*/

int 
ping_init() 
{
  struct protoent *proto;
  
  while (!(proto = getprotobyname("icmp"))) {
    DEBUG("icmp protocol lookup failure; retrying.");
  }
  icmp_proto = proto->p_proto;
}

int 
ping_time(char *other_host) 
{
  struct hostent *hp;
  int cc;
  int retry_count=0;

  /* find IP number of dest host */
  bzero(&to, sizeof(struct sockaddr_in));
  if (inet_aton(other_host, &to.sin_addr) != 0) {
    to.sin_family = AF_INET;
  } else {
    if ((hp = gethostbyname(other_host)) == NULL)
      errx(1, "unknown host %s", other_host);
    to.sin_family = hp->h_addrtype;
    bcopy(hp->h_addr, &to.sin_addr, hp->h_length);
  }
  /* assemble the ping packet */
  ping_packlen = sizeof(struct icmp) + sizeof(struct ip) + 64;
  if (!(ping_packet = (u_char *)malloc(ping_packlen)))
    err(1, "malloc");

  /* look up the protocol the first time */
  if (icmp_proto == -1)
    ping_init();

  /* create us a socket */
  if ((ping_sock = socket(AF_INET, SOCK_RAW, icmp_proto)) < 0)
    err(1,"socket");

  signal(SIGALRM, sig_handler);
  /* unique number for name of ping */
  ping_ident = getpid();

  setjmp(jb); /* if ALARM signal, we jump back to here to retry */
  if (++retry_count > 6) {
    alarm(0);
    goto give_up;
  }
  pinger(retry_count);
  alarm(10);
  
 wait_some_more:
  if ((cc = recv(ping_sock, (char*)ping_packet, ping_packlen, 0)) < 0) {
    if (errno == EINTR)
      goto wait_some_more;
    perror("ping: recvfrom");
    goto wait_some_more;
  }
  alarm(0);
  close(ping_sock);
  return calc_ping_time(ping_packet, ping_packlen);
 give_up:
  close(ping_sock);
  return -1;
}

/*
   send a ping
*/
void 
pinger(int seq) 
{
  register struct icmp *icp;
  register int cc;
  int i;

  icp = (struct icmp *)outpack;
  icp->icmp_type = ICMP_ECHO;
  icp->icmp_code = 0;
  icp->icmp_cksum = 0;
  icp->icmp_seq = seq;
  icp->icmp_id = ping_ident;

  /* part of the data in a ping packet
     is the time of day it was sent, so that
     we have something to compare to when it comes back */

  gettimeofday((struct timeval *)&icp->icmp_data,
               (struct timezone *)NULL);

  cc = sizeof(struct icmp);
  icp->icmp_cksum = in_cksum((u_short *)icp, cc);
  
  i = sendto(ping_sock, (char *)outpack, cc, 0, (struct sockaddr *)&to,
             sizeof(struct sockaddr));
  
    if (i < 0 || i != cc) {
    if (i < 0)
      perror("ping: sendto");
    printf("wrote bytes = %d, should have been %d\n", i, cc);
  }
}

/* 
   read contents of returned ping packet
   and find the time diff from it to NOW
*/
int 
calc_ping_time(char *buf, int length) 
{
  struct timeval tv, tp;
  struct ip *ip;
  register struct icmp *icp;
  int hlen;
  int triptime=-1;

  (void)gettimeofday(&tv, (struct timezone *)NULL);
  ip = (struct ip *)buf;
  hlen = ip->ip_hl << 2;
  if (length < hlen + ICMP_MINLEN) {
    warnx("packet too short (%d bytes, should be %d)", length, hlen);
    return;
  }
  length -= hlen;
  buf += hlen;
  icp = (struct icmp *)buf;
  if (icp->icmp_type == ICMP_ECHOREPLY) {
    /* we may get pings back that aren't for us
       (but for other ping program running on this host)
       since raw datagram sockets are wide-open for whole machine */
    if (icp->icmp_id != ping_ident)      
      return;
    bcopy(&icp->icmp_data, &tp, sizeof(tp));
    timersub(&tv, &tp, &tv);
    
    triptime = ((double)tv.tv_sec) * 1000.0 + ((double)tv.tv_usec) / 1000.0;
  }    
  return triptime;
}

/*
   if a ping packet is to be returned at all, it must have
   the proper checksum on the way out
*/
int 
in_cksum(u_short *addr, int len) 
{
  register int nleft = len;
  register u_short *w = addr;
  register int sum = 0;
  u_short answer = 0;
  
  /*
   * Our algorithm is simple, using a 32 bit accumulator (sum), we add
   * sequential 16 bit words to it, and at the end, fold back all the
   * carry bits from the top 16 bits into the lower 16 bits.
   */
  while (nleft > 1)  {
    sum += *w++;
    nleft -= 2;
  }
  
  /* mop up an odd byte, if necessary */
  if (nleft == 1) {
    *(u_char *)(&answer) = *(u_char *)w ;
    sum += answer;
  }
  
  /* add back carry outs from top 16 bits to low 16 bits */
  sum = (sum >> 16) + (sum & 0xffff);     /* add hi 16 to low 16 */
  sum += (sum >> 16);                     /* add carry */
  answer = ~sum;                          /* truncate to 16 bits */
  return(answer);
}
