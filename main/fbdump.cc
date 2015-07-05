
#include <stdio.h>

#include "FileBlock_iface.h"
#include "FileBlockLocal.h"

extern "C" int
fbdump_main(int argc, char ** argv)
{
    if (argc != 2)
    {
        printf("usage: fbdump <dbfile>\n");
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

    printf("stats:\n"
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

    FileBlockLocal * fbl = (FileBlockLocal*) fbi;

    fbl->validate(true);

    delete fbi;
    return 0;
}
