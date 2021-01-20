#include <stdio.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <sys/ioctl.h>
#include <machine/limits.h>
#include <setjmp.h>

#include "conf.h"
#include "debug.h"

/*
        now we sit and watch the operating link. if the pppd child
        dies, we'll know it (setjmp/longjmp), in the meantime we wake
        up every user-configured seconds and send an icmp ping.
        actually, the library routine ping_time() sends a ping, and
        waits up to ten seconds for the reply. on failure it will retry
        six times. this loop below also gives that loop two chances.
        so from the time it sends the first ping to the time it decides
        the link is dead, is two minutes; and in that time twelve pings
        will have been sent (and not recieved).

        this oughta be enough to prevent false notions of dead-ness.
*/

extern int child_pid;

jmp_buf chld_buf;

void log_ping_time(int);

void 
dead_child() 
{
  child_pid = 0;
  longjmp(chld_buf, 1);
}

void 
line_status_wait() 
{
  int pt;
  char s[128];

  signal(SIGCHLD, dead_child);
  if (setjmp(chld_buf) == 0) {
    /* can't send a ping while pppd is negotiating LCP */
    sleep(10);   /* long enough for connection to come up */
    while (1) {
      pt = ping_time(PING_ADDRESS);
      if (pt == -1) {
        DEBUG("line seems dead! one more try...");
        pt = ping_time(PING_ADDRESS);
        if (pt == -1) {
          DEBUG("line is dead. starting over.\n");
          kill(child_pid, SIGINT);
          sprintf(s, "waiting for pppd(%d) to die before restarting");
          DEBUG(s);
          sleep(60); /* and wait for it to die.. */
          /* if it hasn't died yet, force kill it */
          kill(child_pid, SIGKILL);
          sleep(INT_MAX); /* it should die now. */
        }
      }
      log_ping_time(pt);
      sleep(PING_TIME);
    }
    /*NOTREACHED*/
  }
  /* means child is now dead */
  wait(NULL);  /* to collect the zombie */
  hangup();    /* the line may not have hung up, especially over pty */
}

void 
log_ping_time(ptime) 
     int ptime;
{
  FILE *ping_time_f;

  if ((ping_time_f = fopen("/var/run/ppp-ping-time","w")) != NULL) {
    fprintf(ping_time_f, "%d\n", ptime);
    fclose(ping_time_f);
  }
}
