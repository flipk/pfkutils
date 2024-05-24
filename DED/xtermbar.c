/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
*/

#include <stdio.h>
#include <string.h>

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
