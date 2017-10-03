/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

/* the purpose of this program is to examine a disk device
   and determine the number of sectors it possesses (since
   you can't just query the size of a disk device by any
   sort of ioctl or lseek that i know of) by attempting 
   reads from different sector numbers until we find
   sector numbers that we can't access and then back-pedalling. */

int
main( int argc, char ** argv )
{
	char buf[512];
	int e, fd, cc;
	off_t cur_sect, next_sect, incr_sect;
	fd = open( argv[1], O_RDONLY );
	if ( fd < 0 )
	{
		e = errno;
		fprintf( stderr, "open %s: %d (%s)\n",
			 argv[1], e, strerror( e ));
		return 1;
	}
	incr_sect = (1ULL * 1024ULL * 1024ULL * 1024ULL) / 512ULL;
	cur_sect = 0;
	while ( 1 )
	{
		next_sect = cur_sect + incr_sect;
		lseek( fd, (next_sect * (off_t)512), SEEK_SET );
		errno = 0;
		cc = read( fd, buf, 512 );
//		printf( "incr=%lld, sect=%lld --> %d, errno=%d\n",
//			incr_sect, next_sect, cc, errno );
		if ( cc == 512 )
		{
			cur_sect = next_sect;
		}
		else
		{
			if ( incr_sect <= 1 )
				break;
			incr_sect /= 2;
		}
	}
	printf( "the size is %lld\n", (unsigned long long)(cur_sect+1) );
	close( fd );
	return 0;
}
