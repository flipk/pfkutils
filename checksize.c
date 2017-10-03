
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

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
	printf( "the size is %lld\n", cur_sect+1 );
	close( fd );
	return 0;
}
