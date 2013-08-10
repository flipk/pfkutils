
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

/*
 * Connectix QuickCam parallel-port camera video capture driver.
 * Copyright (c) 1997, Phillip F Knaack.
 *
 * This driver is based on work by Paul Traina, Thomas Davis, 
 * Michael Chinn and Nelson Minar, 
 * though it has been heavily modified.
 *
 * Copyright (c) 1996, Paul Traina.
 * Copyright (c) 1996, Thomas Davis.
 * 
 * QuickCam(TM) is a registered trademark of Connectix Inc.
 * Use this driver at your own risk, it is not warranted by
 * Connectix or the authors.
 */
 
#include "headers.h"

/* flags in softc */
#define QC_OPEN                 0x01            /* device open */
#define QC_ALIVE                0x02            /* probed and attached */
#define QC_BIDIR_HW             0x04            /* bidir parallel port */
#define QC_FORCEUNI             0x08            /* ...but force unidir mode */

#define QC_MAX_XSIZE 320
#define QC_MAX_YSIZE 240
#define QC_ZOOM_100    0

#define QC_SETBRCN_BRIGHTNESS 1
#define QC_SETBRCN_CONTRAST   2
#define QC_SETBRCN_WHITEBAL   4

#define QC_MAXFRAMEBUFSIZE      (QC_MAX_XSIZE*QC_MAX_YSIZE)

#define read_data(P)            inb((P))
#define read_data_word(P)       inw((P))
#define read_status(P)          inb((P)+1)
#define write_data(P, V)        outb((P)+0, (V))
#define write_status(P, V)      outb((P)+1, (V))
#define write_control(P, V)     outb((P)+2, (V))

#define LONGDELAY(n)            DELAY(n)
#define DELAY(n)                usleep(n)

#define QC_TIMEOUT_INIT         512000
#define QC_TIMEOUT_CMD          512000
#define QC_TIMEOUT              512000

#define min(a, b)               ((a) < (b) ? (a) : (b))

#define SET(t, f)       (t) |= (f)
#define CLR(t, f)       (t) |= ~(f)
#define ISSET(t, f)     ((t) & (f))

/*
 * Camera autodetection parameters
 */
#define QC_PROBELIMIT           30              /* number of times to probe */
#define QC_PROBECNTLOW          5               /* minimum transitions */
#define QC_PROBECNTHI           25              /* maximum transitions */

/*
 * QuickCam camera commands
 */
#define QC_BRIGHTNESS           0x0b
#define QC_CONTRAST             0x19
#define QC_WHITEBALANCE         0x1f
#define QC_XFERMODE             0x07
#define QC_XSIZE                0x13
#define QC_YSIZE                0x11
#define QC_YORG                 0x0d
#define QC_XORG                 0x0f

/*
 * XFERmode register flags
 */
#define QC_XFER_BIDIR           0x01            /* bidirectional transfer */
#define QC_XFER_6BPP            0x02            /* 6 bits per pixel */
#define QC_XFER_WIDE            0x00            /* wide angle */
#define QC_XFER_NARROW          0x04            /* narrow */
#define QC_XFER_TIGHT           0x08            /* very narrow */

/*
 * QuickCam default values (don't depend on these staying the same)
 */
#define QC_DEF_XSIZE            160
#define QC_DEF_YSIZE            120
#define QC_DEF_XORG             7
#define QC_DEF_YORG             1
#define QC_DEF_BPP              6
#define QC_DEF_CONTRAST         104
#define QC_DEF_BRIGHTNESS       150
#define QC_DEF_WHITEBALANCE     207
#define QC_DEF_ZOOM             QC_ZOOM_100

/*
 * QuickCam parallel port handshake constants
 */
#define QC_CTL_HIGHNIB          0x06
#define QC_CTL_LOWNIB           0x0e
#define QC_CTL_HIGHWORD         0x26
#define QC_CTL_LOWWORD          0x2f


static const u_char qcam_zoommode[3][3] = {
    { QC_XFER_WIDE,   QC_XFER_WIDE,   QC_XFER_WIDE },
    { QC_XFER_NARROW, QC_XFER_WIDE,   QC_XFER_WIDE },
    { QC_XFER_TIGHT,  QC_XFER_NARROW, QC_XFER_WIDE }
};

static int qcam_timeouts;
int qcam_debug = 0;

static void qcam_init   __P((QSC));
static void qcam_reset  __P((QSC));

#define STATHIGH(T,d)     if (!timeout) qcam_timeouts++
#define STATLOW(T,d)      if (!timeout) qcam_timeouts++

#define READ_STATUS_BYTE_HIGH(P, V, T)      \
do {                                        \
    u_int timeout = (T);                    \
    do {                                    \
        (V) = read_status((P));             \
    } while (!(((V) & 0x08)) && --timeout); \
    STATHIGH(T,"a");                        \
} while(0)

#define READ_STATUS_BYTE_LOW(P, V, T)       \
do {                                        \
    u_int timeout = (T);                    \
    do {                                    \
        (V) = read_status((P));             \
    } while (((V) & 0x08) && --timeout);    \
    STATLOW(T,"b");                         \
} while(0)

#define READ_DATA_WORD_HIGH(P, V, T)        \
do {                                        \
    u_int timeout = (T);                    \
    do {                                    \
        (V) = read_data_word((P));          \
    } while (!((V) & 0x01) && --timeout);   \
    STATHIGH(T,"c");                        \
} while(0)

#define READ_DATA_WORD_LOW(P, V, T)         \
do {                                        \
    u_int timeout = (T);                    \
    do {                                    \
        (V) = read_data_word((P));          \
    } while (((V) & 0x01) && --timeout);    \
    STATLOW(T,"d");                         \
} while(0)

#if 0

/* Set EXTENT bits starting at BASE in BITMAP to value TURN_ON.
   used below to gain I/O permissions to desired ports. */

static void
set_bitmap(bitmap, base, extent, new_value)
    u_long *bitmap;
    short base;
    short extent;
    int new_value;
{
    int mask;
    u_long *bitmap_base = bitmap + (base >> 5);
    u_short low_index = base & 0x1f;
    int length = low_index + extent;
        
    if (low_index != 0) 
    {
        mask = (~0 << low_index);
        if (length < 32)
            mask &= ~(~0 << length);
        if (new_value)
            *bitmap_base++ |= mask;
        else
            *bitmap_base++ &= ~mask;
        length -= 32;
    }
  
    mask = (new_value ? ~0 : 0);
    while (length >= 32)
    {
        *bitmap_base++ = mask;
        length -= 32;
    }
        
    if (length > 0) 
    {
        mask = ~(~0 << length);
        if (new_value)
            *bitmap_base++ |= mask;
        else
            *bitmap_base++ &= ~mask;
    }
}

#define NIOPORTS 1024

/* fetch the bitmap that represents what ports we can access,
   twiddle the bits for the ports we want, then send it back. */

static int
ioperm(startport, howmany, onoff)
    u_int startport;
    u_int howmany;
    int onoff;
{
    u_long bitmap[NIOPORTS/32];
    int err;
        
    if (startport + howmany > NIOPORTS)
        return -1;
        
    if ((err = i386_get_ioperm(bitmap)))
        return err;
    /* now diddle the current bitmap with the request */
    set_bitmap(bitmap, startport, howmany, !onoff);

    return i386_set_ioperm(bitmap);
}

#endif

/*
 * sends a byte to the command port of the quickcam.
 * returns whatever the quickcam returns, which, if the 
 * quickcam accepted the command, will be the same command back.
 */

inline static int
sendbyte(port, value, sdelay)
    u_int port;
    int value;
    int sdelay;
{
    u_char s1, s2;
    
    write_data(port, value);
    
    write_control(port, QC_CTL_HIGHNIB);
    READ_STATUS_BYTE_HIGH(port, s1, sdelay);

    write_control(port, QC_CTL_LOWNIB);
    READ_STATUS_BYTE_LOW(port, s2, sdelay);

    return (s1 & 0xf0) | (s2 >> 4);
}

/*
 * send a generic command to the cam.
 */

static int
send_command(qs, cmd, value)
    struct qcam_softc *qs;
    int cmd;
    int value;
{
    if (sendbyte(qs->iobase, cmd, QC_TIMEOUT_CMD) != cmd)
        return 1;
    
    if (sendbyte(qs->iobase, value, QC_TIMEOUT_CMD) != value)
        return 1;

    return 0;
}

/*
 * prepare the camera for the I/O mode we want to use during a scan:
 * if we want unidir, bidir, 4 bit, 6 bit, etc ..
 */

static int
send_xfermode (qs, value)
    struct qcam_softc *qs;
    int value;
{
    if (sendbyte(qs->iobase, QC_XFERMODE, QC_TIMEOUT_INIT) != QC_XFERMODE)
    {
        /* sometimes this command fails; not sure why, but if we
           just reset the camera, everything goes great and we 
           barely drop a frame */
        qcam_reset(qs);
        qcam_init(qs);
        if (qcam_debug > 1)
            printf("QC_XFERMODE failed.\n");
        return 1;
    }

    if (sendbyte(qs->iobase, value, QC_TIMEOUT_INIT) != value)
    {
        if (qcam_debug > 1)
            printf("sending xfermode value failed.\n");
        return 1;
    }

    return 0;
}

/*
 * twiddle hardware reset lines and try to reset the camera.
 * also attempts to detect if its a bidir port. it does this
 * by sending a byte out the port, trying to turn off bidir
 * (for those hardare devices that have it) and if something 
 * different comes back, turning off bidir must have worked,
 * therefore the port must have bidir capability.
 */

static void
qcam_reset(qs)
    struct qcam_softc *qs;
{
    register u_int iobase = qs->iobase;
    register u_char result;

    /* this turns of bidir */
    write_control(iobase, 0x20);

    /* send test data out */
    write_data   (iobase, 0x75);

    /* read data back in .. if its the same, its not a bidir port */
    result = read_data(iobase);

    if ((result != 0x75) && !ISSET(qs->flags, QC_FORCEUNI))
        SET(qs->flags, QC_BIDIR_HW);
    else
        CLR(qs->flags, QC_BIDIR_HW);

    /* now reset the camera itself. */
    write_control(iobase, 0x0b);
    DELAY(250);
    write_control(iobase, QC_CTL_LOWNIB);
    DELAY(250);
}

/*
 * tell the camera we want bidirectional access; then wait for it
 * to acknowledge. 
 */

static int
qcam_waitfor_bi(port)
    u_int port;
{
    u_char s1, s2;

    write_control(port, QC_CTL_HIGHWORD);
    READ_STATUS_BYTE_HIGH(port, s1, QC_TIMEOUT_INIT);

    write_control(port, QC_CTL_LOWWORD);
    READ_STATUS_BYTE_LOW(port, s2, QC_TIMEOUT);

    return (s1 & 0xf0) | (s2 >> 4);
}

/*
 * The pixels are read in 16 bits at a time, and we get 3 valid pixels per
 * 16-bit read.  The encoding format looks like this:
 *
 * |---- status reg -----| |----- data reg ------|
 * 15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00
 *  3  3  3  3  2  x  x  x  2  2  2  1  1  1  1  R
 *
 *  1 = left pixel      R = camera ready
 *  2 = middle pixel    x = unknown/unused?
 *  3 = right pixel
 *
 * XXX do not use this routine yet!  It does not work.
 *     Nelson believes that even though 6 pixels are read in per 2 words,
 *     only the 1 & 2 pixels from the first word are correct.  This seems
 *     bizzare, more study is needed here.
 */

#define DECODE_WORD_BI4BPP(P, W) \
        *(P)++ = 15 - (((W) >> 12) & 0x0f); \
        *(P)++ = 15 - ((((W) >> 8) & 0x08) | (((W) >> 5) & 0x07)); \
        *(P)++ = 15 - (((W) >>  1) & 0x0f);

static void
qcam_bi_4bit(qs)
    struct qcam_softc *qs;
{
    u_char *p;
    u_int port;
    u_short word;

    port = qs->iobase;

    qcam_waitfor_bi(port);

    /*
     * Unlike the other routines, this routine has NOT be interleaved
     * yet because we don't have the algorythm for 4bbp down tight yet,
     * so why add to the confusion?
     */

    for (p = qs->buffer; p < (qs->buffer + qs->bufsize); ) {
        write_control(port, QC_CTL_HIGHWORD);
        READ_DATA_WORD_HIGH(port, word, QC_TIMEOUT);
        DECODE_WORD_BI4BPP(p, word);

        write_control(port, QC_CTL_LOWWORD);
        READ_DATA_WORD_HIGH(port, word, QC_TIMEOUT);
        DECODE_WORD_BI4BPP(p, word);
    }
}

/*
 * The pixels are read in 16 bits at a time, 12 of those bits contain
 * pixel information, the format looks like this:
 *
 * |---- status reg -----| |----- data reg ------|
 * 15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00
 *  2  2  2  2  2  x  x  x  2  1  1  1  1  1  1  R
 *
 * 1 = left pixel               R = camera ready
 * 2 = right pixel              x = unknown/unused?
 */

#define DECODE_WORD_BI6BPP(P, W) \
        *(P)++ = 63 -  (((W) >>  1) & 0x3f); \
        *(P)++ = 63 - ((((W) >> 10) & 0x3e) | (((W) >> 7) & 0x01));

static void
qcam_bi_6bit(qs)
    struct qcam_softc *qs;
{
    u_char *p;
    u_short hi, low;
    u_int port;

    port = qs->iobase;

    qcam_waitfor_bi(port);

    p = qs->buffer;

    DELAY(250);
    write_control(port, QC_CTL_HIGHWORD);
    READ_DATA_WORD_HIGH(port, hi, QC_TIMEOUT_INIT);
    DECODE_WORD_BI6BPP(p, hi);

    write_control(port, QC_CTL_LOWWORD);
    READ_DATA_WORD_LOW(port, low, QC_TIMEOUT);
    DECODE_WORD_BI6BPP(p, low);
    
    /*
     * This was interleaved before, but I cut it back to the simple
     * mode so that it's easier for people to play with it.  A quick
     * unrolling of the loop coupled with interleaved decoding and I/O
     * should get us a slight CPU bonus later.
     */
    for (; p < (qs->buffer + qs->bufsize); ) {
        write_control(port, QC_CTL_HIGHWORD);
        READ_DATA_WORD_HIGH(port, hi, QC_TIMEOUT);
        DECODE_WORD_BI6BPP(p, hi);

        write_control(port, QC_CTL_LOWWORD);
        READ_DATA_WORD_LOW(port, low, QC_TIMEOUT);
        DECODE_WORD_BI6BPP(p, low);
    }
}

/*
 * We're doing something tricky here that makes this routine a little
 * more complex than you would expect.  We're interleaving the high
 * and low nibble reads with the math required for nibble munging.
 * This should allow us to use the "free" time while we're waiting for
 * the next nibble to come ready to do any data conversion operations.
 */

#define DECODE_WORD_UNI4BPP(P, W) \
        *(P)++ = 15 - ((W) >> 4);

static void
qcam_uni_4bit(qs)
    struct qcam_softc *qs;
{
    u_char  *p, *end, hi, low;
    u_int   port;

    port = qs->iobase;
    p    = qs->buffer;
    end  = qs->buffer + qs->bufsize - 1;

    /* request and wait for first nibble */

    write_control(port, QC_CTL_HIGHNIB);
    READ_STATUS_BYTE_HIGH(port, hi, QC_TIMEOUT_INIT);

    /* request second nibble, munge first nibble while waiting, read 2nd */

    write_control(port, QC_CTL_LOWNIB);
    DECODE_WORD_UNI4BPP(p, hi);
    READ_STATUS_BYTE_LOW(port, low, QC_TIMEOUT);

    while (p < end)
    {
        write_control(port, QC_CTL_HIGHNIB);
        DECODE_WORD_UNI4BPP(p, low);
        READ_STATUS_BYTE_HIGH(port, hi, QC_TIMEOUT);
        
        write_control(port, QC_CTL_LOWNIB);
        DECODE_WORD_UNI4BPP(p, hi);
        READ_STATUS_BYTE_LOW(port, low, QC_TIMEOUT);
    }
    DECODE_WORD_UNI4BPP(p, low);
}

/*
 * If you treat each pair of nibble operations as pulling in a byte, you
 * end up with a byte stream that looks like this:
 *
 *    msb             lsb
 *      2 2 1 1 1 1 1 1
 *      2 2 2 2 3 3 3 3
 *      3 3 4 4 4 4 4 4
 */

static void
qcam_uni_6bit(qs)
    struct qcam_softc *qs;
{
    u_char *p;
    u_int   port;
    u_char  word1, word2, word3, hi, low;

    port = qs->iobase;

    /*
     * This routine has been partially interleaved... we can do a better
     * job, but for right now, I've deliberately kept it less efficient
     * so we can play with decoding without hurting peoples brains.
     */
    for (p = qs->buffer; p < (qs->buffer + qs->bufsize); ) {
        write_control(port, QC_CTL_HIGHNIB);
        READ_STATUS_BYTE_HIGH(port, hi, QC_TIMEOUT_INIT);
        write_control(port, QC_CTL_LOWNIB);
        READ_STATUS_BYTE_LOW(port, low, QC_TIMEOUT);
        write_control(port, QC_CTL_HIGHNIB);
        word1 = (hi & 0xf0) | (low >>4);
        READ_STATUS_BYTE_HIGH(port, hi, QC_TIMEOUT);
        write_control(port, QC_CTL_LOWNIB);
        *p++ = 63 - (word1 >> 2);
        READ_STATUS_BYTE_LOW(port, low, QC_TIMEOUT);
        write_control(port, QC_CTL_HIGHNIB);
        word2 = (hi & 0xf0) | (low >> 4);
        READ_STATUS_BYTE_HIGH(port, hi, QC_TIMEOUT);
        write_control(port, QC_CTL_LOWNIB);
        *p++ = 63 - (((word1 & 0x03) << 4) | (word2 >> 4));
        READ_STATUS_BYTE_LOW(port, low, QC_TIMEOUT);
        word3 = (hi & 0xf0) | (low >> 4);
        *p++ = 63 - (((word2 & 0x0f) << 2) | (word3 >> 6));
        *p++ = 63 - (word3 & 0x3f);
    }
    /* XXX this is something xfqcam does, doesn't make sense to me,
       but we don't see timeoutes here... ? */
    write_control(port, QC_CTL_LOWNIB);
    READ_STATUS_BYTE_LOW(port, word1, QC_TIMEOUT);
    write_control(port, QC_CTL_HIGHNIB);
    READ_STATUS_BYTE_LOW(port, word1, QC_TIMEOUT);
}

/*
 * turn the flags the user sets into flags the camera understands.
 * doesn't actually communicate with the camera in any way; simply
 * changes stuff around in the softc.
 */

static void
qcam_xferparms(qs)
    struct qcam_softc *qs;
{
    int bidir;

    qs->xferparms = 0;

    bidir = ISSET(qs->flags, QC_BIDIR_HW);
    if (bidir)
        SET(qs->xferparms, QC_XFER_BIDIR);

    if (qs->bpp == 6) {
        SET(qs->xferparms, QC_XFER_6BPP);
        qs->scanner    = bidir ? qcam_bi_6bit : qcam_uni_6bit;
    } else {
        qs->scanner    = bidir ? qcam_bi_4bit : qcam_uni_4bit;
    }

    if (qs->x_size > 160 || qs->y_size > 120) {
        qs->xferparms |= qcam_zoommode[0][qs->zoom];
    } else if (qs->x_size > 80 || qs->y_size > 60) {
        qs->xferparms |= qcam_zoommode[1][qs->zoom];
    } else
        qs->xferparms |= qcam_zoommode[2][qs->zoom];
}

/*
 * set only the contrast and brightness, depending on which flags
 * were set in flags. this is a separate routine, because if we
 * only want to set these, its quicker than qcam_init. qcam_init
 * sets EVERYTHING.
 */

void
qcam_setbrcn(qs,flags)
    struct qcam_softc *qs;
    int flags;
{
    qcam_xferparms(qs);

    if (ISSET(flags, QC_SETBRCN_BRIGHTNESS))
        send_command(qs, QC_BRIGHTNESS,   qs->brightness);
    if (ISSET(flags, QC_SETBRCN_CONTRAST))
        send_command(qs, QC_CONTRAST,     qs->contrast);
    if (ISSET(flags, QC_SETBRCN_WHITEBAL))
        send_command(qs, QC_WHITEBALANCE, qs->whitebalance);

}

void
qcam_setsize(qs, sz)
    struct qcam_softc *qs;
    int sz;
{
    switch (sz)
    {
    case QC_S80x60:
        qs->x_size = 80;
        qs->y_size = 60;
        break;
    case QC_S160x120:
        qs->x_size = 160;
        qs->y_size = 120;
        break;
    case QC_S320x240:
        qs->x_size = 320;
        qs->y_size = 240;
        break;
    }
    qs->init_req = 1;
} 

static void
qcam_init(qs)
    struct qcam_softc *qs;
{
    int x_size = (qs->bpp == 4) ? qs->x_size / 2 : qs->x_size / 4;

    qcam_xferparms(qs);

    send_command(qs, QC_BRIGHTNESS,   qs->brightness);
    send_command(qs, QC_YSIZE,        qs->y_size);
    send_command(qs, QC_XSIZE,        x_size);
    send_command(qs, QC_YORG,         qs->y_origin);
    send_command(qs, QC_XORG,         qs->x_origin);
    send_command(qs, QC_CONTRAST,     qs->contrast);
    send_command(qs, QC_WHITEBALANCE, qs->whitebalance);

    if (qs->buffer)
        qs->bufsize = qs->x_size * qs->y_size;

    qs->init_req = 0;
}

/*
 * now actually scan a frame.
 */

void
qcam_scan(qs)
    struct qcam_softc *qs;
{
    int timeouts;
    char *tmp;

    timeouts = qcam_timeouts;

    /* if the params in the softc have changed, but the cam's not
       been told about it, set the init_req flag so that we actually
       inform the camera. */

    if (qs->init_req)
        qcam_init(qs);

    if (send_xfermode(qs, qs->xferparms))
        return;

    if ((qcam_debug > 1) && (timeouts != qcam_timeouts))
        printf("qcam%d: %d timeouts during init\n", qs->unit,
               qcam_timeouts - timeouts);

    timeouts = qcam_timeouts;

    if (qs->scanner)
    {
        /* swap this and the last scans */
        tmp = qs->buffer;
        qs->buffer = qs->oldbuffer;
        qs->oldbuffer = tmp;

        (*qs->scanner)(qs);
    } else
        return;

    if ((qcam_debug > 1) && (timeouts != qcam_timeouts))
        printf("qcam%d: %d timeouts during scan\n", qs->unit,
               qcam_timeouts - timeouts);

    write_control(qs->iobase, 0x0b);
    write_control(qs->iobase, 0x0f);
}

static void
qcam_default(qs)
    struct qcam_softc *qs;
{
    qs->contrast     = QC_DEF_CONTRAST;
    qs->brightness   = QC_DEF_BRIGHTNESS;
    qs->whitebalance = QC_DEF_WHITEBALANCE;
    qs->x_size       = QC_DEF_XSIZE;
    qs->y_size       = QC_DEF_YSIZE;
    qs->x_origin     = QC_DEF_XORG;
    qs->y_origin     = QC_DEF_YORG;
    qs->bpp          = QC_DEF_BPP;
    qs->zoom         = QC_DEF_ZOOM;
}

static int
qcam_detect(port)
    u_int port;
{
    write_control(port, 0x0b);
    DELAY(250);
    write_control(port, 0x0e);
    DELAY(250);

    if (sendbyte(port, QC_BRIGHTNESS, QC_TIMEOUT_CMD) != QC_BRIGHTNESS)
        return 0;       /* failure */
    return (sendbyte(port, 1, QC_TIMEOUT_CMD) == 1);
}

struct qcam_softc *
qcam_new()
{
    struct qcam_softc *qs;

    qs = (struct qcam_softc *) malloc(sizeof(struct qcam_softc));

    if (!qs)
        return NULL;

    if ((qs->buffer = (u_char*) malloc(QC_MAXFRAMEBUFSIZE)))
        qs->oldbuffer = (u_char*) malloc(QC_MAXFRAMEBUFSIZE);
    if (!qs->buffer || !qs->oldbuffer)
    {
        free(qs);
        return NULL;
    }

    qcam_default(qs);
    return qs;
}

struct qcam_softc *
qcam_open(port)
    u_int port;
{
    struct qcam_softc *qs;
    int retries = 0;

    open("/dev/io", O_RDWR);

    while (!qcam_detect(port))
        if (++retries > 5)
        {
            if (qcam_debug > 1)
                printf("qcam not detected.\n");
            return NULL;
        }

    if ((qs = qcam_new()) == NULL)
        return NULL;
    
    qs->iobase = port;
    
    qcam_reset(qs);
    qcam_default(qs);
    
    qs->init_req = 1;

    return qs;
}

void
qcam_close(qs)
    struct qcam_softc *qs;
{
    open("/dev/io", O_RDWR);

    if (qs->buffer)
    {
        free(qs->buffer);
        free(qs->oldbuffer);
    }
    free(qs);
}

struct bestfit_slopes {
    int br_slope;    /* in per cent */
    int cn_slope;    /* in per cent */
};

struct ihist {
    int size;
    int *hist;
};

/*
 * make a histogram.  basically, we examine the intensity of all the
 * pixels, constructing a histogram of each intensity (seeing how often
 * whites occur, and blacks, and all shades in between).
 * then we break this histogram up into 8 regions from dark to light.
 * we allocate a new ihist struct, and return it.
 */

static struct ihist *
make_hist(qs)
    struct qcam_softc *qs;
{
    struct ihist *hp;
    int i;

    hp = (struct ihist *)malloc(sizeof(struct ihist));
    hp->size = 64 >> 3;
    hp->hist = (int *)malloc(sizeof(int) * hp->size);
    bzero(hp->hist, sizeof(int) * hp->size);
    
    for (i=0; i < (qs->x_size * qs->y_size); i++)
        hp->hist[qs->buffer[i] >> 3]++;

    return hp;
}

static void
free_hist(hp)
    struct ihist *hp;
{
    free(hp->hist);
    free(hp);
}

/*
 * Mmmmmm ... statistics ...
 *
 * here we analyze the histogram created above, doing best-fit lines
 * against the magnitudes. we only need the slope of this line; a good
 * picture has a predictable slope (the comparison of light-to-dark
 * pixels should be a 0-slope line).
 *
 * also, more complicated, we ``fold'' the left and right halves of
 * the histogram onto each other, so that we can measure a comparison of
 * light-dark pixels with middle-of-the-road pixels. the slope of this
 * line should be somewhere near zero as well.
 *
 * return the slopes in a struct supplied by the user, and they are scaled
 * to the number of data points in the image (basically a percent figure).
 */

static void
calc_slopes(hp, bfp)
    struct ihist *hp;
    struct bestfit_slopes *bfp;
{
    int SSxy, SSx, SSy, SSxx, N;
    int i, size;

    SSxy = SSx = SSy = SSxx = 0;

    N = size = hp->size;

    for (i=0; i < N; i++)
    {
        int y = hp->hist[i];

        SSx  += i;
        SSy  += y;
        SSxy += (i * y);
        SSxx += i^2;
    }
    
    bfp->br_slope =
        ((N * SSxy) - (SSx * SSy)) / ((N * SSxx) - (SSx ^ 2));

    bfp->br_slope *= 100;
    bfp->br_slope /= SSy;

    N = N >> 1;
    SSx = SSxy = SSxx = 0;  /* SSy should be the same */
    
    for (i=0; i < N; i++)
    {
        int y = hp->hist[i] + hp->hist[size - i - 1];

        SSx  += i;
        SSxy += (i * y);
        SSxx += i^2;
    }
    
    bfp->cn_slope =
        ((N * SSxy) - (SSx * SSy)) / ((N * SSxx) - (SSx ^ 2));

    bfp->cn_slope *= 100;
    bfp->cn_slope /= SSy;
}

/*
 * here we are given a qcam_softc struct, and we analyze the last 
 * scan with the above heuristic. we then use those numbers to adjust
 * the brightness and contrast of the camera. if we make adjustments, we
 * return 1, if we don't make any adjustments, we return 0. calling
 * programs can use this to adjust until correct, then take a full-size
 * snapshot.
 */

static int read_settings_file( struct qcam_softc * qs );

int
qcam_adjustpic(qs)
    struct qcam_softc *qs;
{
    struct ihist *         hp;
    struct bestfit_slopes  bf;
    int ch, ch2, ob, oc, brs, cns;

    ch = ch2 = ob = oc = 0;

    hp = make_hist(qs);
    calc_slopes(hp, &bf);
    free_hist(hp);

    brs = bf.br_slope;
    cns = bf.cn_slope;

    /* we could do some more intelligent guessing here;
       i.e., if we're off by more than 5 per cent, move 
       the brightness faster, if by 8 per cent, even faster.. 
       however, that's more difficult to optimize. so we just
       do it slowly, one setting at a time. */

    if (brs > 2)
        ob = -1;

    if (brs > 5)
        ob = -5;

    if (brs > 8)
        ob = -8;

    if (brs < -2)
        ob = 1;

    if (brs < -5)
        ob = 5;

    if (brs < -8)
        ob = 8;

    /* only adjust contrast if the brightness is correct. 
       contrast settings are useless if the brightness is wrong,
       and in fact, moving the contrast throws off the brightness
       a bit. easier to do one at a time. a bit slower to adjust,
       though.. */

    if (ch == 0 && (ob > 1 || ob < -1))
    {
        ch |= QC_SETBRCN_BRIGHTNESS;
        qs->brightness += ob;
    } else {
        if (ob != 0)
            ch |= QC_SETBRCN_BRIGHTNESS;

        qs->brightness += ob;

        if (cns > 7)
            oc = 3;

        if (cns > 15)
            oc = 6;

        if (cns < 3)
            oc = -3;

        if (cns < -5)
            oc = -6;

        /* however, sometimes it will go nuts, like if you
           are pointing at an image with wide variations in
           the light. 30 is then a lower bound on the contrast,
           and 200 an upper. before these limits, it would
           sometimes wrap around 0 -> 255 and the situation
           would be hopeless from then on. */
        if ((oc) && 
            ((((qs->contrast + oc) >   0) && (oc < 0)) ||
             (((qs->contrast + oc) < 200) && (oc > 0))))
        {
            ch |= QC_SETBRCN_CONTRAST;
            qs->contrast += oc;
        }
    }
        
    if (qcam_debug)
    {
        printf("bright:   %3d %03d   ", brs, qs->brightness);
        printf("contrast: %3d %03d  ",  cns, qs->contrast);
    }

    ch2 = read_settings_file( qs );
    ch |= ch2;

    if (ch)
    {
        if (qcam_debug && (ISSET(ch, QC_SETBRCN_BRIGHTNESS)))
            printf("brightness: %+d ", ob);
        if (qcam_debug && (ISSET(ch, QC_SETBRCN_CONTRAST)))
            printf("contrast: %+d", oc);
        if (qcam_debug)
            printf("\n");

        qcam_setbrcn(qs, ch);
        if ( ch2 != 0 )
            return 0;
        return 1;
    }

    if (qcam_debug) 
        printf("\n");

    return 0;
}

static int
read_settings_file( struct qcam_softc * qs )
{
    FILE * f;
    int contrast;
    int brightness;
    int whitebal;
    int flags = 0;

    f = fopen( "/home/flipk/qcamsettings", "r" );
    if ( f == NULL )
        return 0;

    if ( fscanf( f, "%d %d %d",
                 &brightness, &contrast, &whitebal ) != 3 )
    {
        fclose( f );
        return 0;
    }

    if ( brightness != 0 )
    {
        flags |= QC_SETBRCN_BRIGHTNESS;
        qs->brightness = brightness;
    }

    if ( contrast != 0 )
    {
        flags |= QC_SETBRCN_CONTRAST;
        qs->contrast = contrast;
    }

    if ( whitebal != 0 )
    {
        flags |= QC_SETBRCN_WHITEBAL;
        qs->whitebalance = whitebal;
    }

    fclose( f );
    return flags;
}
