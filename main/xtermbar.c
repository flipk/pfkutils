
/*
    This file is part of the "pfkutils" tools written by Phil Knaack
    (pfk@pfk.org).
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
