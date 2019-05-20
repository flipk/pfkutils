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

#include <X11/Xlib.h>
#include <X11/extensions/dpms.h>
#include <X11/extensions/scrnsaver.h>
#include <stdio.h>
#include <inttypes.h>

#include "xl2.h"

int
get_idle_seconds(Display *d)
{
    XScreenSaverInfo *ssi;
    int evbase, errbase;

    if (!XScreenSaverQueryExtension(d, &evbase, &errbase))
    {
        fprintf(stderr, "screen saver extension not supported\n");
        return -1;
    }
  
    ssi = XScreenSaverAllocInfo();
    if (ssi == NULL)
    {
        fprintf(stderr, "couldn't allocate screen saver info\n");
        return -1;
    }   
  
    if (!XScreenSaverQueryInfo(d, DefaultRootWindow(d), ssi))
    {
        fprintf(stderr, "couldn't query screen saver info\n");
        return -1;
    }
  
    int idle = (int) ssi->idle;

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

    return idle / 1000;
}

#ifdef __TEST_XSS_IDLE__
int
main()
{
    Display * d = XOpenDisplay(":0");
    if (!d)
        return 1;

    uint32_t idle = get_idle_seconds(d);

    XCloseDisplay(d);

    printf("idle=%u\n", idle);

    return 0;
}
#endif
