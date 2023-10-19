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

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "pfkutils_config.h"

int
pfkstat_main( int argc, char ** argv )
{
    struct stat sb;

    printf( "%7s"  // dev
            " %10s" // ino
            " %6s" // mode
            " %4s" // nlnk
            " %4s" // uid
            " %4s" // gid
#if HAVE_STRUCT_STAT_ST_RDEV
            " %7s" // rdev
#endif
            " %7s" // size
#if HAVE_STRUCT_STAT_ST_BLOCKS
            " %4s" // blks
#endif
#if HAVE_STRUCT_STAT_ST_BLKSIZE
            " %5s" // blksz
#endif
#if HAVE_STRUCT_STAT_ST_FLAGS
            " %6s" // flags
#endif
#if HAVE_STRUCT_STAT_ST_GEN
            " %4s" // gen
#endif
            "\n",
            "dev", "ino", "mode", "nlnk",
            "uid", "gid"
#if HAVE_STRUCT_STAT_ST_RDEV
            , "rdev" //xxx
#endif
            , "size"
#if HAVE_STRUCT_STAT_ST_BLOCKS
            , "blks" //xxx
#endif
#if HAVE_STRUCT_STAT_ST_BLKSIZE
            , "blksz" //xxx
#endif
#if HAVE_STRUCT_STAT_ST_FLAGS
            , "flags"
#endif
#if HAVE_STRUCT_STAT_ST_GEN
            , "gen"
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
            printf( "%07x" // dev
                    " %10u" // ino
                    " %6o" // mode
                    " %4u" // nlink
                    " %4d" // uid
                    " %4d" // gid
#if HAVE_STRUCT_STAT_ST_RDEV
                    " %07x" // rdev
#endif
                    " %7d" // size
#if HAVE_STRUCT_STAT_ST_BLOCKS
                    " %4d" // blocks
#endif
#if HAVE_STRUCT_STAT_ST_BLKSIZE
                    " %5d" // blksize
#endif
#if HAVE_STRUCT_STAT_ST_FLAGS
                    " %06x" // flags
#endif
#if HAVE_STRUCT_STAT_ST_GEN
                    " %4d" // gen
#endif
                    "\n",
                    (unsigned int) sb.st_dev,
                    (unsigned int) sb.st_ino, sb.st_mode,
                    (unsigned int) sb.st_nlink,
                    sb.st_uid, sb.st_gid
#if HAVE_STRUCT_STAT_ST_RDEV
                    , (unsigned int) sb.st_rdev
#endif
                    , (int)sb.st_size
#if HAVE_STRUCT_STAT_ST_BLOCKS
                    , (int)sb.st_blocks
#endif
#if HAVE_STRUCT_STAT_ST_BLKSIZE
                    , (int)sb.st_blksize
#endif
#if HAVE_STRUCT_STAT_ST_FLAGS
                    , sb.st_flags
#endif
#if HAVE_STRUCT_STAT_ST_GEN
                    , sb.st_gen
#endif
                );

        argc--;
        argv++;
    }

}
