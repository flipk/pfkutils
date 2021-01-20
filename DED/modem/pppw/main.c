#include <stdio.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>

/*
   wait for ppp to start; this way you can do
          "pppw ; telnet HOSTNAME"
   running in another window while autoppp is dialing.

   it exits and allows the telnet command to start
   when it detects the ppp0 interface has come up.

   also helpful if you have an automatic script that depends
   on the ppplink being up.
*/

int ppp_stat() {
  struct ifreq ifr;
  int s;

  strcpy(ifr.ifr_name, "ppp0");
  s = socket(AF_INET, SOCK_DGRAM, 0);
  if (ioctl(s, SIOCGIFFLAGS, (caddr_t)&ifr) < 0) {
    perror("ioctl (SIOCGIFFLAGS)");
    exit(1);
  }
  close(s);
  return(ifr.ifr_flags);
}

main() {
  while ((ppp_stat() & 1) != 1)
    sleep(1);
}
