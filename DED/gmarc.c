
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

static int
copy_file( char * from, char * to )
{
    FILE * fromf, * tof;
    int cc;
    char buf[16384];

    fromf = fopen( from, "r" );
    if ( fromf == NULL )
        return -1;

    tof = fopen( to, "w" );
    if ( tof == NULL )
    {
        int esave = errno;
        fclose( fromf );
        errno = esave;
        return -1;
    }

    while (( cc = fread( buf, 1, sizeof(buf), fromf )) > 0 )
    {
        (void) fwrite( buf, cc, 1, tof );
    }

    fclose( fromf );
    fclose( tof );

    return 0;
}

int
gmarc_main( int argc, char ** argv )
{
    char * file;
    char orig[200];
    struct stat sb;

    if ( argc != 2 )
    {
        printf( "usage: gmarc <file>\n" );
        return 1;
    }

    file = argv[1];
    sprintf( orig, "%s.orig", file );

    if ( stat( orig, &sb ) == 0 )
    {
        char origorig[80];
        printf( "warning: %s already exists\n", orig );
        sprintf( origorig, "%s2", orig );
        if ( rename( orig, origorig ) < 0 )
        {
            printf( "rename %s to %s failed (%s)\n",
                    orig, origorig, strerror( errno ));
            return 1;
        }
    }

    if ( rename( file, orig ) < 0 )
    {
        printf( "rename %s to %s failed (%s)\n",
                file, orig, strerror( errno ));
        return 1;
    }

    if ( copy_file( orig, file ) < 0 )
    {
        printf( "copy_file failed (%s)\n", strerror( errno ));
        return 1;
    }

    return 0;
}
