#if 0
files="PageCache.C BlockCache.C FileBlockLocal.C FileBlockLocalCache.C ../dll2/dll2_hash.C main.C"
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

#include "BlockCache.H"

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
    BlockCacheBlock * bcb1, * bcb2, * bcb3;

    fd = open("testfile.db", O_RDWR | O_CREAT, 0644);
    if (fd < 0)
    {
        fprintf(stderr, "unable to open file\n");
        exit( 1 );
    }

    io = new PageIOFileDescriptor(fd);
    bc = new BlockCache(io, 12*1024*1024);

    if (strcmp(argv[1],"make")==0)
    {
        bcb1 = bc->get( 500, 5000, true );
        memset(bcb1->get_ptr(), 0xee, 5000);
        bcb1->mark_dirty();

        bcb2 = bc->get( 5700, 100, true );
        memset(bcb2->get_ptr(), 0xff, 100);
        bcb2->mark_dirty();

        bcb3 = bc->get( 6050, 500, true );
        memset(bcb3->get_ptr(), 0xee, 500);
        bcb3->mark_dirty();

        bc->flush();

        bc->release(bcb1,true);
        bc->release(bcb2,true);
        bc->release(bcb3,true);

        bc->flush();
    }
    else if (strcmp(argv[1],"get")==0)
    {
        char buf[5000];
        memset(buf,0xee,5000);
        bcb1 = bc->get( 500, 5000, false );
        printf("memcmp 0xee returns %d\n",
               memcmp(buf,bcb1->get_ptr(),5000));
        bc->release(bcb1,false);
    }

    delete bc;
    delete io;
    close(fd);

    return 0;
}
