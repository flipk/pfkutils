#include <setjmp.h>

typedef enum { M_BUSY, M_CONNECTED, M_TIMEOUT, M_FAIL } M_RESULT;
extern char *device;  /* set this before beginning to show what device to use*/
extern int s_device;
extern int online;           /* flag that answers the q: "are we online?" */

int waitfor(char **targets, int timeout);   /* wait for one of the target
                                               strings within timeout 
                                               period */

void open_device();    /* opens device */
void init_all();       /* initializes clocal, -icanon, echo, etc */
void init_clocal();    /* set clocal mode */
void init_no_clocal(); /* turn off clocal (so we get a HUP when hangs up) */
void init_icanon();    /* string entry mode (for usernames and such */
void init_no_icanon(); /* char-by-char mode */
void init_echo();      /* echo the chars? */
void init_no_echo();   /* for password prompts and stuff */
void send_string(char *);  /* send a string to modem */
char *read_string(int);    /* read a string (newline terminated) */
void init_modem();           /* initialize the modem (send ATZ) */
M_RESULT dial(char *, int);  /* dial the phone, has the four above responses */
void hangup();               /* hang the phone up (also does ATZ) */
void answer_phone();         /* waits for RING, sends ATA, wait for CONNECT */
