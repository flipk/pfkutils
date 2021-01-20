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
#include "detach.h"

/*
   detach from the controlling terminal
*/
void 
detach() 
{
  int f;

  if (fork()) 
    exit(0);
  f = open("/dev/tty", O_RDWR);
  if (f >= 0) {
    if (ioctl(f, TIOCNOTTY, 0) < 0) {
      perror("TIOCNOTTY");
      exit(1);
    }
    close(f);
  }
  chdir("/");
  setpgrp(0, getpid());
  close(0);
  close(1);
  close(2);
  (void)open("/dev/null", O_RDWR);
  (void)open("/dev/null", O_RDWR);
  (void)open("/dev/null", O_RDWR);
}
