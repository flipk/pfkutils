/*-
 * flame.c - recursive fractal cosmic flames.
 *
 * 4 Feb 2019 - modified by pfk@pfk.org to port to C++ classes in XLOCK2
 *
 * Copyright (c) 1991 by Patrick J. Naughton.
 *
 * See xlock.c for copying information.
 *
 * Revision History:
 * 27-Jun-91: vary number of functions used.
 * 24-Jun-91: fixed portability problem with integer mod (%).
 * 06-Jun-91: Written. (received from Scott Graves, spot@cs.cmu.edu).
 */

#include "xl2.h"
#include <stdlib.h>
#include <math.h>

class FlameLockProc : public LockProc {

    static const int MAXTOTAL = 10000;
    static const int MAXBATCH = 10;
    static const int MAXLEV = 4;
    static const int batchcount = 100;

    double      f[2][3][MAXLEV];/* three non-homogeneous transforms */
    int         max_levels;
    int         cur_level;
    int         snum;
    int         anum;
    int         width, height;
    int         num_points;
    int         total_points;
    int         pixcol;
    XPoint      pts[MAXBATCH];

static short halfrandom(int mv)
{
    static short lasthalf = 0;
    unsigned long r;

    if (lasthalf) {
        r = lasthalf;
        lasthalf = 0;
    } else {
        r = random();
        lasthalf = r >> 16;
    }
    return r % mv;
}

void
initflame(void)
{
    XWindowAttributes xwa;

    XGetWindowAttributes(s.dsp, s.win, &xwa);
    width = xwa.width;
    height = xwa.height;

    max_levels = batchcount;
    cur_level = 0;

    XSetForeground(s.dsp, s.gc, BlackPixel(s.dsp, s.screen));
    XFillRectangle(s.dsp, s.win, s.gc, 0, 0, width, height);

    if (s.npixels > 2) {
        pixcol = halfrandom(s.npixels);
        XSetForeground(s.dsp, s.gc, s.pixels[pixcol]);
    } else {
        XSetForeground(s.dsp, s.gc, WhitePixel(s.dsp, s.screen));
    }
}

Bool
recurse(double x, double y, int l)
{
    int         xp, yp, i;
    double      nx, ny;

    if (l == max_levels) {
        total_points++;
        if (total_points > MAXTOTAL)    /* how long each fractal runs */
            return False;

        if (x > -1.0 && x < 1.0 && y > -1.0 && y < 1.0) {
            xp = pts[num_points].x = (int) ((width / 2)
                                                    * (x + 1.0));
            yp = pts[num_points].y = (int) ((height / 2)
                                                    * (y + 1.0));
            num_points++;
            if (num_points > MAXBATCH) {    /* point buffer size */
                XDrawPoints(s.dsp, s.win, s.gc, pts,
                            num_points, CoordModeOrigin);
                num_points = 0;
            }
        }
    } else {
        for (i = 0; i < snum; i++) {
            nx = f[0][0][i] * x + f[0][1][i] * y + f[0][2][i];
            ny = f[1][0][i] * x + f[1][1][i] * y + f[1][2][i];
            if (i < anum) {
                nx = sin(nx);
                ny = sin(ny);
            }
            if (!recurse(nx, ny, l + 1))
                return False;
        }
    }
    return True;
}

void
drawflame(void)
{
    int         i, j, k;
    static   int    alt = 0;

    if (!(cur_level++ % max_levels)) {
        XClearWindow(s.dsp, s.win);
        alt = !alt;
    } else {
        if (s.npixels > 2) {
            XSetForeground(s.dsp, s.gc,
                           s.pixels[pixcol]);
            if (--pixcol < 0)
                pixcol = s.npixels - 1;
        }
    }

    /* number of functions */
    snum = 2 + (cur_level % (MAXLEV - 1));

    /* how many of them are of alternate form */
    if (alt)
        anum = 0;
    else
        anum = halfrandom(snum) + 2;

    /* 6 coefs per function */
    for (k = 0; k < snum; k++) {
        for (i = 0; i < 2; i++)
            for (j = 0; j < 3; j++)
                f[i][j][k] = ((double) (random() & 1023) / 512.0 - 1.0);
    }

    num_points = 0;
    total_points = 0;
    (void) recurse(0.0, 0.0, 0);
    XDrawPoints(s.dsp, s.win, s.gc,
                pts, num_points, CoordModeOrigin);
}

public:

FlameLockProc(xl2_screen &_s) : LockProc(_s)
{
    initflame();
}

virtual ~FlameLockProc(void)
{
}

virtual void draw(void)
{
    drawflame();
}

};

class FlameLockProcFactory : public LockProcFactory
{
public:
    FlameLockProcFactory(void) : LockProcFactory("flame") { }
    virtual LockProc * make(xl2_screen &_s)
    {
        return new FlameLockProc(_s);
    };
};

static FlameLockProcFactory flame_factory;
