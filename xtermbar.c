
#include <stdio.h>

/* 
 * This program takes the input string and puts it into the 
 * the xterm title.  If you change the value of the first 
 * number in the printf statement, you can determine where
 * the string will go.  You can use either 0, 1, or 2.  0
 * puts it in the title of the xterm; 1 puts it into the
 * the icon name, and 2 does both.
 */ 

int
xtermbar_main( int argc, char ** argv )
{
    char buff[512];

    argv++;
    argc--;
    buff[0] = '\0';

    while (argc--)
    {
        strcat( buff, *argv++ );
        strcat( buff, " " );
    }

    buff[strlen(buff)-1] = '\0';
    printf( "%c]0;%s%c", (char)27, buff, (char)7 );
    printf( "%c]1;%s%c", (char)27, buff, (char)7 );
    fflush(stdout);

    return 0;
}
