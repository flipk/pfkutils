 /*-
 * rotor.c - A swirly rotor for xlock, the X Window System lockscreen.
 *
 * 4 Feb 2019 - modified by pfk@pfk.org to port to C++ classes in XLOCK2
 *
 * Copyright (c) 1991 by Patrick J. Naughton.
 *
 * See xlock.c for copying information.
 *
 * Revision History:
 * 11-Nov-90: put into xlock (by Steve Zellers, zellers@sun.com)
 * 16-Oct-90: Received from Tom Lawrence (tcl@cs.brown.edu: 'flight' simulator)
 */

/*
 * A 'batchcount' of 3 or 4 works best!
 */

#include "xl2.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <vector>

#define CAT(X,Y)    X##Y

class RotorLockProc : public LockProc {

    static const int SAVE = 100; /* this is a good constant to tweak */
    static const int REPS = 50;
#define MAXANGLE 10000.0  // why doesn't static const float work? stupid.
    static const int batchcount = 3;

    struct elem {
        float       angle;
        float       radius;
        float       start_radius;
        float       end_radius;
        float       radius_drift_max;
        float       radius_drift_now;
        float       ratio;
        float       start_ratio;
        float       end_ratio;
        float       ratio_drift_max;
        float       ratio_drift_now;
    };

    std::vector<elem> elements;

    int         pix;
    int         lastx, lasty;
    int         num, rotor, prev;
    int         savex[SAVE], savey[SAVE];
    float       angle;
    int         centerx, centery;
    bool        firsttime;
    bool        smallscreen;    /* for iconified view */
    bool        forward;


void
initrotor(void)
{
    XWindowAttributes xgwa;
    int         x;
//    bool     wassmall;

    XGetWindowAttributes(s.dsp, s.win, &xgwa);
    centerx = xgwa.width / 2;
    centery = xgwa.height / 2;

    /*
     * sometimes, you go into small view, only to see a really whizzy pattern
     * that you would like to look more closely at. Normally, clicking in the
     * icon reinitializes everything - but I don't, cuz I'm that kind of guy.
     * HENCE, the wassmall stuff you see here.
     */

    smallscreen = false;
    pix = 0;

//    wassmall = smallscreen;
//    smallscreen = (xgwa.width < 100);

//    if (wassmall && !smallscreen)
        firsttime = true;
//    else
    {
        num = batchcount;

        elements.resize(num);

        memset(savex, 0, sizeof(savex));

        for (x = 0; x < num; x++)
        {
            elem * pelem = &elements[x];

            pelem->radius_drift_max = 1.0;
            pelem->radius_drift_now = 1.0;

            pelem->end_radius = 100.0;

            pelem->ratio_drift_max = 1.0;
            pelem->ratio_drift_now = 1.0;
            pelem->end_ratio = 10.0;
        }

        rotor = 0;
        prev = 1;
        lastx = centerx;
        lasty = centery;
        angle = (random() % (long) MAXANGLE) / 3;
        forward = firsttime = true;
    }

    XSetForeground(s.dsp, s.gc, BlackPixel(s.dsp, s.screen));
    XFillRectangle(s.dsp, s.win, s.gc, 0, 0, xgwa.width, xgwa.height);
}

void
drawrotor(void)
{
    struct elem *pelem;
    int         thisx, thisy;
    int         i, rp;
    int         x1, y1, x2, y2;

#define SCALE(W,N)  CAT(W,N)/=12; CAT(W,N)+=(CAT(center,W)-2)
#define SCALEIFSMALL() \
    if (smallscreen) { \
        SCALE(x,1); SCALE(x,2);  \
        SCALE(y,1); SCALE(y,2); \
    }

    for (rp = 0; rp < REPS; rp++)
    {
        thisx = centerx;
        thisy = centery;

        for (i = 0; i < num; i++)
        {
            elem * pelem = &elements[i];

            if (pelem->radius_drift_max <= pelem->radius_drift_now)
            {
                pelem->start_radius = pelem->end_radius;
                pelem->end_radius =
                    (float) (random() % 40000) / 100.0 - 200.0;
                pelem->radius_drift_max =
                    (float) (random() % 100000) + 10000.0;
                pelem->radius_drift_now = 0.0;
            }
            if (pelem->ratio_drift_max <= pelem->ratio_drift_now)
            {
                pelem->start_ratio = pelem->end_ratio;
                pelem->end_ratio =
                    (float) (random() % 2000) / 100.0 - 10.0;
                pelem->ratio_drift_max =
                    (float) (random() % 100000) + 10000.0;
                pelem->ratio_drift_now = 0.0;
            }
            pelem->ratio = pelem->start_ratio +
                (pelem->end_ratio - pelem->start_ratio) /
                pelem->ratio_drift_max * pelem->ratio_drift_now;
            pelem->angle = angle * pelem->ratio;
            pelem->radius = pelem->start_radius +
                (pelem->end_radius - pelem->start_radius) /
                pelem->radius_drift_max * pelem->radius_drift_now;

            thisx += (int) (cos(pelem->angle) * pelem->radius);
            thisy += (int) (sin(pelem->angle) * pelem->radius);

            pelem->ratio_drift_now += 1.0;
            pelem->radius_drift_now += 1.0;
        }
        if (firsttime)
            firsttime = false;
        else
        {
            XSetForeground(s.dsp, s.gc, BlackPixel(s.dsp, s.screen));

            x1 = (int) savex[rotor];
            y1 = (int) savey[rotor];
            x2 = (int) savex[prev];
            y2 = (int) savey[prev];

            SCALEIFSMALL();

            XDrawLine(s.dsp, s.win, s.gc, x1, y1, x2, y2);

            XSetForeground(s.dsp, s.gc, s.pixels[pix]);
            if (++pix >= s.npixels)
                pix = 0;

            x1 = lastx;
            y1 = lasty;
            x2 = thisx;
            y2 = thisy;

            SCALEIFSMALL();

            XDrawLine(s.dsp, s.win, s.gc, x1, y1, x2, y2);
        }

        savex[rotor] = lastx = thisx;
        savey[rotor] = lasty = thisy;

        ++rotor;
        rotor %= SAVE;
        ++prev;
        prev %= SAVE;

        if (forward)
        {
            angle += 0.01;
            if (angle >= MAXANGLE)
            {
                angle = MAXANGLE;
                forward = false;
            }
        }
        else
        {
            angle -= 0.1;
            if (angle <= 0)
            {
                angle = 0.0;
                forward = true;
            }
        }
    }
}

public:
RotorLockProc(xl2_screen &_s) : LockProc(_s)
{
    initrotor();
}
virtual ~RotorLockProc(void)
{

}
virtual void draw(void)
{
    drawrotor();
}

};

class RotorLockProcFactory : public LockProcFactory
{
public:
    RotorLockProcFactory(void) : LockProcFactory("rotor") { }
    virtual LockProc * make(xl2_screen &_s)
    {
        return new RotorLockProc(_s);
    };
};

static RotorLockProcFactory flame_factory;
