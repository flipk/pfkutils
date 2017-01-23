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
#include <time.h>
#include "FileBlock_iface.h"

static uint32_t max_free = 0;

static bool
compaction_status_function(FileBlockStats *stats, void *arg)
{
    if (stats->num_aus < 1000)
        // don't bother compacting a small file.
        return false;
    if (max_free == 0)
        max_free = stats->num_aus / 100;
    static time_t last = 0;
    if (stats->free_aus <= max_free)
        return false;
    time_t now = time(NULL);
    if (now != last)
    {
        printf("free aus: %d (max %d)\n", stats->free_aus, max_free);
        last = now;
    }
    return true;
}

extern "C" int
fbcompact_main(int argc, char ** argv)
{
    if (argc != 2)
    {
        printf("usage: fbcompact <dbfile>\n");
        return 1;
    }

    FileBlockInterface * fbi;

    fbi = FileBlockInterface::openFile(argv[1], 128 * 1024 * 1024);

    if (!fbi)
    {
        printf("unable to open file %s\n", argv[1]);
        return 1;
    }

    FileBlockStats stats;

    fbi->get_stats( &stats );
    printf("stats before:\n"
           "AU size : %d\n"
           "Used AUs : %d\n"
           "Free AUs : %d\n"
           "Used Regions : %d\n"
           "Free Regions : %d\n"
           "Num AUs : %d\n",
           stats.au_size,
           stats.used_aus,
           stats.free_aus,
           stats.used_regions,
           stats.free_regions,
           stats.num_aus);

    printf("\npacking...\n");

    fbi->compact(compaction_status_function, NULL);

    fbi->get_stats( &stats );
    printf("\nstats after:\n"
           "AU size : %d\n"
           "Used AUs : %d\n"
           "Free AUs : %d\n"
           "Used Regions : %d\n"
           "Free Regions : %d\n"
           "Num AUs : %d\n",
           stats.au_size,
           stats.used_aus,
           stats.free_aus,
           stats.used_regions,
           stats.free_regions,
           stats.num_aus);

    delete fbi;
    return 0;
}
