#if 0
files="PageCache.C BlockCache.C FileBlockLocal.C ExtentMap.C"
files="$files ../dll2/dll2_hash.C main.C"
defs="-D_FILE_OFFSET_BITS=64"
opts="-Wall -Werror -g3"
incs="-I../h -I../dll2"
set -e
set -x
rm -f *.o
for f in $files ; do 
  g++ $incs $opts $defs -c $f
done
g++ $opts *.o -o a.out
exit 0
    ;
#endif

/** \file main.C
 * \brief Test harness for FileBlock
 * \author Phillip F Knaack
 *
 * This file is the test harness for the components of FileBlock.
 * It is used to test anything and everything during development. */

#include "FileBlockLocal.H"

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int
main(int argc, char ** argv)
{
    int           fd;
    PageIO      * io;
    BlockCache  * bc;
    FileBlockInterface * fbi;

    fd = open("testfile.db", O_RDWR | O_CREAT | O_LARGEFILE, 0644);
    if (fd < 0)
    {
        fprintf(stderr, "unable to open file\n");
        exit( 1 );
    }

    io = new PageIOFileDescriptor(fd);
    bc = new BlockCache(io, 12*1024*1024);
    fbi = new FileBlockLocal( bc );

    //xxx

    delete fbi;
    delete bc;
    delete io;
    close(fd);

    return 0;
}
