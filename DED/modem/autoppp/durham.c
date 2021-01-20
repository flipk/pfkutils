#include <stdio.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <sys/ioctl.h>

#include "serial_port.h"
#include "conf.h"
#include "debug.h"

/*
	this dials up thru the durham modem pool, telnets to
	the proper machine (the "remote end") and logs in as
	the account on that machine that has been set up to
	start ppp.

	this is very aggressive in its retry strategies,
	recognizing many of the common errors encountered
	while travelling thru the ENET menus.
*/

extern int child_pid;
void go_away();

const char *durham1_wait[] = {
  "keys:",
  "host... ",
  "login: ",
  "Password:",
  "continue...",  /* ENET error condition always ends with this */
  NULL
};

void 
do_durham_connection() 
{
  M_RESULT mres;
  int retrycount;
  int gotstr;
  int passwdtries;

  retrycount = passwdtries = 0;
  DEBUG("beginning durham connection");
 dial_durham_again:
  /* ENET can be busy occasionally, so we try really hard to get in. */
  mres = dial(DURHAM_PHONE, 45);
  switch (mres) {
  case M_BUSY:
    /* dial() has an internal busy retry counter, we don't have to retry */
    DEBUG("too many busy retries, bugging out!");
    go_away();
    /*NOTREACHED*/
  case M_TIMEOUT:
    if (++retrycount < 10) {
      DEBUG("durham phone timeout; retrying");
      goto dial_durham_again;
    }
    DEBUG("too many retries, bugging out!");
    go_away();
    /*NOTREACHED*/
  case M_FAIL:
    if (++retrycount < 2) {
      DEBUG("generic dialing failure, retrying");
      goto dial_durham_again;
    }
    DEBUG("generic dialing failure? bugging out!");
    go_away();
    /*NOTREACHED*/
  case M_CONNECTED:
    DEBUG("connected! beginning script..");
    break;
  }
 script1_again:
  gotstr = waitfor((char**)durham1_wait, 120);
  switch (gotstr) {
  case 0:
    send_string("1\r");
    break;
  default:
    if (++retrycount < 5) {
      DEBUG("xyplex menu never received.. retrying.");
      hangup();
      goto dial_durham_again;
    }
    DEBUG("too many tries getting xyplex menu.. bugging out!");
    hangup();
    go_away();
    /*NOTREACHED*/
  }
  gotstr = waitfor((char**)durham1_wait, 120);
  switch (gotstr) {
  case 1:
    send_string(PING_ADDRESS);
    send_string("\r");
    break;
  case 4:
    if (++retrycount < 5) {
      DEBUG("xyplex says error of some kind, trying menu again");
      send_string("\r");
      goto script1_again;
    }
    DEBUG("too many errors traversing xyplex menu, bugging out!");
    hangup();
    go_away();
    /*NOTREACHED*/
  default:
    if (++retrycount < 5) {
      DEBUG("xyplex traversal error (timeout?), trying again");
      hangup();
      goto dial_durham_again;
    }
    DEBUG("xyplex traversal error (timeout?), bugging out!");
    hangup();
    go_away();
    /*NOTREACHED*/
  }
 script2_again:
  gotstr = waitfor((char**)durham1_wait, 120);
  switch (gotstr) {
  case 2:
    send_string(DURHAM_LOGIN);
    send_string("\r");
    break;
  case 4:
    /* if our dest host reboots, it may take longer to come up than
       it takes us to redial. (it takes 36 seconds for my machine to
       dial, and a minute and a half for the dest host to reboot.)
       we try over and over 20 times, then we'll start sleeping and
       trying it every 15 seconds for 15 minutes (or so, depending
       on how long the timeout is in ENET if the host doesn't respond
       to TCP packets at all. If it gives up after 15 minutes of that,
       it then hangs up and tries once every 10 minutes after that. */
    if (++retrycount < 20) {
      DEBUG("remote machine not responding to xyplex, retrying..");
      send_string("\r");
      goto script2_again;
    }
    if (++retrycount < 70) {
      DEBUG("remote machine not responding, sleeping 15 seconds");
      goto script2_again;
    }
    DEBUG("machine is not responding. sleeping 10 minutes");
    hangup();
    sleep(600);
    DEBUG("trying again");
    goto dial_durham_again;
    /*NOTREACHED*/
  default:
    /* this should actually never happen (xyplex shutdown) */
    if (++retrycount < 5) {
      DEBUG("xyplex menus stopped responding, retrying..");
      hangup();
      goto dial_durham_again;
    }
    DEBUG("xyplex menus not responding, bugging out!");
    hangup();
    go_away();
    /*NOTREACHED*/
  }
  gotstr = waitfor((char**)durham1_wait, 15);
  switch (gotstr) {
  case 3:
    send_string(DURHAM_PASSWORD);
    send_string("\r");
    break;
  default:
    /* if i ever changed the password on the dest machine,
       this loop would go crazy */
    if (++passwdtries < 5) {
      DEBUG("machine doesn't want password? sleeping 5 minutes..");
      hangup();
      sleep(300);
      DEBUG("trying again..");
      goto dial_durham_again;
    }
    DEBUG("machine doesn't want the password. giving up.");
    hangup();
    go_away();
    /*NOTREACHED*/
  }
  DEBUG("connection established!");
}

void 
do_durham_command() 
{
  if ((child_pid = fork()) == 0) {
    execl(DURHAM_COMMAND, DURHAM_COMMAND, 0);
    DEBUG("couldn't exec! bugging out...");
    hangup();
    go_away();
    /*NOTREACHED*/
  }
  if (child_pid == -1) {
    DEBUG("couldn't fork! bugging out..");
    hangup();
    go_away();
    /*NOTREACHED*/
  }
}
