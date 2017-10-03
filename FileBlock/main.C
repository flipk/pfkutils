#if 0
files="PageCache.C BlockCache.C FileBlockLocal.C ../dll2/dll2_hash.C main.C"
set -e
set -x
rm -f *.o
for f in $files ; do 
  g++ -g3 -I../h -I../dll2 -Wall -Werror -c $f
done
g++ -g3 *.o -o a.out
exit 0
    ;
#endif

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

    fd = open("testfile.db", O_RDWR | O_CREAT, 0644);
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
