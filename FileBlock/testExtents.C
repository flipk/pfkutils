
#include "FileBlock_iface.H"
#include "FileBlockLocal.C"

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#define MAX_BYTES (16*1024*1024)

int
main(int argc, char ** argv)
{
    int fd, options;

    if (argc != 2)
    {
        fprintf(stderr, "usage: t3 <file>\n");
        return 1;
    }

    options = O_RDWR;
#ifdef O_LARGEFILE
    options |= O_LARGEFILE;
#endif
    fd = open(argv[1], options);
    if ( fd < 0 )
    {
        fprintf(stderr, "open: %s\n", strerror(errno));
        return 1;
    }

    PageIO * pageio = new PageIOFileDescriptor(fd);
    BlockCache * bc = new BlockCache( pageio, MAX_BYTES );
    FileBlockLocal * fbi = new FileBlockLocal(bc);

    fbi->dump_extents();

    delete fbi;
    delete bc;
    delete pageio;

    return 0;
}
