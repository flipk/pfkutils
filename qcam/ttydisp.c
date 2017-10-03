#include "headers.h"

static const char *dispchars = " .:-+=#";

void
ttydisp_init( struct qcam_softc *qs )
{
    printf("%c[H%c[J", 27, 27);
}

void
ttydisp( struct qcam_softc *qs )
{
    int xs = qs->x_size;
    int ys = qs->y_size;
    char *buf = qs->buffer;
    int x, y = 0, xsc, ysc, xoff, yoff;

    xsc = xs / 80;
    ysc = ys / 25;
    printf("%c[H", 27);
    switch (ys) {
    case  60: y = 5; break;
    case 120: y = 2; break;
    case 240: y = 6; break;
    }

    for (yoff = 0; yoff < 25; yoff++)
    {
        for (x = xoff = 0; xoff < 80; xoff++)
        {
            putchar(dispchars[buf[y*xs+x] / 10]);
            x += xsc;
        }
        y += ysc;
    }

    fflush(stdout);
}
