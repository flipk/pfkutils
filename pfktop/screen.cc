
#include "screen.h"
#include <iostream>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <signal.h>

using namespace pfktop;
using namespace std;

//static
Screen * Screen::instance = NULL;

Screen :: Screen(void)
{
    if (!isatty(0))
        return;

    home = " [H";  // move cursor to home.
    erase = " [J"; // erase to end of screen.
    home[0] = 27;
    erase[0] = 27;
    nl = "\r\n [K"; // newline, erase to end of line.
    nl[2] = 27;

#if 1
    struct termios  new_tios;

    if (tcgetattr(0, &old_tios) < 0)
    {
        char * errstring = strerror(errno);
        cerr << "failed to get termios: " << errstring << nl;
        return;
    }
    new_tios = old_tios;

//       raw    same as -ignbrk -brkint -ignpar -parmrk  -inpck  -istrip  -inlcr
//              -igncr  -icrnl  -ixon  -ixoff -icanon -opost -isig -iuclc -ixany
//              -imaxbel -xcase min 1 time 0

    new_tios.c_iflag &= ~(IGNBRK | BRKINT | IGNPAR | INPCK   | IXANY |
                          PARMRK | ISTRIP | INPCK  | INLCR   | IGNCR |
                          ICRNL  | IXON   | IXOFF  | IMAXBEL | IUCLC);
    new_tios.c_oflag &= ~(OPOST);
// leave out ISIG because this program needs to support ^C
    new_tios.c_lflag &= ~(XCASE | ICANON | /*ISIG |*/ ECHO);
    new_tios.c_cc[VMIN] = 1;
    new_tios.c_cc[VTIME] = 0;
    if (tcsetattr(0, TCSANOW, &new_tios) < 0)
    {
        char * errstring = strerror(errno);
        cerr << "failed to set termios: " << errstring << nl;
        return;
    }
#else
    // gross
    system("stty -echo raw");
#endif

    // a nicer output is if the whole screen
    // appears to be re-drawn at once. this forces
    // the cout to accumulate all output and not
    // spit it out until a flush().
    outbuffer.resize(8192);
    cout.rdbuf()->pubsetbuf((char*)outbuffer.c_str(),
                            outbuffer.size());

    fds[0] = fds[1] = -1;

    instance = this;
    started = false;
}

Screen :: ~Screen(void)
{
    if (started)
        stop_winch();
    instance = NULL;

#if 1
    if (tcsetattr(0, TCSANOW, &old_tios) < 0)
    {
        char * errstring = strerror(errno);
        cerr << "failed to set termios: " << errstring << nl;
        return;
    }
#else
    // gross
    system("stty echo cooked");
#endif

    if (fds[0] != -1)
        close(fds[0]);
    if (fds[1] != -1)
        close(fds[1]);

    instance = NULL;
}

int
Screen :: height(void) const
{
    if (!isatty(0))
        return 24; // hardcode

    struct winsize sz;
    if (ioctl(0, TIOCGWINSZ, &sz) < 0)
    {
        char * errstring = strerror(errno);
        cerr << "failed to get winsz: " << errstring << nl;
        return -1;
    }

    return sz.ws_row;
}

//static
void
Screen :: sigwinch_handler(int sig)
{
    int c = 1;
    (void) write(instance->fds[1], &c, 1);
}

int
Screen :: start_winch(void)
{
    pipe(fds);
    struct sigaction act;
    act.sa_handler = &Screen::sigwinch_handler;
    sigfillset(&act.sa_mask);
    act.sa_flags = SA_RESTART;
    sigaction(SIGWINCH, &act, NULL);
    started = true;
    return fds[0];
}

void
Screen :: stop_winch(void)
{
    struct sigaction act;
    act.sa_handler = SIG_IGN;
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_RESTART;
    sigaction(SIGWINCH, &act, NULL);
    close(fds[1]);
    fds[1] = -1;
    started = false;
}
