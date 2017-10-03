#if 0
#files="PageCache.C BlockCache.C FileBlockLocal.C ../dll2/dll2_hash.C main.C"
files="ExtentMap.C ../dll2/dll2_hash.C main.C"
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

#include "ExtentMap.H"

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int
main()
{
    Extents   extents;
    Extent  * e;
    UINT32 id1, id2, id3, id4, id5;

    extents.add( (off_t) 0, (UINT32) 16384 );

    e = extents.alloc(512);
    id1 = e->id;
    printf("allocated id %d, %lld (%d)\n", e->id, e->offset, e->size);
    e = extents.find(e->id);
    printf("located id %d, %lld (%d)\n", e->id, e->offset, e->size);
    
    printf("after 1 allocs:\n");
    extents.print();

    e = extents.alloc(64);
    id2 = e->id;
    printf("allocated id %d, %lld (%d)\n", e->id, e->offset, e->size);
    e = extents.find(e->id);
    printf("located id %d, %lld (%d)\n", e->id, e->offset, e->size);
    
    printf("after 2 allocs:\n");
    extents.print();

    e = extents.alloc(128);
    id3 = e->id;
    printf("allocated id %d, %lld (%d)\n", e->id, e->offset, e->size);
    e = extents.find(e->id);
    printf("located id %d, %lld (%d)\n", e->id, e->offset, e->size);
    
    printf("after 3 allocs:\n");
    extents.print();

    e = extents.alloc(32);
    id4 = e->id;
    printf("allocated id %d, %lld (%d)\n", e->id, e->offset, e->size);
    e = extents.find(e->id);
    printf("located id %d, %lld (%d)\n", e->id, e->offset, e->size);
    
    printf("after 4 allocs:\n");
    extents.print();

    e = extents.alloc(96);
    id5 = e->id;
    printf("allocated id %d, %lld (%d)\n", e->id, e->offset, e->size);
    e = extents.find(e->id);
    printf("located id %d, %lld (%d)\n", e->id, e->offset, e->size);
    
    printf("after 5 allocs:\n");
    extents.print();

    extents.free(id2);
    printf("after 1 frees:\n");
    extents.print();
    extents.free(id4);
    printf("after 2 frees:\n");
    extents.print();
    extents.free(id5);
    printf("after 3 frees:\n");
    extents.print();
    extents.free(id3);
    printf("after 4 frees:\n");
    extents.print();
    extents.free(id1);
    printf("after 5 frees:\n");
    extents.print();

}

#if 0
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
#endif
