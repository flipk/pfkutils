#include <stdio.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <sys/ioctl.h>

#include "serial_port.h"
#include "conf.h"
#include "debug.h"

/*
	dial the phone directly thru my office phone.

	this is designed to be a lot weenier in its attack,
	because the phone at work is used for other stuff.
	we don't want lynn to have to put up with her phone
	ringing until she picks it up every ten seconds
	until i get into work, now would we?

	(although it was tempting, because it really should be
	 my phone anyway.)
*/

extern int child_pid;
void go_away();

const char *direct1_wait[] = {
  "login: ",
  "Password:",
  NULL
};

void 
do_direct_connection() 
{
  M_RESULT mres;
  int retrycount;
  int gotstr;
  int passwdtries;

  retrycount = passwdtries = 0;
  DEBUG("beginning direct connection");
 dial_direct_again:
  mres = dial(DIRECT_PHONE, 0);
  if (mres == M_BUSY) {
    DEBUG("office phone is busy?! Bugging out!");
    go_away();
  }
  if (mres == M_TIMEOUT) {
    if (++retrycount < 2) {
      DEBUG("generic timeout; trying again.");
      hangup();
      goto dial_direct_again;
    }
    DEBUG("bugging out on generic timeout..");
    hangup();
    go_away();
  }
  if (mres == M_FAIL) {
    DEBUG("generic failure? bugging out..");
    hangup();
    go_away();
  }
 script1_again:
  gotstr = waitfor((char**)direct1_wait, 120);
  if (gotstr != 0) {
    hangup();
    if (++retrycount < 2) {
      DEBUG("failure; retrying.");
      goto dial_direct_again;
    }
    DEBUG("failure; bugging out");
    go_away();
  }
  send_string(DIRECT_LOGIN);
  send_string("\r");
  gotstr = waitfor((char**)direct1_wait, 120);
  if (gotstr != 1) {
    hangup();
    if (++retrycount < 2) {
      DEBUG("failure; retrying.");
      goto dial_direct_again;
    }
    DEBUG("failure; bugging out");
    go_away();
  }
  send_string(DIRECT_PASSWORD);
  send_string("\r");
  DEBUG("connection successful!");
}

void 
do_direct_command() 
{
  if ((child_pid = fork()) == 0) {
    execl(DIRECT_COMMAND, DIRECT_COMMAND, 0);
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
