/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
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
