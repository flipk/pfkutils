/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

int
pkfstat_main( int argc, char ** argv )
{
    struct stat sb;

    printf( "%7s %10s %6s %4s %4s %4s %7s %7s %4s %5s"
#if !defined(SOLARIS) && !defined(CYGWIN)
            " %6s %4s"
#endif
            "\n",
            "dev", "ino", "mode", "nlnk",
            "uid", "gid", "rdev", "size",
            "blks", "blksz"
#if !defined(SOLARIS) && !defined(CYGWIN)
            , "flags", "gen"
#endif
        );
    printf( "----------------------------------------"
            "---------------------------------------\n" );

    argv++;
    argc--;

    while ( argc > 0 )
    {
        printf( "%s\n", argv[0] );

        if ( stat( argv[0], &sb ) < 0 )
            printf( "stat failed: %s\n", strerror( errno ));
        else
            printf( "%07x %10d %6o %4d %4d %4d %07x %7d %4d %5d"
#if !defined(SOLARIS) && !defined(CYGWIN)
                    " %06x %4d"
#endif
                    "\n",
                    sb.st_dev, sb.st_ino, sb.st_mode, sb.st_nlink,
                    sb.st_uid, sb.st_gid, sb.st_rdev, (int)sb.st_size,
                    (int)sb.st_blocks, sb.st_blksize
#if !defined(SOLARIS) && !defined(CYGWIN) && !defined(LINUX)
                    , sb.st_flags, sb.st_gen
#endif
                );

        argc--;
        argv++;
    }

}
