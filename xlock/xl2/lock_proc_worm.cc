/*-
 * worm.c - draw wiggly worms.
 *
 * 4 Feb 2019 - modified by pfk@pfk.org to port to C++ classes in XLOCK2
 *
 * Copyright (c) 1991 by Patrick J. Naughton.
 *
 * See xlock.c for copying information.
 *
 * Revision History:
 * 27-Sep-91: got rid of all malloc calls since there were no calls to free().
 * 25-Sep-91: Integrated into X11R5 contrib xlock.
 *
 * Adapted from a concept in the Dec 87 issue of Scientific American.
 *
 * SunView version: Brad Taylor (brad@sun.com)
 * X11 version: Dave Lemke (lemke@ncd.com)
 * xlock version: Boris Putanec (bp@cs.brown.edu)
 *
 * This code is a static memory pig... like almost 200K... but as contributed
 * it leaked at a massive rate, so I made everything static up front... feel
 * free to contribute the proper memory management code.
 * 
 */

#include "xl2.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

class WormLockProc : public LockProc {

    static const int MAXCOLORS = 64;
    static const int MAXWORMS = 64;
    static const int CIRCSIZE = 2;
    static const int MAXWORMLEN = 50;
    static const int SEGMENTS = 36;
    static const int batchcount = 100;

    struct wormstuff {
        int         xcirc[MAXWORMLEN];
        int         ycirc[MAXWORMLEN];
        int         dir;
        int         tail;
        int         x;
        int         y;
    };

    int  sintab[SEGMENTS];
    int  costab[SEGMENTS];
    int         xsize;
    int         ysize;
    int         wormlength;
    int         nc;
    int         nw;
    wormstuff   worm[MAXWORMS];
    XRectangle  rects[MAXCOLORS][MAXWORMS];
    int         size[MAXCOLORS];

    int myround(float x) { return (int) rint((double)x); }

void
initworm(void)
{
    int         i, j;
    XWindowAttributes xwa;

    XClearWindow(s.dsp, s.win);

    for (i = 0; i < SEGMENTS; i++) {
        sintab[i] = myround(CIRCSIZE * sin(i * 2 * M_PI / SEGMENTS));
        costab[i] = myround(CIRCSIZE * cos(i * 2 * M_PI / SEGMENTS));
    }

    nc = s.npixels;
    if (nc > MAXCOLORS)
        nc = MAXCOLORS;

    nw = batchcount;
    if (nw > MAXWORMS)
        nw = MAXWORMS;

    XGetWindowAttributes(s.dsp, s.win, &xwa);
    xsize = xwa.width;
    ysize = xwa.height;

    if (xwa.width < 100) {
        wormlength = MAXWORMLEN / 10;
    } else {
        wormlength = MAXWORMLEN;
    }

    for (i = 0; i < nc; i++) {
        for (j = 0; j < nw / nc + 1; j++) {
            rects[i][j].width = CIRCSIZE;
            rects[i][j].height = CIRCSIZE;
        }
    }
    memset(size, 0, nc * sizeof(int));

    for (i = 0; i < nw; i++) {
        for (j = 0; j < wormlength; j++) {
            worm[i].xcirc[j] = xsize / 2;
            worm[i].ycirc[j] = ysize / 2;
        }
        worm[i].dir = (unsigned) random() % SEGMENTS;
        worm[i].tail = 0;
        worm[i].x = xsize / 2;
        worm[i].y = ysize / 2;
    }
}

void
worm_doit(int which, unsigned long color)
{
    wormstuff  *ws = &worm[which];
    int         x, y;

    ws->tail++;
    if (ws->tail == wormlength)
        ws->tail = 0;

    x = ws->xcirc[ws->tail];
    y = ws->ycirc[ws->tail];
    XClearArea(s.dsp, s.win, x, y, CIRCSIZE, CIRCSIZE, False);

    if (random() & 1) {
        ws->dir = (ws->dir + 1) % SEGMENTS;
    } else {
        ws->dir = (ws->dir + SEGMENTS - 1) % SEGMENTS;
    }

    x = (ws->x + costab[ws->dir] + xsize) % xsize;
    y = (ws->y + sintab[ws->dir] + ysize) % ysize;

    ws->xcirc[ws->tail] = x;
    ws->ycirc[ws->tail] = y;
    ws->x = x;
    ws->y = y;

    rects[color][size[color]].x = x;
    rects[color][size[color]].y = y;
    size[color]++;
}

void
drawworm(void)
{
    int         i;
    unsigned int wcolor;
    static unsigned int chromo = 0;

    memset(size, 0, nc * sizeof(int));

    for (i = 0; i < nw; i++) {
        wcolor = (i + chromo) % nc;
        worm_doit(i, wcolor);
    }


    for (i = 0; i < nc; i++) {
        XSetForeground(s.dsp, s.gc, s.pixels[i]);
        XFillRectangles(s.dsp, s.win, s.gc, rects[i],
                        size[i]);
    }

    if (++chromo == nc)
        chromo = 0;
}

public:

WormLockProc(xl2_screen &_s) : LockProc(_s)
{
    initworm();
}
virtual ~WormLockProc(void)
{

}
virtual void draw(void)
{
    drawworm();
}
};

class WormLockProcFactory : public LockProcFactory
{
public:
    WormLockProcFactory(void) : LockProcFactory("worm") { }
    virtual LockProc * make(xl2_screen &_s)
    {
        return new WormLockProc(_s);
    };
};

static WormLockProcFactory flame_factory;
