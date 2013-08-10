
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

#define QCAM 0x378

#define CAMERA_IN    0x001
#define MOVIE_IN     0x002
#define ONE_IN       0x004
#define PREVIEW_ONE  0x008
#define MOVIE_OUT    0x010
#define TTY_OUT      0x020
#define XWIN_OUT     0x040
#define CAM_DEBUG1   0x080
#define CAM_DEBUG2   0x100
#define MOTION_DET   0x200
#define TTY_WEB_OUT  0x400

#define   SET(a)  (options) |= (a)
#define ISSET(a) ((options) &  (a))

static char * progname;

static void  usage           __P((void));
static int   keyinput        __P((void));
static void  motion_detect   __P((u_int, char *, int, int));
static void  dispartial      __P((struct qcam_softc *,
                                  struct motion_location *,
                                  FILE *));

extern char *optarg;
extern int optind;
extern int optopt;
extern int opterr;
extern int optreset;

static void
usage()
{
    fprintf(stderr,
"\n"
"usage: %s [-qoptwx123] [-z pixel frame] [-d arg]\n"
"             [-m moviefile] [-M moviefile]\n"
"\n"
"   -q: read from quickcam\n"
"   -m: read input from 'moviefile' (can be a gzipped file)\n"
"\n"
"   -o: read one frame\n"
"   -p: preview before taking one frame\n"
"   -1: set quickcam to 80x60 (previews are always 80x60)\n"
"   -2: set quickcam to 160x120 (or if showing movie on X, double size)\n"
"   -3: set quickcam to 320x240 (or if showing movie on X, triple size)\n"
"\n"
"   -t: output to tty (or preview to tty) (using text charset, must be 80x25)\n"
"   -w: tty output in a web-ish format\n"
"   -x: output to X11 (or preview to X11)\n"
"   -M: output to 'moviefile'\n"
"\n"
"   -s: sleep useconds between frame\n"
"   -d: put quickcam driver in debug mode (arg 0 [default], 1, or 2)\n"
"   -z: motion-detect on qcam using pixel and frame threshold warnings\n"
"\n"
"Note that -p must be accompanied by -t or -x, and -M.\n"
"The -q and -m options are exclusive. An input device is required.\n"
"At least one output device is required, and more than one may be specified.\n"
"Both inputting and outputting to a movie file is not allowed.\n"
"The -o and -p options require -M as well, unless -o is used with -m,\n"
"in which case %s will wait for user input after displaying one frame.\n"
"-z assumes -q, but you must specify -M, -t, or -x for output.\n"
"\n", progname, progname);

    exit(1);
}

void
sigbus_handler( int s )
{
    printf( "\n"
            "  **** BUS ERROR! ****\n"
            "  (not running as root or with setuid root?)\n"
            "\n" );
    exit(1);
}

int
qcam_main( int argc, char ** argv )
{
    struct qcam_softc *qs = NULL;
    u_int options = 0;
    int scan_size = QC_S80x60;
    char *infile = "/dev/stdin";
    char *outfile = "/dev/stdout";
    FILE *infilef  = NULL;
    FILE *outfilef = NULL;
    int ch, sizemult=0;
    int pixel_threshold=0, frame_threshold=0;
    int sleeptime = 0;

    progname = argv[0];

    signal( SIGBUS, sigbus_handler );

    if (argc == 1)
        usage();

    while ((ch = getopt(argc, argv, "qm:os:pM:d:twx123z:")) != -1)
        switch (ch) {
        case 'q':
            SET(CAMERA_IN);
            break;
        case 'm':
            SET(MOVIE_IN);
            infile = optarg;
            break;
        case 'o':
            SET(ONE_IN);
            break;
        case 'p':
            SET(PREVIEW_ONE);
            break;
        case 'M':
            SET(MOVIE_OUT);
            outfile = optarg;
            break;
        case 't':
            SET(TTY_OUT);
            break;
        case 'w':
            SET(TTY_OUT);
            SET(TTY_WEB_OUT);
            break;
        case 'x':
            SET(XWIN_OUT);
            break;
        case 's':
            sleeptime = atoi(optarg);
            break;
        case 'd':
        {
            int d = atoi(optarg);
            if (d == 1)
                SET(CAM_DEBUG1);
            else if (d == 2)
                SET(CAM_DEBUG2);
            else if (d != 0)
                usage();
            break;
        }
        case '1':
            if (!sizemult)
                sizemult=1;
            scan_size = QC_S80x60;
            break;
        case '2':
            if (sizemult)
                sizemult=2;
            else {
                sizemult=1;
                scan_size = QC_S160x120;
            }
            break;
        case '3':
            if (sizemult)
                sizemult=3;
            else {
                sizemult=1;
                scan_size = QC_S320x240;
            }
            break;
        case 'z':
            pixel_threshold = atoi(optarg);
            frame_threshold = atoi(argv[optind]);
            optind++;
            SET(MOTION_DET);
            break;
        default:
            usage();
        }
    argc -= optind;
    argv += optind;

    if (ISSET(MOTION_DET))
    {
        motion_detect(options, ISSET(MOVIE_OUT) ? outfile : NULL,
                      pixel_threshold, frame_threshold);
        return 0;
    }

    if (!(ISSET(CAMERA_IN) || ISSET(MOVIE_IN)))
        errx(1, "need input device");

    if (!(ISSET(MOVIE_OUT) || ISSET(TTY_OUT) || ISSET(XWIN_OUT)))
        errx(1, "need output device");

    if (ISSET(MOVIE_IN) && ISSET(MOVIE_OUT))
        errx(1, "file input and file output are mutually exclusive");

    if (ISSET(PREVIEW_ONE) && 
        !(ISSET(TTY_OUT) || ISSET(XWIN_OUT)))
        errx(1, "need tty_out or xwin_out for preview");

    if (ISSET(PREVIEW_ONE) && !(ISSET(MOVIE_OUT)))
        errx(1, "need a file name to output snapshot to");
        
    if (ISSET(CAMERA_IN))
    {
        qs = qcam_open(QCAM);
        if (!qs)
            errx(1, "cannot access quickcam");
        qcam_setsize(qs, scan_size);
        if (ISSET(CAM_DEBUG1))
            qcam_debug = 1;
        if (ISSET(CAM_DEBUG2))
            qcam_debug = 2;
    }

    setuid(getuid());

    if (ISSET(MOVIE_IN))
    {
        int l = strlen(infile);
        qs = qcam_new();
        if (strcmp(infile, "-") == 0)
            infilef = stdin;
        else if ((l > 3 && strcmp(infile+l-3, ".gz")  == 0) ||
                 (l > 4 && strcmp(infile+l-4, ".bz2") == 0))
        {
            int fd, pid, pipefds[2];
            pipe(pipefds);
            fd = open(infile, O_RDONLY);
            if (fd < 0)
                goto error_opening;
            pid = fork();
            if (pid > 0)
            {
                /* in parent */
                close(fd);
                close(pipefds[1]);
                infilef = fdopen(pipefds[0], "r");
            }
            if (pid == 0)
            {
                /* in child */
                close(pipefds[0]);
                if (pipefds[1] != 1)
                {
                    dup2(pipefds[1], 1);
                    close(pipefds[1]);
                }
                if (fd != 0)
                {
                    dup2(fd, 0);
                    close(fd);
                }
                if (strcmp(infile+l-3, ".gz")  == 0)
                    execl("/usr/bin/gzip", "gzip", "-dc", NULL);
                else
                    execl("/usr/bin/bzip2", "bzip2", "-dc", NULL);
                close(0);
                close(1);
                errx(1, "cannot exec");
            }
            if (pid < 0)
            {
                /* busted */
                errx(1, "cannot fork");
            }
        }           
        else
            infilef = fopen(infile, "r");

        if (!infilef)
            error_opening:
        errx(1, "cannot open input file");
        pgm_readhdr(qs, infilef);
    }
 
    if (ISSET(MOVIE_OUT))
    {
        if (strcmp(outfile, "-") == 0)
            outfilef = stdout;
        else
            outfilef = fopen(outfile, "w");

        if (!outfilef)
            errx(1, "cannot open output file");
        pgm_writehdr(qs, outfilef);
    }

    if (ISSET(TTY_OUT))
        ttydisp_init(qs,ISSET(TTY_WEB_OUT));

    if (ISSET(XWIN_OUT) && !ISSET(PREVIEW_ONE))
    {
        if (!sizemult)
            sizemult=1;
        xwindisp_init(qs, sizemult);
    }

    if (ISSET(ONE_IN) && ISSET(PREVIEW_ONE))
    {
        qcam_setsize(qs, QC_S80x60);
        if (ISSET(XWIN_OUT))
            xwindisp_init(qs, 1);
        do {
            qcam_scan(qs);
            qcam_adjustpic(qs);
            if (ISSET(TTY_OUT))
                ttydisp(qs,ISSET(TTY_WEB_OUT));
            if (ISSET(XWIN_OUT))
                xwindisp(qs);
        } while (!keyinput());
        qcam_setsize(qs, scan_size);
        qcam_scan(qs);
        pgm_write(qs, outfilef);
    } else if (ISSET(ONE_IN) && !ISSET(MOVIE_IN))
    {
        qcam_setsize(qs, QC_S80x60);
        do {
            qcam_scan(qs);
        } while (qcam_adjustpic(qs));
        qcam_setsize(qs, scan_size);
        qcam_scan(qs);
        if (ISSET(MOVIE_OUT))
            pgm_write(qs, outfilef);
        if (ISSET(TTY_OUT))
            ttydisp(qs,ISSET(TTY_WEB_OUT));
    } else {
        int done = 0;
        if (ISSET(CAMERA_IN) && ISSET(MOVIE_OUT))
        {
            qcam_setsize(qs, QC_S80x60);
            do {
                qcam_scan(qs);
            } while (qcam_adjustpic(qs));
            qcam_setsize(qs, scan_size);
        }
        while (!done)
        {
            if ( sleeptime != 0 )
                usleep( sleeptime );
            if (ISSET(MOVIE_IN))
            {
                pgm_read(qs, infilef);
                if (feof(infilef))
                    done++;
            }
            if (ISSET(CAMERA_IN))
            {
                qcam_scan(qs);
                qcam_adjustpic(qs);
            }
            if (ISSET(MOVIE_OUT))
                pgm_write(qs, outfilef);
            if (ISSET(TTY_OUT))
            {
                ttydisp(qs,ISSET(TTY_WEB_OUT));
                if (ISSET(MOVIE_IN))
                    usleep(200000);
            }
            if (ISSET(XWIN_OUT))
            {
                xwindisp(qs);
                if (ISSET(MOVIE_IN))
                    usleep(200000);
            }
            if (ISSET(ONE_IN))
            {
                char dummy;
                done++;
                if (ISSET(MOVIE_IN))
                    read(0, &dummy, 1);
            }
            if (!ISSET(MOVIE_IN) && keyinput())
                done++;
        }
    }
    
    if (ISSET(MOVIE_IN))
        fclose(infilef);
    if (ISSET(MOVIE_OUT))
        fclose(outfilef);
    if (ISSET(XWIN_OUT))
        xwindisp_close();
    if (ISSET(CAMERA_IN))
        qcam_close(qs);

    return 0;
}

static int
keyinput()
{
    fd_set rdfs;
    struct timeval ztv = { 0, 0 };

    FD_ZERO(&rdfs);
    FD_SET(0, &rdfs);
    if (select(1, &rdfs, NULL, NULL, &ztv))
        return 1;
    else
        return 0;
}

#if 1
#define SCANSIZE QC_S320x240
#else
#define SCANSIZE QC_S160x120
#endif


static void
motion_detect(options, outfile, pth, fth)
    u_int options;
    int pth, fth;
    char * outfile;
{
    struct qcam_softc *qs = NULL;
    struct motion_location ml;
    FILE *out;
    time_t last, now;

    qs = qcam_open(QCAM);
    if (qs == NULL)
        errx(1, "cannot access qcam");

    setuid(getuid());

    if (outfile)
    {
        if (strcmp(outfile,"-") == 0)
        {
            if (ISSET(TTY_OUT))
            {
                errx(1, "-t is mutually exclusive with stdout movie file!");
            }
            out = stdout;
        }
        else
            out = fopen(outfile, "w");
        if (out == NULL)
        {
            err(1, "fopen motionlog.pgm");
        }
    }
    else
        out = NULL;

    qcam_setsize(qs, SCANSIZE);
    if (ISSET(TTY_OUT))
        ttydisp_init(qs,ISSET(TTY_WEB_OUT));
    if (ISSET(XWIN_OUT))
        xwindisp_init(qs, 1);
    if (out)
        pgm_writehdr(qs, out);
    qcam_setsize(qs, QC_S80x60);

    do {
        qcam_scan(qs);
    } while (qcam_adjustpic(qs));

    last = time(0);
    while (!keyinput())
    {
        qcam_setsize(qs, SCANSIZE);
        qcam_scan(qs);

        if (qcam_adjustpic(qs))
            continue;

        qcam_detect_motion2(qs, &ml, pth, fth);

#if 0
        /* xxx broken, assumes xwin out */
        dispartial(qs, &ml, out);
#else
        now = time(0);
        if (now != last)
        {
            last = now;
            if (ISSET(XWIN_OUT))
                xwindisp(qs);
            if (ISSET(TTY_OUT))
                ttydisp(qs,ISSET(TTY_WEB_OUT));
        }
        if (ml.detected && out)
            pgm_write(qs, out);
#endif

        usleep(100000);
    }
    if (out)
        fclose(out);
    if (ISSET(XWIN_OUT))
        xwindisp_close();
}

#if 0
static void
dispartial(qs, ml, out)
    struct qcam_softc *qs;
    struct motion_location *ml;
    FILE *out;
{
    int x,y;
    char tmpbuf[320 * 240];
    char *ip, *op;
    char *tmp;

    ip = qs->buffer;
    op = tmpbuf;

    for (y=0; y < qs->y_size; y++)
    {
        for (x=0; x < qs->x_size; x++)
        {
            if ((x >= ml->top.x) && (x <= ml->bottom.x) &&
                (y >= ml->top.y) && (y <= ml->bottom.y) &&
                (ml->detected != 0))
            {
                *op = *ip;
            }
            else
            {
                *op = *ip / 3;
            }

            op++;
            ip++;
        }
    }

    tmp = qs->buffer;
    qs->buffer = tmpbuf;
    if (ISSET(XWIN_OUT))
        xwindisp(qs);
    if (ml->detected && out)
        if (out)
            pgm_write(qs, out);
    qs->buffer = tmp;
}
#endif
