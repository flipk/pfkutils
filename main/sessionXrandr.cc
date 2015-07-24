#if 0
set -e -x
g++ -DpfkSessionXrandr_main=main sessionXrandr.cc -lpthread -lXrandr -lX11 -o sxr
exit 0
#endif

#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <stdio.h>

#include "sessionManager.h"

int s_randr_event_type = 0;
int randr_error_base;

static void * xrandr_monitor_thread(void * arg);

static int xrandr_event_occurred = 0;
static pid_t session_mgr_pid;

extern "C" int
pfkSessionXrandr_main()
{
    Display * disp;
    Window rootWindow = None;
    Drawable win;
    int randr_mask;
    int firstScreen = 0;
    union { int i; unsigned int ui; } ignore;

    char * pidVar = getenv(PFK_SESS_MGR_ENV_VAR_NAME);
    if (pidVar == NULL)
    {
        std::cerr << "error: should be invoked from within pfkSessionMgr\n";
        return 1;
    }
    session_mgr_pid = strtol(pidVar, NULL, 10);

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

    xrandr_event_occurred = 0;

    pthread_t id;
    pthread_attr_t  attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&id,  &attr, &xrandr_monitor_thread, NULL);
    pthread_attr_destroy(&attr);

    while (true)
    {
        XEvent e;
        XNextEvent(disp, &e);
        if (e.type == s_randr_event_type)
            xrandr_event_occurred = 1;
    }

    return 0;
}

static void *
xrandr_monitor_thread(void * arg)
{
    while (1)
    {
        usleep(100000);
        if (xrandr_event_occurred != 0)
        {
            while (xrandr_event_occurred != 0)
            {
                xrandr_event_occurred = 0;
                sleep(1);
            }
            kill(session_mgr_pid, PFK_SESS_MGR_RESTART_SIG);
        }
    }
}
