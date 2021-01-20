#include <stdio.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <sys/ioctl.h>

#include "conf.h"
#include "detach.h"
#include "serial_port.h"
#include "sig_handler.h"
#include "debug.h"

/*
   serve dialup

   this program solves a couple of problems with getty.

   getty on a modem operates as follows: you configure the modem to
   automatically answer incoming calls ("ats0=N" where N is the number
   of rings); then getty opens the port in BLOCKING and -clocal mode,
   which causes getty to block until the modem answers a call and the
   kernel detects that CarrierDetect has gone true. then it proceeds
   as usual.

   unfortunately, this means that the modem cannot be told NOT to answer
   the phone w/o running a hand-written external module that munges the
   /etc/ttys, does some kill -1's, etc, etc.

   this program opens the modem port and waits for the "RING" from the
   modem (so it doesn't need to set the S0 register and have the modem
   answer itself), and when RING is detected, it answers via "ATA\n".
   It also spits out a login banner and spawns a login process, restarting
   (or exiting, depending on your cmdline options) when the connection
   has stopped.
*/

void serve_dialup();

char *progname;
int my_online=0;

void
usage() 
{
}

void 
go_away() 
{
  if (my_online)
    hangup();
  exit(0);
}

int 
main(argc, argv) 
     int argc;
     char **argv;
{
  progname = argv[0];

  DEBUG("initializing signals");
  signal(SIGINT, go_away);
  signal(SIGQUIT, go_away);
  signal(SIGHUP, SIG_IGN);
  signal(SIGTERM, go_away);

  DEBUG("detach here");
/*  detach(); */
  
  while (1)
    {
      DEBUG("serving dialup");
      serve_dialup();
    }

  /*NOTREACHED*/
  go_away();
}

void
serve_dialup()
{
  char dummy;

  device = DEVICE;
  DEBUG("opening device");
  open_device();
  DEBUG("initializing serial port");
  init_all();
  DEBUG("initializing modem");
  init_modem();
  DEBUG("waiting for call");
  answer_phone();
  my_online=1;
  send_string("\n\n");
  signal(SIGALRM, sig_handler);
  alarm(5);
  DEBUG("catching icky bytes after call");
  if (setjmp(jb) == 0)
    while (1)
      read(s_device, &dummy, 1);
  signal(SIGALRM, SIG_IGN);
  alarm(0);
  DEBUG("beginning connection");
  do_connection();
  DEBUG("hanging up");
  hangup();
  my_online=0;
}
