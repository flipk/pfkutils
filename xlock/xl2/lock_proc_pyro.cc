/*-
 * pyro.c - Fireworks for xlock, the X Window System lockscreen.
 *
 * 4 Feb 2019 - modified by pfk@pfk.org to port to C++ classes in XLOCK2
 *
 * Copyright (c) 1991 by Patrick J. Naughton.
 *
 * See xlock.c for copying information.
 *
 * Revision History:
 * 16-Mar-91: Written. (received from David Brooks, brooks@osf.org).
 */

/* The physics of the rockets is a little bogus, but it looks OK.  Each is
 * given an initial velocity impetus.  They decelerate slightly (gravity
 * overcomes the rocket's impulse) and explode as the rocket's main fuse
 * gives out (we could add a ballistic stage, maybe).  The individual
 * stars fan out from the rocket, and they decelerate less quickly.
 * That's called bouyancy, but really it's again a visual preference.
 */

#include "xl2.h"
#include <stdlib.h>
#include <math.h>
#include <vector>

using namespace std;

class PyroLockProc : public LockProc {

    static const int MAXSTARS = 100; /* Number of stars issued from a shell */
    static const int batchcount = 100;

#define STARSIZE 2 // for now this is easier as a macro

    enum rocket_state {
        SILENT, REDGLARE, BURSTINGINAIR
    };

    enum shell_type {
        CLOUD, DOUBLECLOUD
    };

/* P_xxx parameters represent the reciprocal of the probability... */
#define P_IGNITE 2000       /* ...of ignition per cycle */
#define P_DOUBLECLOUD 10    /* ...of an ignition being double */
#define P_MULTI 15      /* ...of an ignition being several @ once */
#define P_FUSILLADE 50      /* ...of an ignition starting a fusillade */

#define ROCKETW 2       /* Dimensions of rocket */
#define ROCKETH 4
#define XVELFACTOR 0.0025   /* Max horizontal velocity / screen width */
#define MINYVELFACTOR 0.016 /* Min vertical velocity / screen height */
#define MAXYVELFACTOR 0.018
#define GRAVFACTOR 0.0002   /* delta v / screen height */
#define MINFUSE 50      /* range of fuse lengths for rocket */
#define MAXFUSE 100

#define FUSILFACTOR 10      /* Generate fusillade by reducing P_IGNITE */
#define FUSILLEN 100        /* Length of fusillade, in ignitions */

#define SVELFACTOR 0.1      /* Max star velocity / yvel */
#define BOUYANCY 0.2        /* Reduction in grav deceleration for stars */

#define MINSTARS 50
#define MINSFUSE 50     /* Range of fuse lengths for stars */
#define MAXSFUSE 100

#define MAXRAND (2147483648.0)
#define INTRAND(min,max) (random()%((max+1)-(min))+(min))
#define FLOATRAND(min,max) ((min)+(random()/MAXRAND)*((max)-(min)))

    struct rocket {
        rocket_state state;
        shell_type   shelltype;
        int         color1, color2;
        int         fuse;
        float       xvel, yvel;
        float       x, y;
        int         nstars;
#if STARSIZE > 1
        XRectangle  Xpoints[MAXSTARS];
        XRectangle  Xpoints2[MAXSTARS];
#else
        XPoint      Xpoints[MAXSTARS];
        XPoint      Xpoints2[MAXSTARS];
#endif
        float       sx[MAXSTARS], sy[MAXSTARS]; /* Distance from notional
                                                 * center  */
        float       sxvel[MAXSTARS], syvel[MAXSTARS];   /* Relative to notional
                                                         * center */
    };

    Colormap    cmap;
    int         p_ignite;
    int         orig_p_ignite;
    unsigned long bgpixel;
    unsigned long fgpixel;
    unsigned long rockpixel;
    GC          bgGC;
    int         nflying;
    int         fusilcount;
    int         width, lmargin, rmargin, height;
    float       minvelx, maxvelx;
    float       minvely, maxvely;
    float       maxsvel;
    float       rockdecel, stardecel;
    vector<rocket>  rockq;
    bool        just_started;

void
initpyro(void)
{
    rocket     *rp;
    XWindowAttributes xwa;
    XGCValues   xgcv;
    int         rockn, starn, bsize;

    just_started = true;
    XGetWindowAttributes(s.dsp, s.win, &xwa);

    rockq.resize(batchcount);

    orig_p_ignite = P_IGNITE / batchcount;
    if (orig_p_ignite <= 0)
        orig_p_ignite = 1;
    p_ignite = orig_p_ignite;

    nflying = fusilcount = 0;

    bsize = (xwa.height <= 64) ? 1 : STARSIZE;
    for (rockn = 0; rockn < batchcount; rockn++) {
        rocket &rp = rockq[rockn];
        rp.state = SILENT;
#if STARSIZE > 1
        for (starn = 0; starn < MAXSTARS; starn++) {
            rp.Xpoints[starn].width = rp.Xpoints[starn].height =
                rp.Xpoints2[starn].width = rp.Xpoints2[starn].height =
                bsize;
        }
#endif
    }

    width = xwa.width;
    lmargin = xwa.width / 4;
    rmargin = xwa.width - lmargin;
    height = xwa.height;
    cmap = DefaultColormapOfScreen(s.scr);

    fgpixel = WhitePixelOfScreen(s.scr);
    bgpixel = BlackPixelOfScreen(s.scr);
    rockpixel = s.pixels[3];  /* Just the right shade of
                                 orange */

    xgcv.foreground = bgpixel;
    bgGC = XCreateGC(s.dsp, s.win, GCForeground, &xgcv);

/* Geometry-dependent physical data: */
    maxvelx = (float) (xwa.width) * XVELFACTOR;
    minvelx = -maxvelx;
    minvely = -(float) (xwa.height) * MINYVELFACTOR;
    maxvely = -(float) (xwa.height) * MAXYVELFACTOR;
    maxsvel = minvely * SVELFACTOR;
    rockdecel = (float) (height) * GRAVFACTOR;
    stardecel = rockdecel * BOUYANCY;

    XFillRectangle(s.dsp, s.win, bgGC, 0, 0, xwa.width, xwa.height);
}

void
ignite(void)
{
    rocket     *rp;
    int         multi, fuse, npix, pix;
    float       xvel, yvel, x;

    npix = s.npixels;
    x = random() % width;
    xvel = FLOATRAND(-maxvelx, maxvelx);
/* All this to stop too many rockets going offscreen: */
    if (x < lmargin && xvel < 0.0 || x > rmargin && xvel > 0.0)
        xvel = -xvel;
    yvel = FLOATRAND(minvely, maxvely);
    fuse = INTRAND(MINFUSE, MAXFUSE);

    if (random() % P_MULTI == 0)
    {
        multi = INTRAND(5, 15);
    }
    else
    {
        multi = 1;
    }

    int rpind = 0;
    rp = &rockq[rpind];

    while (multi--)
    {
        if (nflying >= batchcount)
            return;
        while (rp->state != SILENT)
        {
            rpind++;
            if (rpind >= rockq.size())
                return;
            rp = &rockq[rpind];
        }
        nflying++;
        rp->state = REDGLARE;

        if (random() % P_DOUBLECLOUD == 0)
        {
            rp->shelltype = DOUBLECLOUD;
        }
        else
        {
            rp->shelltype = CLOUD;
        }

        /*
         * Some of the rockets should burst white, which
         * won't be in the set of colors generated by
         * hsbramp().
         */
        pix = random() % (npix + 1);

        if (pix == npix)
        {
            pix = random() % (npix + 1);
            rp->color1 = WhitePixel (s.dsp, s.screen);
            rp->color2 = s.pixels[pix];
        }
        else
        {
            rp->color1 = s.pixels[pix];
            rp->color2 = s.pixels[(pix + (npix / 2)) % npix];
        }

        rp->xvel = xvel;
        rp->yvel = FLOATRAND(yvel * 0.97, yvel * 1.03);
        rp->fuse = INTRAND((fuse * 90) / 100, (fuse * 110) / 100);
        rp->x = x + FLOATRAND(multi * 7.6, multi * 8.4);
        rp->y = height - 1;
        rp->nstars = INTRAND(MINSTARS, MAXSTARS);
    }
}

void
animate(int rpind)
{
    rocket *rp = &rockq[rpind];
    int         starn;
    float       r, theta;

    if (rp->state == REDGLARE) {
        shootup(rpind);

/* Handle setup for explosion */
        if (rp->state == BURSTINGINAIR) {
            for (starn = 0; starn < rp->nstars; starn++) {
                rp->sx[starn] = rp->sy[starn] = 0.0;
                rp->Xpoints[starn].x = (int) rp->x;
                rp->Xpoints[starn].y = (int) rp->y;
                if (rp->shelltype == DOUBLECLOUD) {
                    rp->Xpoints2[starn].x = (int) rp->x;
                    rp->Xpoints2[starn].y = (int) rp->y;
                }
/* This isn't accurate solid geometry, but it looks OK. */

                r = FLOATRAND(0.0, maxsvel);
                theta = FLOATRAND(0.0, (M_PI * 2.0));
                rp->sxvel[starn] = r * cos(theta);
                rp->syvel[starn] = r * sin(theta);
            }
            rp->fuse = INTRAND(MINSFUSE, MAXSFUSE);
        }
    }
    if (rp->state == BURSTINGINAIR) {
        burst(rpind);
    }
}

void
shootup(int rpind)
{
    rocket *rp = &rockq[rpind];

    XFillRectangle(s.dsp, s.win, bgGC, (int) (rp->x), (int) (rp->y),
                   ROCKETW, ROCKETH + 3);

    if (rp->fuse-- <= 0) {
        rp->state = BURSTINGINAIR;
        return;
    }
    rp->x += rp->xvel;
    rp->y += rp->yvel;
    rp->yvel += rockdecel;
    XSetForeground(s.dsp, s.gc, rockpixel);
    XFillRectangle(s.dsp, s.win, s.gc, (int) (rp->x), (int) (rp->y),
                   ROCKETW, ROCKETH + random() % 4);
}

void
burst(int rpind)
{
    rocket *rp = &rockq[rpind];
    int starn;
    int nstars, stype;
    float rx, ry, sd;  /* Help compiler optimize :-) */
    float sx, sy;

    nstars = rp->nstars;
    stype = rp->shelltype;

#if STARSIZE > 1
    XFillRectangles(s.dsp, s.win, bgGC, rp->Xpoints, nstars);
    if (stype == DOUBLECLOUD)
        XFillRectangles(s.dsp, s.win, bgGC, rp->Xpoints2, nstars);
#else
    XDrawPoints(s.dsp, s.win, bgGC, rp->Xpoints, nstars, CoordModeOrigin);
    if (stype == DOUBLECLOUD)
        XDrawPoints(s.dsp, s.win, bgGC, rp->Xpoints2, nstars, CoordModeOrigin);
#endif

    if (rp->fuse-- <= 0) {
        rp->state = SILENT;
        nflying--;
        return;
    }
/* Stagger the stars' decay */
    if (rp->fuse <= 7) {
        if ((rp->nstars = nstars = nstars * 90 / 100) == 0)
            return;
    }
    rx = rp->x;
    ry = rp->y;
    sd = stardecel;
    for (starn = 0; starn < nstars; starn++) {
        sx = rp->sx[starn] += rp->sxvel[starn];
        sy = rp->sy[starn] += rp->syvel[starn];
        rp->syvel[starn] += sd;
        rp->Xpoints[starn].x = (int) (rx + sx);
        rp->Xpoints[starn].y = (int) (ry + sy);
        if (stype == DOUBLECLOUD) {
            rp->Xpoints2[starn].x = (int) (rx + 1.7 * sx);
            rp->Xpoints2[starn].y = (int) (ry + 1.7 * sy);
        }
    }
    rp->x = rx + rp->xvel;
    rp->y = ry + rp->yvel;
    rp->yvel += sd;

    XSetForeground(s.dsp, s.gc, rp->color1);
#if STARSIZE > 1
    XFillRectangles(s.dsp, s.win, s.gc, rp->Xpoints, nstars);
    if (stype == DOUBLECLOUD) {
        XSetForeground(s.dsp, s.gc, rp->color2);
        XFillRectangles(s.dsp, s.win, s.gc, rp->Xpoints2, nstars);
    }
#else
    XDrawPoints(s.dsp, s.win, s.gc, rp->Xpoints, nstars, CoordModeOrigin);
    if (stype == DOUBLECLOUD) {
        XSetForeground(s.dsp, s.gc, rp->color2);
        XDrawPoints(s.dsp, s.win, s.gc, rp->Xpoints2, nstars,
                    CoordModeOrigin);
    }
#endif
}

void
drawpyro(void)
{
    rocket     *rp;
    int         rockn;

    if (just_started || (random() % p_ignite == 0)) {
        just_started = false;
        if (random() % P_FUSILLADE == 0) {
            p_ignite = orig_p_ignite / FUSILFACTOR;
            fusilcount = INTRAND(FUSILLEN * 9 / 10, FUSILLEN * 11 / 10);
        }
        ignite();
        if (fusilcount > 0) {
            if (--fusilcount == 0)
                p_ignite = orig_p_ignite;
        }
    }
    for (rockn = 0; rockn < rockq.size(); rockn++) {
        rp = &rockq[rockn];
        if (rp->state != SILENT) {
            animate(rockn);
        }
    }
}

public:

PyroLockProc(xl2_screen &_s) : LockProc(_s)
{
    initpyro();
}
virtual ~PyroLockProc(void)
{
    XFreeGC(s.dsp, bgGC);
}
virtual void draw(void)
{
    drawpyro();
}

};

class PyroLockProcFactory : public LockProcFactory
{
public:
    PyroLockProcFactory(void) : LockProcFactory("pyro") { }
    virtual LockProc * make(xl2_screen &_s)
    {
        return new PyroLockProc(_s);
    };
};

static PyroLockProcFactory flame_factory;
