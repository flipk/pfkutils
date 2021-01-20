#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <setjmp.h>
#include <signal.h>
#include <errno.h>

#include "serial_port.h"
#include "sig_handler.h"
#include "debug.h"

/* 
   do things with a serial port and with a modem.
*/

char *device;
int s_device;
struct termios tmode;

void
open_device()
{
  int off=0;

  if ((s_device = open(device, O_RDWR | O_NONBLOCK)) < 0)
    err(1,"open device");
  /* get its mode */
  if (tcgetattr(s_device, &tmode) < 0) {
    perror("tcgetattr failed");
    exit(1);
  }
  /* flush everything from it */
  (void)tcflush(s_device, TCIOFLUSH);
  ioctl(s_device, FIONBIO, &off);
  ioctl(s_device, FIOASYNC, &off);
}

void
init_all()
{

#define SET(a,b)    (a) |= (b)
#define RESET(a,b)  (a) &= ~(b)

/*
  tmode.c_iflag |= (IGNBRK | IGNPAR | IXON | IXOFF | IXANY);
  tmode.c_iflag &= (~BRKINT & ~INPCK & ~ISTRIP);
  
  tmode.c_cflag &= (~CSIZE & ~PARODD & ~PARENB);
  tmode.c_cflag |= (CS8 | CREAD | CRTSCTS | HUPCL | CLOCAL);

  tmode.c_lflag |= (ECHOE | ECHOKE);
  tmode.c_lflag &= (~ICANON & ~ISIG & ~IEXTEN & ~ECHO & ~ECHOCTL
                    & ~ECHOK & ~ECHONL);
*/

  SET(tmode.c_iflag, IGNBRK | IGNPAR | IXON | IXOFF | IXANY);
  RESET(tmode.c_iflag, BRKINT | INPCK | ISTRIP);

  SET(tmode.c_cflag, CS8 | CREAD | CRTSCTS | HUPCL | CLOCAL);
  RESET(tmode.c_cflag, CSIZE | PARODD | PARENB);

  SET(tmode.c_lflag, ECHOE | ECHOKE);
  RESET(tmode.c_lflag, 
	ICANON | ISIG | IEXTEN | ECHO | ECHOCTL | ECHOK | ECHONL);

  tmode.c_ispeed = 115200;
  tmode.c_ospeed = 115200;

  if (tcsetattr(s_device, TCSANOW, &tmode) < 0) {
    perror("tcsetattr failed");
    exit(1);
  }
}

void
init_clocal()
{
  struct termios nmode;
  tcgetattr(s_device, &nmode);
  SET(nmode.c_cflag, CLOCAL);
  tcsetattr(s_device, TCSANOW, &nmode);
}

void 
init_no_clocal()
{
  struct termios nmode;
  tcgetattr(s_device, &nmode);
  RESET(nmode.c_cflag, CLOCAL);
  tcsetattr(s_device, TCSANOW, &nmode);
}

void
init_icanon() 
{
  struct termios nmode;
  tcgetattr(s_device, &nmode);
  SET(nmode.c_lflag, ICANON);
  tcsetattr(s_device, TCSANOW, &nmode);
}  

void
init_no_icanon() 
{
  struct termios nmode;
  tcgetattr(s_device, &nmode);
  RESET(nmode.c_lflag,ICANON);
  tcsetattr(s_device, TCSANOW, &nmode);
}

void 
init_echo() 
{
  struct termios nmode;
  tcgetattr(s_device, &nmode);
  SET(nmode.c_lflag, ECHO);
  tcsetattr(s_device, TCSANOW, &nmode);
}

void 
init_no_echo() 
{
  struct termios nmode;
  tcgetattr(s_device, &nmode);
  RESET(nmode.c_lflag, ECHO);
  tcsetattr(s_device, TCSANOW, &nmode);
}

void 
send_string(s) 
     char *s;
{
  write(s_device, s, strlen(s));
}

/*
   wait for a string to come over the link; we give it an array of strings
   to watch for, if one comes over, we return the index into the array 
   indicating which one we got back.

   if the timeout period expires, return -1. if timeout==0, wait forever
   until one returns.
*/

int 
waitfor(targets, timeout) 
     char **targets;
     int timeout;
{
  char *buffer, **b, *d;
  int maxlen = 0;
  char c;
  int ret;
  char s[256];

  buffer = NULL;

  if (timeout > 0) {
    signal(SIGALRM, sig_handler);
    alarm(timeout);
  }
  if (setjmp(jb) == 0) {
    for (b=targets; *b; b++) {
      register int l = strlen(*b);
      maxlen = (maxlen < l) ? l : maxlen;
    }
    
    buffer = (char*) malloc (maxlen);
    fflush(stdout);
    b = NULL;
    while (read(s_device, &c, 1)) {
      if (maxlen > 1)
        bcopy(buffer+1, buffer, maxlen-1);
      buffer[maxlen-1] = c;
      for (b=targets; *b; b++) {
        if (strncmp(buffer+maxlen-strlen(*b), *b, strlen(*b)) == 0)
          goto found_it;
      }
      b = NULL;
    }
  } else {
    sprintf(s, "waitfor timed out (%ds)", timeout);
    DEBUG(s);
    ret = -1;
    goto done_waiting;
  }
  /* this should never happen */
  if (b == NULL) {
    ret = -2;
    goto done_waiting;
  }
 found_it:
  ret = b - targets;
 done_waiting:
  signal(SIGALRM, SIG_IGN);
  alarm(0);
  if (buffer)
    free(buffer);
  sprintf(s, "waitfor got string %d, '%s'", ret, (ret>=0)?targets[ret]:"");
  DEBUG(s);
  return ret;
}

/*
   control the modem: dial a number and wait for connect, wait for an
   incoming call and answer it, hangup the phone and reset the modem
*/

int online = 0;

static char *at_prompt[] = {
  "OK",
  NULL
};

static char *dial_results[] = {
  "CONNECT",
  "BUSY",
  "NO DIALTONE",
  "NO CARRIER",
  NULL
};

static char *ring_results[] = {
  "RING", 
  NULL
};

void
init_modem()
{
  int fl=0;

  send_string("at\n");
  while (waitfor((char**)at_prompt, 2) == -1) {
    if (fl ^= 1)
      send_string("+++");
    else
      send_string("at\n");
  }
  send_string("atz\n");
  waitfor(at_prompt, 0);
}


M_RESULT 
dial(number, busy_retries) 
     char *number;
     int busy_retries;
{
  char *cmd;
  M_RESULT ret;
  int fl=0;

  cmd = (char*) malloc (strlen(number) + 6);
  sprintf(cmd, "atdt%s%c", number, 13);

  send_string("at\n");
  while (waitfor((char**)at_prompt, 2) == -1) {
    if (fl ^= 1)
      send_string("+++");
    else
      send_string("at\n");
  }

 dial_again:
  send_string(cmd);
  switch(waitfor((char**)dial_results, 120)) {
  case -1:
    ret = M_TIMEOUT;
    break;
  case 0:
    ret = M_CONNECTED;
    break;
  case 1:
    if (--busy_retries > 0)
      goto dial_again;
    ret = M_BUSY;
    break;
  case 2:
  case 3:
    ret = M_FAIL;
  }
  free(cmd);
  return ret;
}

void 
hangup() 
{
  int fl=0;
  int e;

  send_string("at\n");
  while ((e=waitfor((char**)at_prompt, 2)) == -1) {
    if (fl ^= 1)
      send_string("+++");
    else
      send_string("at\n");
  }
  if (e == -2)
    return;
  send_string("ath\n");
  waitfor((char**)at_prompt, 0);
  init_modem();
}

void 
answer_phone() 
{
  while (1) {
    waitfor((char**)ring_results, 0);
    send_string("ata\n");
    if (waitfor((char**)dial_results, 120) == 0)
      return;
  }
}
