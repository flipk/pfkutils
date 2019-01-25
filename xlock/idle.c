#if 0
set -e -x
gcc -D__TEST_XSS_IDLE__ -Wall idle.c -o xpi -lX11 -lXext -lXss
exit 0
#endif

#include <X11/Xlib.h>
#include <X11/extensions/dpms.h>
#include <X11/extensions/scrnsaver.h>
#include <stdio.h>
#include <inttypes.h>

uint32_t
get_idle_milliseconds(Display *d)
{
    XScreenSaverInfo *ssi;
    int evbase, errbase;

    if (!XScreenSaverQueryExtension(d, &evbase, &errbase))
    {
        fprintf(stderr, "screen saver extension not supported\n");
        return 1;
    }
  
    ssi = XScreenSaverAllocInfo();
    if (ssi == NULL)
    {
        fprintf(stderr, "couldn't allocate screen saver info\n");
        return 1;
    }   
  
    if (!XScreenSaverQueryInfo(d, DefaultRootWindow(d), ssi))
    {
        fprintf(stderr, "couldn't query screen saver info\n");
        return 1;
    }
  
    uint32_t idle = (uint32_t) ssi->idle;

    XFree(ssi);

    // ugh. ssi->idle is wrong if DPMS is enabled, we have
    // to compensate.
    if (DPMSQueryExtension(d, &evbase, &errbase))
    {
        if (DPMSCapable(d))
        {
            CARD16 standby, suspend, off, state;
            BOOL onoff;

            DPMSGetTimeouts(d, &standby, &suspend, &off);
            DPMSInfo(d, &state, &onoff);

            if (onoff)
            {
                uint32_t offset = 0;
                switch (state)
                {
                case DPMSModeStandby:
                    offset = (uint32_t) (standby * 1000);
                    break;

                case DPMSModeSuspend:
                    offset = (uint32_t) ((suspend + standby) * 1000);
                    break;

                case DPMSModeOff:
                    offset = (uint32_t) ((off + suspend + standby) * 1000);
                    break;

                default:
                    /*nothing*/;
                }
                if (offset != 0 && idle < offset)
                    idle += offset;
            }
        } 
    }

    return idle;
}

#ifdef __TEST_XSS_IDLE__
int
main()
{
    Display * d = XOpenDisplay(":0");
    if (!d)
        return 1;

    uint32_t idle = get_idle_milliseconds(d);

    XCloseDisplay(d);

    printf("idle=%u\n", idle);

    return 0;
}
#endif
