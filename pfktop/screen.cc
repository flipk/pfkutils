
#include "screen.h"

#include <stdlib.h>
#include <iostream>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <signal.h>

using namespace pfktop;
using namespace std;

//static
Screen * Screen::instance = NULL;

Screen :: Screen(void)
{
    if (!isatty(0))
        return;

    erase = " [H [J";
    erase[0] = erase[3] = 27;
    nl = "\r\n";

#if 0
    struct termios  tios;

    if (tcgetattr(0, &tios) < 0)
    {
        char * errstring = strerror(errno);
        cerr << "failed to get termios: " << errstring << nl;
        return -1;
    }
#else
    // gross
    system("stty -echo raw");
#endif

    string outbuffer;
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

    // gross
    system("stty echo cooked");

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
