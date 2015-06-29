#if 0
set -e -x
g++ sessionXrandr.cc -lXrandr -lX11 -o sxr
exit 0
#endif

#include <X11/extensions/Xrandr.h>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include "sessionManager.h"

int s_randr_event_type = 0;
int randr_error_base;

extern "C" int
pfkSessionXrandr_main()
{
    Display * disp;
    Window rootWindow = None;
    Drawable win;
    bool done = false;
    int randr_mask;
    int firstScreen = 0;
    union { int i; unsigned int ui; } ignore;
    pid_t pid;

    char * pidVar = getenv(PFK_SESS_MGR_ENV_VAR_NAME);
    if (pidVar == NULL)
    {
        std::cerr << "error: should be invoked from within pfkSessionMgr\n";
        return 1;
    }
    pid = strtol(pidVar, NULL, 10);

    disp = XOpenDisplay(NULL);

    XRRQueryExtension(disp, &s_randr_event_type, &randr_error_base);

    win = RootWindow(disp, firstScreen);

    XGetGeometry(disp, win, &rootWindow,
                 &ignore.i, &ignore.i,
                 &ignore.ui, &ignore.ui,
                 &ignore.ui, &ignore.ui);

    randr_mask = RRScreenChangeNotifyMask;
# ifdef RRCrtcChangeNotifyMask
    randr_mask |= RRCrtcChangeNotifyMask;
# endif
# ifdef RROutputChangeNotifyMask
    randr_mask |= RROutputChangeNotifyMask;
# endif
# ifdef RROutputPropertyNotifyMask
    randr_mask |= RROutputPropertyNotifyMask;
# endif

    XRRSelectInput(disp, rootWindow, randr_mask);

    while (!done)
    {
        XEvent e;
        XNextEvent(disp, &e);
        if (e.type == s_randr_event_type)
        {
            std::cout << "XRANDR event!\n";
            sleep(1);
            kill(pid, PFK_SESS_MGR_RESTART_SIG);
        }
    }

    return 0;
}
