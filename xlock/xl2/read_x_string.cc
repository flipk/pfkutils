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

#include "xl2.h"

int
read_x_string(xl2_screen &s, std::string &str)
{
    str.clear();
    
    return 0;
}

#if 0
#define PASSLENGTH 20

int
ReadXString(char *s, int slen)
{
    XEvent      event;
    char        keystr[20];
    char        c;
    int         i;
    int         bp;
    int         len;
    char        pwbuf[PASSLENGTH];

    // init(?);
    bp = 0;
    *s = 0;
    while (True) {
        unsigned long lasteventtime = seconds();
        while (!XPending(s.dsp)) {
            // draw(?);
            XFlush(s.dsp);
            usleep(delay);
            if (seconds() - lasteventtime > timeout) {
                return 1;
            }
        }
        XNextEvent(s.dsp, &event);
        switch (event.type) {
        case KeyPress:
            len = XLookupString((XKeyEvent *) & event, keystr, 20, NULL, NULL);
            for (i = 0; i < len; i++) {
                c = keystr[i];
                switch (c) {
                case 8: /* ^H */
                case 127:   /* DEL */
                    if (bp > 0)
                        bp--;
                    break;
                case 10:    /* ^J */
                case 13:    /* ^M */
                    s[bp] = '\0';
                    return 0;
                case 21:    /* ^U */
                    bp = 0;
                    break;
                default:
                    s[bp] = c;
                    if (bp < slen - 1)
                        bp++;
                    else
                        XSync(s.dsp, True);   /* flush input buffer */
                }
            }
            XSetForeground(s.dsp, s.gc, s.bgcol);
            if (echokeys) {
                memset(pwbuf, '?', slen);
                XFillRectangle(s.dsp, s.win, s.gc,
                               passx, passy - font->ascent,
                               XTextWidth(font, pwbuf, slen),
                               font->ascent + font->descent);
                XDrawString(s.dsp, s.win, s.textgc,
                            passx, passy, pwbuf, bp);
            }
            /*
             * eat all events if there are more than enough pending... this
             * keeps the Xlib event buffer from growing larger than all
             * available memory and crashing xlock.
             */
            if (XPending(s.dsp) > 100) {  /* 100 is arbitrarily big enough */
                register Status status;
                do {
                    status = XCheckMaskEvent(s.dsp,
                                             KeyPressMask | KeyReleaseMask, &event);
                } while (status);
                XBell(s.dsp, 100);
            }
            break;

        case ButtonPress:
            if (((XButtonEvent *) & event)->window == s.icon) {
                return 1;
            }
            break;

        case VisibilityNotify:
            if (event.xvisibility.state != VisibilityUnobscured) {
#ifndef DEBUG
                XRaiseWindow(s.dsp, s.win);
#endif
                s[0] = '\0';
                return 1;
            }
            break;

        case KeymapNotify:
        case KeyRelease:
        case ButtonRelease:
        case MotionNotify:
        case LeaveNotify:
        case EnterNotify:
            break;

        default:
            fprintf(stderr, "%s: unexpected event: %d\n",
                    ProgramName, event.type);
            break;
        }
    }
}


static int
getPassword()
{
    char        buffer[PASSLENGTH];
    char        userpass[PASSLENGTH];
    char        rootpass[PASSLENGTH];
    char       *user;

    XWindowAttributes xgwa;
    int         y, left, done;
    struct passwd *pw;

    pw = getpwnam("root");
    strcpy(rootpass, pw->pw_passwd);

    pw = getpwuid (getuid());
    strcpy(userpass, pw->pw_passwd);

    user = pw->pw_name;

    XGetWindowAttributes(dsp, win[screen], &xgwa);

    XChangeGrabbedCursor(XCreateFontCursor(dsp, XC_left_ptr));

    XSetForeground(dsp, Scr[screen].gc, bgcol[screen]);
    XFillRectangle(dsp, win[screen], Scr[screen].gc,
                   0, 0, xgwa.width, xgwa.height);

    XMapWindow(dsp, icon[screen]);
    XRaiseWindow(dsp, icon[screen]);

    left = iconx[screen] + ICONW + font->max_bounds.width;
    y = icony[screen] + font->ascent;

    XDrawString(dsp, win[screen], textgc[screen],
                left, y, text_name, strlen(text_name));
    XDrawString(dsp, win[screen], textgc[screen],
                left + 1, y, text_name, strlen(text_name));
    XDrawString(dsp, win[screen], textgc[screen],
                left + XTextWidth(font, text_name, strlen(text_name)), y,
                user, strlen(user));

    y += font->ascent + font->descent + 2;
    XDrawString(dsp, win[screen], textgc[screen],
                left, y, text_pass, strlen(text_pass));
    XDrawString(dsp, win[screen], textgc[screen],
                left + 1, y, text_pass, strlen(text_pass));

    passx = left + 1 + XTextWidth(font, text_pass, strlen(text_pass))
        + XTextWidth(font, " ", 1);
    passy = y;

    y = icony[screen] + ICONH + font->ascent + 2;
    XDrawString(dsp, win[screen], textgc[screen],
                iconx[screen], y, text_info, strlen(text_info));

    XFlush(dsp);

    y += font->ascent + font->descent + 2;

    done = False;
    while (!done) {
        if (ReadXString(buffer, PASSLENGTH))
            break;

        if ( strcmp( buffer, my_password_string ) == 0 )
            done = True;
        else
            done = False;

        XSetForeground(dsp, Scr[screen].gc, bgcol[screen]);

        XFillRectangle(dsp, win[screen], Scr[screen].gc,
                       iconx[screen], y - font->ascent,
                       XTextWidth(font, text_invalid, strlen(text_invalid)),
                       font->ascent + font->descent + 2);

        XDrawString(dsp, win[screen], textgc[screen],
                    iconx[screen], y, text_valid, strlen(text_valid));

        if (done)
            return 0;
        else {
            XSync(dsp, True);   /* flush input buffer */
            sleep(1);
            XFillRectangle(dsp, win[screen], Scr[screen].gc,
                           iconx[screen], y - font->ascent,
                           XTextWidth(font, text_valid, strlen(text_valid)),
                           font->ascent + font->descent + 2);
            XDrawString(dsp, win[screen], textgc[screen],
                        iconx[screen], y, text_invalid, strlen(text_invalid));
            if (echokeys)   /* erase old echo */
                XFillRectangle(dsp, win[screen], Scr[screen].gc,
                               passx, passy - font->ascent,
                               xgwa.width - passx,
                               font->ascent + font->descent);
        }
    }
    XChangeGrabbedCursor(mycursor);
    XUnmapWindow(dsp, icon[screen]);
    return 1;
}

#define MAX_TIME 15




#endif
