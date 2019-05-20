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

/*-
 * xlock.c - X11 client to lock a display and show a screen saver.
 *
 * 4 Feb 2019 - modified by pfk@pfk.org to port to C++ classes in XLOCK2
 *
 * Copyright (c) 1988-91 by Patrick J. Naughton.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation.
 *
 * This file is provided AS IS with no warranties of any kind.  The author
 * shall have no liability with respect to the infringement of copyrights,
 * trade secrets or any patents by this file or any part thereof.  In no
 * event will the author be liable for any lost revenue or profits or
 * other special, indirect and consequential damages.
 */

#include "xl2.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <setjmp.h>
#include <stdio.h>

#include <iostream>
#include <vector>

#include "signal_backtrace.h"

using namespace std;

#define AllPointerEventMask \
    (ButtonPressMask | ButtonReleaseMask | \
    EnterWindowMask | LeaveWindowMask | \
    PointerMotionMask | PointerMotionHintMask | \
    Button1MotionMask | Button2MotionMask | \
    Button3MotionMask | Button4MotionMask | \
    Button5MotionMask | ButtonMotionMask | \
    KeymapStateMask)

class xl2_main
{
    bool debug;
    char * display_str;
    const char * password_string;
    Display * dsp;
    const char *fontname;
    const char *foreground;
    const char *background;
    const char *text_info;
    XFontStruct * font;
    XHostAddress *XHosts;
    int  HostAccessCount;
    Bool HostAccessState;
    Cursor mycursor;
    int num_screens;
    char no_bits[1];
    vector<xl2_screen> screens;
    bool got_signal;
    bool _ok;
    jmp_buf  jmp_env;

    static const int default_idle_time = 5; // 600
    static const int default_grace_period = 5;
    static const int default_saver_period = 5; // 600
    static const int default_passwd_period = 30;

    int idle_time;     // seconds of idle before locker activates
    int grace_period;  // seconds after activation for no password
    int saver_period;  // seconds after lock before power save
    int passwd_period; // seconds passwd prompt is shown

    enum {
        STATE_NOT_IDLE,
        STATE_GRACE_PERIOD,
        STATE_LOCKED,
        STATE_OFF
    } state;
    int seconds_in_state;

void
hsb2rgb(double  H, double  S, double  B,
        u_char *r, u_char *g, u_char *b)
{
    int         i;
    double      f;
    double      bb;
    u_char      p;
    u_char      q;
    u_char      t;

    H -= floor(H);		/* remove anything over 1 */
    H *= 6.0;
    i = floor(H);		/* 0..5 */
    f = H - (float) i;		/* f = fractional part of H */
    bb = 255.0 * B;
    p = (u_char) (bb * (1.0 - S));
    q = (u_char) (bb * (1.0 - (S * f)));
    t = (u_char) (bb * (1.0 - (S * (1.0 - f))));
    switch (i) {
    case 0:
	*r = (u_char) bb;
	*g = t;
	*b = p;
	break;
    case 1:
	*r = q;
	*g = (u_char) bb;
	*b = p;
	break;
    case 2:
	*r = p;
	*g = (u_char) bb;
	*b = t;
	break;
    case 3:
	*r = p;
	*g = q;
	*b = (u_char) bb;
	break;
    case 4:
	*r = t;
	*g = p;
	*b = (u_char) bb;
	break;
    case 5:
	*r = (u_char) bb;
	*g = p;
	*b = q;
	break;
    }
}

void
hsbramp(double h1, double s1, double b1,
        double h2, double s2, double b2,
        int count,
        u_char *red, u_char *green, u_char *blue)
{
    double dh = (h2 - h1) / count;
    double ds = (s2 - s1) / count;
    double db = (b2 - b1) / count;
    while (count--)
    {
	hsb2rgb(h1, s1, b1, red++, green++, blue++);
	h1 += dh;
	s1 += ds;
	b1 += db;
    }
}

long
allocpixel(Colormap cmap, const char *name, const char *def)
{
    XColor col, tmp;
    XParseColor(dsp, cmap, name, &col);
    if (!XAllocColor(dsp, cmap, &col))
    {
        cerr << "couldn't allocate " << name
             << "using " << def << " instead\n";
        XAllocNamedColor(dsp, cmap, def, &col, &tmp);
    }
    return col.pixel;
}

public:

bool ok(void) const
{
    return _ok;
}

xl2_main(int argc, char ** argv, bool _debug)
{
    XSetWindowAttributes xswa;
    XGCValues   xgcv;
    XColor      nullcolor;

    debug = _debug;
    _ok = false;
    dsp = NULL;
    font = NULL;
    fontname = DEFAULT_FONT;
    num_screens = 0;
    XHosts = NULL;
    foreground = DEF_FG;
    background = DEF_BG;

    text_info = "Enter password to unlock; select icon to lock.";

    display_str = getenv("DISPLAY");
    if (!display_str)
    {
        cerr << "DISPLAY not set\n";
        return;
    }

    dsp = XOpenDisplay(display_str);
    if (!dsp)
    {
        cerr << "Cannot open DISPLAY " << display_str << "\n";
        return;
    }

    password_string = getenv("XLOCK_PASSWORD");
    if (!password_string)
    {
        cerr << "XLOCK_PASSWORD not set\n";
        password_string = "one";
    }

    font = XLoadQueryFont(dsp, fontname);
    if (!font)
    {
        cerr << "cannot load font " << fontname
             << ", falling back to " << FALLBACK_FONT << "\n";
        font = XLoadQueryFont(dsp, FALLBACK_FONT);
        if (!font)
        {
            cerr << "cannot load fallback!\n";
            return;
        }
    }

    num_screens = ScreenCount(dsp);
    screens.resize(num_screens);
    for (int screen = 0; screen < num_screens; screen++)
    {
        xl2_screen &s = screens[screen];
        s.screen = screen;
        s.dsp = dsp;
        s.scr = ScreenOfDisplay(dsp, screen);
        Colormap cmap = DefaultColormapOfScreen(s.scr);

        s.root = RootWindowOfScreen(s.scr);
        s.fgcol = allocpixel(cmap, foreground, "Black");
        s.bgcol = allocpixel(cmap, background, "White");

        int         colorcount = xl2_screen::NUMCOLORS;
        u_char      red[xl2_screen::NUMCOLORS];
        u_char      green[xl2_screen::NUMCOLORS];
        u_char      blue[xl2_screen::NUMCOLORS];
        int         i;

        float saturation = 1.0;
        hsbramp(0.0, saturation, 1.0, 1.0, saturation, 1.0, colorcount,
                red, green, blue);

        s.npixels = 0;
        for (i = 0; i < colorcount; i++)
        {
            XColor      xcolor;

            xcolor.red = red[i] << 8;
            xcolor.green = green[i] << 8;
            xcolor.blue = blue[i] << 8;
            xcolor.flags = DoRed | DoGreen | DoBlue;

            if (!XAllocColor(dsp, cmap, &xcolor))
                break;

            s.pixels[s.npixels++] = xcolor.pixel;
        }

        xswa.override_redirect = True;
        xswa.background_pixel = BlackPixelOfScreen(s.scr);
        xswa.event_mask =
            KeyPressMask | ButtonPressMask | VisibilityChangeMask;

        int LEFT = 0, TOP = 0;
        unsigned int WIDTH = WidthOfScreen(s.scr);
        unsigned int HEIGHT = HeightOfScreen(s.scr);
        unsigned int ICONW = 64;
        unsigned int ICONH = 64;
        unsigned long CWMASK = 0;

        if (debug)
        {
            LEFT = TOP = 100;
            WIDTH -= 200;
            HEIGHT -= 200;
            CWMASK = CWBackPixel | CWEventMask;
        }
        else
        {
            CWMASK = CWOverrideRedirect | CWBackPixel | CWEventMask;
        }

        s.win = XCreateWindow(
            dsp, s.root, LEFT, TOP, WIDTH, HEIGHT, 0,
            CopyFromParent, InputOutput, CopyFromParent,
            CWMASK, &xswa);

        s.iconx = (DisplayWidth(dsp, screen) -
                         XTextWidth(font, text_info,
                                    strlen(text_info))) / 2;

        s.icony = DisplayHeight(dsp, screen) / 6;

        xswa.border_pixel = s.fgcol;
        xswa.background_pixel = s.bgcol;
        xswa.event_mask = ButtonPressMask;

        s.icon = XCreateWindow(dsp, s.win, s.iconx, s.icony,
                                ICONW, ICONH, 1, CopyFromParent,
                                InputOutput, CopyFromParent,
                                CWBorderPixel | CWBackPixel | CWEventMask,
                                &xswa);

        xgcv.foreground = WhitePixelOfScreen(s.scr);
        xgcv.background = BlackPixelOfScreen(s.scr);

        s.gc = XCreateGC(dsp, s.win,
                          GCForeground | GCBackground, &xgcv);

        xgcv.foreground = s.fgcol;
        xgcv.background = s.bgcol;
        xgcv.font = font->fid;

        s.textgc = XCreateGC(dsp, s.win,
                              GCFont | GCForeground | GCBackground,
                              &xgcv);
    }

    no_bits[0] = 0;
    Pixmap lockc = XCreateBitmapFromData(dsp, screens[0].root, no_bits, 1, 1);
    Pixmap lockm = XCreateBitmapFromData(dsp, screens[0].root, no_bits, 1, 1);
    mycursor = XCreatePixmapCursor(dsp, lockc, lockm,
                                   &nullcolor, &nullcolor, 0, 0);
    XFreePixmap(dsp, lockc);
    XFreePixmap(dsp, lockm);

    if (!debug)
    {
        XSetScreenSaver(dsp, DisableScreenSaver, DisableScreenInterval,
                        DontPreferBlanking, AllowExposures);
    }

    got_signal = false;
    BackTraceUtil::SignalBacktrace::get_instance()->
        register_handler(PROGRAM_NAME, (void*) this,
                         &xl2_main::signal_handler);

    // xxx getenvs
    idle_time = default_idle_time;
    grace_period = default_grace_period;
    saver_period = default_saver_period;
    passwd_period = default_passwd_period;

    state = STATE_NOT_IDLE;
    seconds_in_state = 0;

    _ok = true;
}

private:

static void
signal_handler(void *arg,
               const BackTraceUtil::SignalBacktraceInfo *info)
{
    printf("\n%s\n", info->description);

    xl2_main * m = (xl2_main *) arg;
    if (!m)
        exit(1);

    if (info->fatal)
        longjmp(m->jmp_env, 1);
    else
        m->got_signal = true;
}

void
activate(void)
{
    for (int screen = 0; screen < num_screens; screen++)
    {
        xl2_screen &s = screens[screen];
        XMapWindow(dsp, s.win);
        XRaiseWindow(dsp, s.win);
    }

    if (!debug)
    {
        Status      status;

        // try grabbing multiple times, because the window manager
        // sometimes grabs while decorating, or processing menu events.
        do {
            usleep(10000);
            status = XGrabKeyboard(dsp, screens[0].win, True,
                                   GrabModeAsync, GrabModeAsync,
                                   CurrentTime);
        } while (status != GrabSuccess);

        do {
            usleep(10000);
            status = XGrabPointer(dsp, screens[0].win, True,
                                  AllPointerEventMask,
                                  GrabModeAsync, GrabModeAsync,
                                  None, mycursor,
                                  CurrentTime);
        } while (status != GrabSuccess);

        XHosts = XListHosts(dsp, &HostAccessCount, &HostAccessState);
        if (XHosts)
            XRemoveHosts(dsp, XHosts, HostAccessCount);
        XEnableAccessControl(dsp);
    }

    XSync(dsp, False);
}

void
deactivate(void)
{
    for (int screen = 0; screen < num_screens; screen++)
    {
        xl2_screen &s = screens[screen];
        XUnmapWindow(dsp, s.win);
    }

    XSync(dsp, False);

    if (!debug)
    {
        if (XHosts)
        {
            XAddHosts(dsp, XHosts, HostAccessCount);
            XFree((char *) XHosts);
            XHosts = NULL;
        }
        if (HostAccessState == False)
            XDisableAccessControl(dsp);

        XUngrabPointer(dsp, CurrentTime);
        XUngrabKeyboard(dsp, CurrentTime);
    }

    XFlush(dsp);
}

public:

~xl2_main(void)
{
    BackTraceUtil::SignalBacktrace::get_instance()->cleanup();
    if (XHosts)
        XFree((char*) XHosts);
    if (font)
        XFreeFont(dsp, font);
    if (dsp)
    {
        XSetScreenSaver(dsp, DisableScreenSaver,
                        DisableScreenInterval, DontPreferBlanking,
                        AllowExposures);
        XCloseDisplay(dsp);
    }
}

public:

int
main(void)
{
    if (setjmp(jmp_env) != 0)
        exit(1); // immediately!

    activate();

    bool done = false;
    while (!done)
    {
        LockProc * lp = LockProc::make_random(screens[0]);

        bool done2 = false;
        time_t started = time(NULL);
        while (!done2)
        {
            lp->draw();
            XFlush(dsp);
            usleep(10000);

            while (XPending(dsp))
            {
                XEvent event;
                XNextEvent(dsp, &event);
                if (event.type == VisibilityNotify)
                    XRaiseWindow(dsp, event.xany.window);
                if (event.type == ButtonPress  ||
                    event.type == KeyPress)
                {
                    done2 = done = true;
                }
            }

            time_t now = time(NULL);
            if ((now - started) > 2)
                done2 = true;

            if (got_signal)
                break;
        }
        if (got_signal)
            break;

        delete lp;
    }

    deactivate();

    return 0;
}

};

int
main(int argc, char ** argv)
{
    bool debug = false;

    if (argc > 1 && argv[1][0] == 'd')
        debug = true;

    srandom(getpid() * time(NULL));

    xl2_main  xl2(argc, argv, debug);

    if (xl2.ok() == true)
        return xl2.main();

    return 1;
}
