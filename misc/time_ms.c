
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
#include <stdlib.h>

int
main( int argc, char ** argv )
{
	unsigned int t, days, hours, minutes, seconds, ms;

	t = strtoul( argv[1], NULL, 10 );
	// t is now in ms
	ms = t % 1000;
	t /= 1000;
	// t is now in s
	printf( "%d s\n", t );
	days = t / 86400;
	t -= (days * 86400);
	printf( "%d s\n", t );
	hours = t / 3600;
	t -= (hours * 3600);
	printf( "%d s\n", t );
	minutes = t / 60;
	t -= (minutes * 60);
	printf( "%d s\n", t );
	seconds = t;
	printf( "%d days %d hours %d minutes %d seconds %d milliseconds\n",
		days, hours, minutes, seconds, ms );
	return 0;
}
