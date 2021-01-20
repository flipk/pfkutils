#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <setjmp.h>
#include <signal.h>

#include "serial_port.h"

void 
do_connection() 
{
  char s[1024];
  char myname[256];
  int pid;

  if ((pid=fork()) == 0) {
    init_no_clocal();
    init_icanon();
    init_echo();
    gethostname(myname, 255);
    sprintf(s, "\n\nNetBSD/i386 Dialup Server (%s)\n\n",
            myname);
    send_string(s);
    setsid();
    if (ioctl(s_device, TIOCSCTTY, (char *)NULL) < 0) {
      perror("TIOCSCTTY failed");
      _exit(0);
    }
    dup2(s_device, 0);
    dup2(s_device, 1);
    dup2(s_device, 2);
    close(s_device);
    s_device=0;
    execl("/usr/bin/login", "login", 0);
    _exit(0);
  }
  if (pid > 0)
    wait(NULL);
  else
    perror("error in fork");
  /* device only resets properly if you completely close it
     and re-open it (since the forked process also had a file
     handle open on the same device); device resets on the
     last close of it. */
  close(s_device);
  /* give kernel a chance to do its resetting thing */
  sleep(3);
  /* and reopen and init the device so that we leave things as 
     we found them */
  open_device();
  init_all();
}
