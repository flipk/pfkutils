/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

#include "headers.h"

static const char *dispchars = " .:-+=#";

void
ttydisp_init( struct qcam_softc *qs, int web )
{
    if ( !web )
        printf("%c[H%c[J", 27, 27);
}

void
ttydisp( struct qcam_softc *qs, int web )
{
    int xs = qs->x_size;
    int ys = qs->y_size;
    char *buf = qs->buffer;
    int x, y = 0, xsc, ysc, xoff, yoff;

    xsc = xs / 80;
    ysc = ys / 25;
    if ( web )
        printf(
"<html>\n"
"<body alink=\"#000099\" vlink=\"#990099\" link=\"#000099\"\n"
"style=\"color: rgb(255, 255, 255); background-color: rgb(0, 0, 0);\">\n"
"<pre style=\"font-family: monospace;\"><font size=\"-1\">\n"
            );
    else
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
        if ( web )
            putchar( '\n' );
        y += ysc;
    }

    if ( web )
        printf( "</pre></body></html>\n" );

    fflush(stdout);
}
