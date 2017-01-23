/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
*/

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
    if (write(instance->fds[1], &c, 1) < 0)
        cerr << "Screen::sigwinch: write failed\n";
}

int
Screen :: start_winch(void)
{
    if (pipe(fds) < 0)
        cerr << "Screen:start_winch: pipe failed\n";
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
