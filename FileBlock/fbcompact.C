
#include <stdio.h>

#include "FileBlock_iface.H"

int
main(int argc, char ** argv)
{
    if (argc != 2)
    {
        printf("usage: fbcompact <dbfile>\n");
        return 1;
    }

    FileBlockInterface * fbi;

    fbi = FileBlockInterface::openFile(argv[1], 128 * 1024);

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

    fbi->compact(0);

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
