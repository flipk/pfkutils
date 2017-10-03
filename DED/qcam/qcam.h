
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

extern int qcam_debug;

struct qcam_softc {
    u_char          *buffer;             /* frame buffer */
    u_char          *oldbuffer;          /* the previous scan buffer */
    u_int           bufsize;             /* x_size times y_size */
    u_int           flags;
    u_int           iobase;
    int             unit;                /* device */
    void            (*scanner)(struct qcam_softc *);
    int             init_req;            /* initialization required */
    int             x_size;              /* pixels */
    int             y_size;              /* pixels */
    int             x_origin;            /* ?? units */
    int             y_origin;            /* ?? units */
    int             zoom;                /* 0=none, 1=1.5x, 2=2x */
    int             bpp;                 /* 4 or 6 */
    u_char          xferparms;           /* calcualted transfer params */
    u_char          contrast;
    u_char          brightness;
    u_char          whitebalance;
};

typedef struct qcam_softc * QSC;

QSC   qcam_open           __P((u_int));
QSC   qcam_new            __P((void));
void  qcam_scan           __P((QSC));
int   qcam_adjustpic      __P((QSC));
void  qcam_close          __P((QSC));
void  qcam_setbrcn        __P((QSC, int));
void  qcam_setsize        __P((QSC, int));
int   qcam_detect_motion  __P((QSC, int, int));

#define QC_S80x60   1
#define QC_S160x120 2
#define QC_S320x240 3

