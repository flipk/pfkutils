
/*
    This file is part of the "pkutils" tools written by Phil Knaack
    (pknaack1@netscape.net).
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
