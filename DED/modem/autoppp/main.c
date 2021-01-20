#include <stdio.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <sys/ioctl.h>
#include <machine/limits.h>
#include <setjmp.h>

#include "serial_port.h"
#include "conf.h"
#include "debug.h"

void do_durham_connection();
void do_durham_command();

void do_direct_connection();
void do_direct_command();

void line_status_wait();

char *progname;
int child_pid;

extern jmp_buf chld_buf;

void
go_away()
{
  DEBUG("I am going away");
  signal(SIGCHLD, SIG_IGN);
  hangup();
  exit(0);
}

void
usage()
{
}

int
main(argc, argv)
     int argc;
     char **argv;
{
  int do_durham_conn, do_direct_conn;

  progname = argv[0];
  child_pid = 0;

  do_durham_conn = do_direct_conn = 0;


  if (strcmp(argv[1], "durham") == 0)
    do_durham_conn = 1;
  if (strcmp(argv[1], "direct") == 0)
    do_direct_conn = 1;

  signal(SIGINT, go_away);
  signal(SIGQUIT, go_away);
  signal(SIGHUP, SIG_IGN);
  signal(SIGTERM, go_away);

/* detach(); */

  while (1) {
    device = DEVICE;
    DEBUG("opening device");
    open_device();
    DEBUG("initializing");
    init_all();
    DEBUG("initializing modem");
    init_modem();
    if (do_durham_conn) {
      do_durham_connection();
      do_durham_command();
    } else {
      do_direct_connection();
      do_direct_command();
    }
    line_status_wait();
  }
  /*NOTREACHED*/
}
