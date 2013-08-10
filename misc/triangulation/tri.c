
/*
    This file is part of the "pfkutils" tools written by Phil Knaack
    (pknaack1@netscape.net).
    Copyright (C) 2008  Phillip F Knaack

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdio.h>
#include <math.h>

// if you get some fails, perhaps try
// increasing this constant value.
#define PRESCALE 60.0
#define SCALE_THRESHOLD 100000
#define MAX_PASSES 300

typedef struct {
    double x;
    double y;
    double z;
} Point;

// this array declares the x,y,z coordinates of the pulse emitters.

Point emitters[4] = {
    {  0,  0,  0 },   // emitter 0
    { 10,  0,  0 },   // emitter 1
    {  5, 10,  0 },   // emitter 2
    {  5,  5, 10 }    // emitter 3
};


typedef struct {
    double e0e1; // delay between emitters 0 and 1
    double e1e2; // delay between emitters 1 and 2
    double e2e3; // delay between emitters 2 and 3
} DelayVector;

// use pythagorean theorem to calculate distance between
// two points.

double
distance( Point * p1, Point * p2 )
{
    // don't bother with fabs since squaring anyway

    double dx = p1->x - p2->x;
    double dy = p1->y - p2->y;
    double dz = p1->z - p2->z;

    return sqrt( pow(dx,2.0) + pow(dy,2.0) + pow(dz,2.0) );
}

// convert distance in meters to a delay in milliseconds

double
delay1( double dist )
{
    // consider 300m/s to be the speed of sound in STP
    return dist / 0.3;
}

// take in a point, and calculate the delayvector
// based on the position of the emitters.

void
calculate_dv( Point * p, DelayVector * dv )
{
    int i;
    double del[4];

    // calculate delay time from each emitter to the receiver

    for (i=0; i < 4; i++)
        del[i] = delay1(distance(p,&emitters[i]));

#if 0
    printf("delay(%2d,%2d,%2d,%2d) ",
           (int)del[0], (int)del[1],
           (int)del[2], (int)del[3]);
#endif

    // convert actual delays to perceived delays at the
    // receiver; this is because receiver can only measure
    // time between pulses, since it doesn't actually have
    // a way to know when the first pulse was sent.

    for (i=1; i < 4; i++)
        del[i] -= del[0];
    del[0]  = 0;   // first is always zero
    del[1] += 100; // second pulse issued at t=100ms
    del[2] += 200; // third pulse issued at t=200ms
    del[3] += 300; // fourth pulse issued at t=300ms

#if 0
    printf("perceived(%d,%3d,%3d,%3d) ",
           (int)del[0], (int)del[1],
           (int)del[2], (int)del[3]);
#endif
    dv->e0e1 = del[1] - del[0];
    dv->e1e2 = del[2] - del[1];
    dv->e2e3 = del[3] - del[2];
}

// take in a point, an error, and 2 emitter points,
// and adjust the point according to the error.
// return the scale used, to assist with determining
// the termination point.

double
adjust( Point * p, double error, Point * ea, Point * eb )
{
    double dax, day, daz;
    double dbx, dby, dbz;
    double scale;
    int i;

    // avoid a divide-by-zero error in extremely
    // rare case where we hit it dead-on.
    if (error == 0.0)
        return SCALE_THRESHOLD;

    // if you get some fails, perhaps try
    // increasing this constant value.

    scale = PRESCALE / fabs(error);

    if (scale > SCALE_THRESHOLD)
        return scale;

    dax = (ea->x - p->x) / scale;
    day = (ea->y - p->y) / scale;
    daz = (ea->z - p->z) / scale;

    dbx = (eb->x - p->x) / scale;
    dby = (eb->y - p->y) / scale;
    dbz = (eb->z - p->z) / scale;

//    printf("e: %6.2f s: %6.2f ds: %5.2f %5.2f %5.2f %5.2f %5.2f %5.2f\n",
//           error, scale, dax, day, daz, dbx, dby, dbz);

    // depending on the sign of the error, move the 
    // point closer to or further from one of the emitters

    if (error < 0)
    {
        p->x += dax;  p->x -= dbx;
        p->y += day;  p->y -= dby;
        p->z += daz;  p->z -= dbz;
    }
    else
    {
        p->x -= dax;  p->x += dbx;
        p->y -= day;  p->y += dby;
        p->z -= daz;  p->z += dbz;
    }

    return scale;
}

// take in a delayvector and try to estimate the 
// position given what we know about the emitters.

int
estimate_pos( DelayVector * indv, Point * _p )
{
    Point p = { 5,5,5 };
    DelayVector dv, error;
    int passes = 0, exceeded = 0;
    double scale1, scale2, scale3;

    while(1)
    {
//        printf("try : %6.2f,%6.2f,%6.2f\n", p.x, p.y, p.z);

        calculate_dv( &p, &dv );

        error.e0e1 = dv.e0e1 - indv->e0e1;
        error.e1e2 = dv.e1e2 - indv->e1e2;
        error.e2e3 = dv.e2e3 - indv->e2e3;

//        printf("error: %6.2f,%6.2f,%6.2f\n",
//               error.e0e1, error.e1e2, error.e2e3);

        scale1 = adjust( &p, error.e0e1, &emitters[0], &emitters[1] );
        scale2 = adjust( &p, error.e1e2, &emitters[1], &emitters[2] );
        scale3 = adjust( &p, error.e2e3, &emitters[2], &emitters[3] );

//        printf("\n");


        if (++passes == MAX_PASSES)
        {
            exceeded = 1;
            break;
        }

        if (scale1 > SCALE_THRESHOLD &&
            scale2 > SCALE_THRESHOLD &&
            scale3 > SCALE_THRESHOLD  )
            break;
    }

    *_p = p;

    if (exceeded)
        return -1;

    return passes;
}

int
main()
{
    Point p, try;
    DelayVector dv;

#if 0 // test one point
    p.x = 7.2; p.y = 7.8; p.z = 2.0;
//    p.x = 8.0; p.y = 7.2; p.z = 1.9;
//    p.x = 2.8; p.y = 9.6; p.z = 8.4;
    printf("(%5.2f,%5.2f,%5.2f):\n", p.x, p.y, p.z);
    calculate_dv( &p, &dv );
    passes = estimate_pos( &dv, &try );
    printf("%d passes -> %f,%f,%f\n",
           passes, try.x, try.y, try.z);
    total_passes += passes;
#endif

#if 1 // run thru the test point list
    int i;
    int fails = 0, passes, total_passes = 0;
    int max, min;

#include "test_points.c"

    max = 0; min = MAX_PASSES;
    for (i=0; i < NUM_COORDS; i++)
    {
        p = test_points[i];
        printf("(%5.2f,%5.2f,%5.2f): ", p.x, p.y, p.z);
        calculate_dv( &p, &dv );
        printf("delta(%6.2f,%6.2f,%6.2f) -> ", dv.e0e1, dv.e1e2, dv.e2e3);
        passes = estimate_pos( &dv, &try );
        printf(" %3d passes -> (%5.2f,%5.2f,%5.2f)\n",
               passes, try.x, try.y, try.z);
        if (passes < 0)
            fails++;
        else
        {
            total_passes += passes;
            if (passes > max)
                max = passes;
            if (passes < min)
                min = passes;
        }
    }

    fprintf(stderr,
            "%d points, %d total passes "
            "(%d passes/point, min %d max %d), %d fails\n",
            NUM_COORDS, total_passes,
            (total_passes / NUM_COORDS), min, max,
            fails);
#endif

    return 0;
}
