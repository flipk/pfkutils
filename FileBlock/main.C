#if 0
files="FileBlockLocal.C FileBlockLocalCache.C PageCache.C"
set -e
set -x
g++ -g3 -I../h -I../dll2 -Wall -Werror $files main.C ../dll2/dll2_hash.C
exit 0
    ;
#endif

#include "PageCache.H"

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

class myIO : public PageIO {
    int fd;
public:
    myIO(char *fname) {
        fd = open(fname, O_RDWR | O_CREAT, 0644);
        if (fd < 0)
        {
            fprintf(stderr, "unable to open file\n");
            exit( 1 );
        }
    }
    ~myIO(void) {
        close(fd);
    }
    // return false if some error occurred
    bool get_page( PageCachePage * pg ) {
        lseek(fd, pg->get_page_number() * PageCache::PAGE_SIZE, SEEK_SET);
        read(fd, pg->ptr, PageCache::PAGE_SIZE);
        printf("read page %d\n", pg->get_page_number());
        return true;
    }
    bool put_page( PageCachePage * pg ) {
        lseek(fd, pg->get_page_number() * PageCache::PAGE_SIZE, SEEK_SET);
        write(fd, pg->ptr, PageCache::PAGE_SIZE);
        printf("write page %d\n", pg->get_page_number());
        return true;
    }

};

int
main()
{
    PageIO      * io;
    PageCache   * pc;
    PageCachePage * p;

    io = new myIO("testfile.db");
    pc = new PageCache(io, 15);

    p = pc->get( 4 );
    sprintf( (char*) p->ptr, "This is a test 4\n");
    pc->unlock(p, true);

    p = pc->get( 3 );
    sprintf( (char*) p->ptr, "This is a test 3\n");
    pc->unlock(p, true);

    delete pc;
    delete io;

    return 0;
}
