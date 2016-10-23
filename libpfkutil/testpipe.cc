
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "pfkposix.h"

class readerThread : public pfk_pthread
{
    int which;
    int closerPipe[2];
    /*virtual*/ void * entry(void)
    {
        // fifo with O_NONBLOCK cannot be opened for write first,
        // it will fail with error 6 : no such device or address
        // open for read side will open first.
        // but if you open for read, you cannot tell when the other
        // side opened for write. you can only tell when they write
        // some data.

        if (which == 1)
        {
            sleep(1);
            printf("t1 trying open for write\n");
            int fd = open("testfifo", O_WRONLY | O_NONBLOCK);
            if (fd > 0)
                printf("t1 open successful fd %d\n", fd);
            else
                printf("t1 open failed err %d:%s\n", errno, strerror(errno));
            pfk_select sel;
            sel.rfds.set(closerPipe[0]);
            sel.wfds.set(fd);
            sel.tv.set(5,0);
            printf("t1 entering select\n");
            int cc = sel.select();
            printf("t1 select returns %d\n", cc);
            if (cc > 0)
            {
                if (sel.rfds.isset(closerPipe[0]))
                    printf("t1 closer pipe\n");
                if (sel.wfds.isset(fd))
                {
                    printf("t1 select for write\n");
                    sleep(1);
                    char c = 1;
                    printf("t1 writing 1\n");
                    write(fd, &c, 1);
                    printf("t1 writing complete\n");
                }
            }
            sleep(1);
            printf("t1 closing fd %d\n",fd);
            close(fd);
            printf("t1 close complete\n");
            return NULL;
        }
        else if (which == 2)
        {
            printf("t2 trying open for read\n");
            int fd = open("testfifo", O_RDONLY | O_NONBLOCK);
            if (fd > 0)
                printf("t2 open successful fd %d\n", fd);
            else
                printf("t2 open failed err %d:%s\n", errno, strerror(errno));
            pfk_select sel;
            sel.rfds.set(closerPipe[0]);
            sel.rfds.set(fd);
            sel.tv.set(5,0);
            printf("t2 entering select\n");
            int cc = sel.select();
            printf("t2 select returns %d\n", cc);
            if (cc > 0)
            {
                if (sel.rfds.isset(closerPipe[0]))
                    printf("t2 closer pipe\n");
                if (sel.rfds.isset(fd))
                    printf("t2 select for read\n");
            }
            sleep(1);
            printf("t2 closing fd %d\n",fd);
            close(fd);
            printf("t2 close complete\n");
            return NULL;
        }
        
        return NULL;
    }
    /*virtual*/ void send_stop(void)
    {
        char c = 1;
        write(closerPipe[1], &c, 1);
    }
public:
    readerThread(int _which)
    {
        which = _which;
        pipe(closerPipe);
    }
    ~readerThread(void)
    {
        stopjoin();
        close(closerPipe[0]);
        close(closerPipe[1]);
    }
};

int
main()
{
    readerThread   r1(1), r2(2);

    unlink("testfifo");
    mkfifo("testfifo", 0600);

    r1.create();
    r2.create();
    sleep(10);
    r1.join();
    r2.join();

    unlink("testfifo");

    return 0;
}
