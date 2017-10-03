
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

#include "headers.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

static int multiplier;
static Screen *screen;
static Display *disp;
static Window root,win;
static XColor col;
static Colormap cmap;
static XImage *ximage;
static XEvent ev;
static GC gc;
static int screen_num;
static unsigned long white,black;
static int *sbuf_int;
static char *sbuf_char;
static int *colortable;
static int has24;

static int *    xqc_createpalette   __P((Colormap));

void
xwindisp_init( struct qcam_softc *qs, int m )
{
    XSizeHints hints;
    XWMHints wmhints;
    int width, height;
    char *window_name = "QuickCam";
    char *icon_name = "QuickCam";
    XTextProperty windowName, iconName;
    GC dgc;

    multiplier = m;
    width = qs->x_size * m;
    height = qs->y_size * m;
    disp = XOpenDisplay(NULL);
    if (!disp)
        errx(1, "cannot open display");
  
    screen = DefaultScreenOfDisplay(disp);
    screen_num = DefaultScreen(disp);
    white = XWhitePixel(disp,screen_num);
    black = XBlackPixel(disp,screen_num);
  
    root = DefaultRootWindow(disp);
  
    win = XCreateSimpleWindow(disp,root,0,0,width,height,0,white,black);
    if (!win)
        errx(1, "cannot open window");
  
    /* tell window manager about our window */
    hints.flags = PSize|PMaxSize|PMinSize;
    hints.min_width = hints.max_width = hints.width=width;
    hints.min_height = hints.max_height = hints.height=height;
    wmhints.input = True;
    wmhints.flags = InputHint;

    XStringListToTextProperty(&window_name, 1 ,&windowName);
    XStringListToTextProperty(&icon_name, 1 ,&iconName);


    XSetWMProperties(disp, win, 
                     &windowName, &iconName,
                     NULL, 0,
                     &hints, &wmhints, NULL);
        
    /*  XStoreName(disp, win, "QuickCam"); */
    XSelectInput(disp, win, ExposureMask);
    XMapRaised(disp, win);
    XNextEvent(disp, &ev);
  
    dgc = DefaultGC( disp, screen_num );
    gc = XCreateGC(disp, win, 0, 0);
    XCopyGC( disp, dgc, 0xffffffff, gc );
  
    has24 = 0;
    {
        int i;
        int ndepths;
        int * depths;
        depths = XListDepths( disp, screen_num, &ndepths );
        for ( i = 0; i < ndepths && !has24; i++ )
            if ( depths[i] == 24 )
                has24 = 1;
        XFree( depths );
    }

    ximage = XCreateImage(disp, DefaultVisual(disp, screen_num), 
                          has24 ? 24 : 8,
                          ZPixmap, NULL, NULL, width, height,
                          8, 0);

    if (!ximage)
        errx(1, "cannot create image");

    if ( has24 )
    {
        ximage->data = malloc(ximage->bytes_per_line * ximage->height * 4);
        sbuf_int = (int*)ximage->data;
    }
    else
    {
        ximage->data = malloc(ximage->bytes_per_line * ximage->height);
        sbuf_char = ximage->data;
    }

    cmap = DefaultColormap(disp, screen_num);
    colortable = xqc_createpalette(cmap);
}

static int *
xqc_createpalette( Colormap cmap )
{
    int *pal;
    int i;

    pal = (int*)malloc(sizeof(int[64]));

    for (i=0; i<64; i++)
    {
        col.red = col.green = col.blue = i * 1024;
        if (!XAllocColor(disp, cmap, &col))
            fprintf(stderr, "XAllocColor failed on %d\n", i);
        pal[i] = col.pixel;
    }

    return pal;
}

void
xwindisp( struct qcam_softc *qs )
{
    int i,j,i1,j1,k;
    int x,y;
    char *scan = qs->buffer;

    x = qs->x_size;
    y = qs->y_size;

    if ( has24 )
        for (k = j = 0; j < y; j++)
            for (j1=0; j1 < multiplier; j1++)
                for (i=0; i < x; i++)
                    for (i1=0; i1 < multiplier; i1++)
                        sbuf_int[k++] = colortable[(int)scan[j*x+i]];
    else
        for (k = j = 0; j < y; j++)
            for (j1=0; j1 < multiplier; j1++)
                for (i=0; i < x; i++)
                    for (i1=0; i1 < multiplier; i1++)
                        sbuf_char[k++] = colortable[(int)scan[j*x+i]];

    XPutImage(disp, win, gc, ximage, 0, 0, 0, 0,
              qs->x_size * multiplier, qs->y_size * multiplier);
    XFlush(disp);
}

void
xwindisp_close( void )
{
    XDestroyImage(ximage);
    XDestroyWindow(disp, win);
    XCloseDisplay(disp);
}

