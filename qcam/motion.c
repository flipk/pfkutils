/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

#include "headers.h"

int
qcam_detect_motion(qs, pth, frth)
	struct qcam_softc *qs;
	int pth, frth;
{
	char *ap, *bp;
	int i, j = qs->x_size * qs->y_size;
	int cnt = 0;

	for (ap = qs->buffer, bp = qs->oldbuffer, i=0;
	     i < j;
	     i++, ap++, bp++)
	{
		char df = *ap - *bp;
		if (df < 0)
			df = -df;
		if (df > pth)
			cnt++;
	}

	if (cnt > frth)
		return 1;
	else
		return 0;
}

int
qcam_detect_motion2(qs, loc, pth, frth)
	struct qcam_softc *qs;
	int pth, frth;
	struct motion_location *loc;
{
	char *ap, *bp;
	int i, j = qs->x_size * qs->y_size;
	int foundfirst;
	int largestx, largesty;
	int cnt = 0;

	foundfirst = 0;
	largestx = largesty = 0;

	loc->detected = 0;

	for (ap = qs->buffer, bp = qs->oldbuffer, i=0;
         i < j; i++, ap++, bp++)
	{
		char df = *ap - *bp;
		if (df < 0)
			df = -df;
		if (df > pth)
		{
			int x,y;
			x = i % qs->x_size;
			y = i / qs->x_size;
			if (!foundfirst)
			{
				foundfirst = 1;
				loc->top.x = x;
				loc->top.y = y;
			}
			else
			{
				if (x > largestx)
					largestx = x;
				if (y > largesty)
					largesty = y;
			}
			cnt++;
		}
	}

	loc->bottom.x = largestx;
	loc->bottom.y = largesty;

	if (cnt > frth)
	{
		loc->detected = 1;
		return 1;
	}

	return 0;
}

