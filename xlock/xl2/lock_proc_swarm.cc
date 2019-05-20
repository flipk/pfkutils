 /*-
 * swarm.c - swarm of bees for xlock, the X Window System lockscreen.
 *
 * 4 Feb 2019 - modified by pfk@pfk.org to port to C++ classes in XLOCK2
 *
 * Copyright (c) 1991 by Patrick J. Naughton.
 *
 * Revision History:
 * 31-Aug-90: Adapted from xswarm by Jeff Butterworth. (butterwo@ncsc.org)
 */

#include "xl2.h"
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

using namespace std;

class SwarmLockProc : public LockProc {

    static const int TIMES   = 4;  /* number of time positions recorded */
    static const int BEEACC  = 3;  /* acceleration of bees */
    static const int WASPACC = 5;  /* maximum acceleration of wasp */
    static const int BEEVEL  = 11; /* maximum bee velocity */
    static const int WASPVEL = 12; /* maximum wasp velocity */
    static const int BORDER  = 50; /* wasp won't go closer than
                                      this to the edge */

    static const int batchcount = 100;

    int& X(int t, int b) { return x[t * beecount + b]; }
    int& Y(int t, int b) { return y[t * beecount + b]; }
    /* random number around 0 */
    int RAND(int v) { return ((random()%(v))-((v)/2)); }

    int         pix;
    long        startTime;
    int         width;
    int         height;
    int         beecount;       /* number of bees */
    vector<XSegment> segs;      /* bee lines */
    vector<XSegment> old_segs;  /* old bee lines */
    vector<int>      x;  /* bee positions x[time][bee#] */
    vector<int>      y;
    vector<int>      xv;
    vector<int>      yv;     /* bee velocities xv[bee#] */
    int       wx[3];
    int       wy[3];
    int       wxv;
    int       wyv;

void
initswarm(void)
{
    XWindowAttributes xgwa;
    int         b;

    startTime = (long) time(NULL);
    beecount = batchcount;

    XGetWindowAttributes(s.dsp, s.win, &xgwa);
    width = xgwa.width;
    height = xgwa.height;

    /* Clear the background. */
    XSetForeground(s.dsp, s.gc, BlackPixel(s.dsp, s.screen));
    XFillRectangle(s.dsp, s.win, s.gc, 0, 0, width, height);

    /* Allocate memory. */

    segs.resize(beecount);
    old_segs.resize(beecount);
    x.resize(beecount * TIMES);
    y.resize(beecount * TIMES);
    xv.resize(beecount);
    yv.resize(beecount);
    pix = 0;

    /* Initialize point positions, velocities, etc. */

    /* wasp */
    wx[0] = BORDER + random() % (width - 2 * BORDER);
    wy[0] = BORDER + random() % (height - 2 * BORDER);
    wx[1] = wx[0];
    wy[1] = wy[0];
    wxv = 0;
    wyv = 0;

    /* bees */
    for (b = 0; b < beecount; b++)
    {
        X(0, b) = random() % width;
        X(1, b) = X(0, b);
        Y(0, b) = random() % height;
        Y(1, b) = Y(0, b);
        xv[b] = RAND(7);
        yv[b] = RAND(7);
    }
}

void
drawswarm(void)
{
    int         b;

    /* <=- Wasp -=> */
    /* Age the arrays. */
    wx[2] = wx[1];
    wx[1] = wx[0];
    wy[2] = wy[1];
    wy[1] = wy[0];
    /* Accelerate */
    wxv += RAND(WASPACC);
    wyv += RAND(WASPACC);

    /* Speed Limit Checks */
    if (wxv > WASPVEL)
        wxv = WASPVEL;
    if (wxv < -WASPVEL)
        wxv = -WASPVEL;
    if (wyv > WASPVEL)
        wyv = WASPVEL;
    if (wyv < -WASPVEL)
        wyv = -WASPVEL;

    /* Move */
    wx[0] = wx[1] + wxv;
    wy[0] = wy[1] + wyv;

    /* Bounce Checks */
    if ((wx[0] < BORDER) || (wx[0] > width - BORDER - 1)) {
        wxv = -wxv;
        wx[0] += wxv;
    }
    if ((wy[0] < BORDER) || (wy[0] > height - BORDER - 1)) {
        wyv = -wyv;
        wy[0] += wyv;
    }
    /* Don't let things settle down. */
    xv[random() % beecount] += RAND(3);
    yv[random() % beecount] += RAND(3);

    /* <=- Bees -=> */
    for (b = 0; b < beecount; b++) {
        int         distance,
            dx,
            dy;
        /* Age the arrays. */
        X(2, b) = X(1, b);
        X(1, b) = X(0, b);
        Y(2, b) = Y(1, b);
        Y(1, b) = Y(0, b);

        /* Accelerate */
        dx = wx[1] - X(1, b);
        dy = wy[1] - Y(1, b);
        distance = abs(dx) + abs(dy);   /* approximation */
        if (distance == 0)
            distance = 1;
        xv[b] += (dx * BEEACC) / distance;
        yv[b] += (dy * BEEACC) / distance;

        /* Speed Limit Checks */
        if (xv[b] > BEEVEL)
            xv[b] = BEEVEL;
        if (xv[b] < -BEEVEL)
            xv[b] = -BEEVEL;
        if (yv[b] > BEEVEL)
            yv[b] = BEEVEL;
        if (yv[b] < -BEEVEL)
            yv[b] = -BEEVEL;

        /* Move */
        X(0, b) = X(1, b) + xv[b];
        Y(0, b) = Y(1, b) + yv[b];

        /* Fill the segment lists. */
        segs[b].x1 = X(0, b);
        segs[b].y1 = Y(0, b);
        segs[b].x2 = X(1, b);
        segs[b].y2 = Y(1, b);
        old_segs[b].x1 = X(1, b);
        old_segs[b].y1 = Y(1, b);
        old_segs[b].x2 = X(2, b);
        old_segs[b].y2 = Y(2, b);
    }

    XSetForeground(s.dsp, s.gc, BlackPixel(s.dsp, s.screen));
    XDrawLine(s.dsp, s.win, s.gc,
              wx[1], wy[1], wx[2], wy[2]);
    XDrawSegments(s.dsp, s.win, s.gc, old_segs.data(), beecount);

    XSetForeground(s.dsp, s.gc, WhitePixel(s.dsp, s.screen));
    XDrawLine(s.dsp, s.win, s.gc,
              wx[0], wy[0], wx[1], wy[1]);
    XSetForeground(s.dsp, s.gc, s.pixels[pix]);
    if (++pix >= s.npixels)
        pix = 0;
    XDrawSegments(s.dsp, s.win, s.gc, segs.data(), beecount);
}

public:

SwarmLockProc(xl2_screen &_s) : LockProc(_s)
{
    initswarm();
}
virtual ~SwarmLockProc(void)
{

}
virtual void draw(void)
{
    drawswarm();
}

};

class SwarmLockProcFactory : public LockProcFactory
{
public:
    SwarmLockProcFactory(void) : LockProcFactory("swarm") { }
    virtual LockProc * make(xl2_screen &_s)
    {
        return new SwarmLockProc(_s);
    };
};

static SwarmLockProcFactory flame_factory;
