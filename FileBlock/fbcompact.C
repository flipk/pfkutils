
#include <stdio.h>
#include <time.h>
#include "FileBlock_iface.H"

// largest allowed free space in a file is 1% or 1MB
// whichever is smaller.
#define MAX_FREE (1*1024*1024)

static UINT32 max_free = 0;

static bool
compaction_status_function(FileBlockStats *stats, void *arg)
{
    if (stats->num_aus < 1000)
        // don't bother compacting a small file.
        return false;
    if (max_free == 0)
    {
        max_free = stats->num_aus / 100;
        if (max_free > (MAX_FREE / stats->au_size))
            max_free = (MAX_FREE / stats->au_size);
    }
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
