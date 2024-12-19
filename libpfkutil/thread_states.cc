
#include "thread_states.h"
#include <sys/mman.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define PAGE_SIZE 4096    /*one system page*/
static int thstates_fd = -1;
thread_states * thstates = NULL;

void thread_states_init(bool init_contents /*=true*/)
{
    thstates_fd = open(THREADSATES_FILE, O_RDWR | O_CREAT, 0666);
    if (thstates_fd < 0)
    {
        fprintf(stderr, "open thread states file: errno %d (%s)\n",
                errno, strerror(errno));
        return;
    }
    if (ftruncate(thstates_fd, PAGE_SIZE) < 0)
    {
        fprintf(stderr, "ftruncate thread states file: errno %d (%s)\n",
                errno, strerror(errno));
        return;
    }
    void * ptr = mmap(NULL, PAGE_SIZE,
                      PROT_READ | PROT_WRITE, MAP_SHARED | MAP_LOCKED,
                      thstates_fd, 0);
    if (ptr == MAP_FAILED)
    {
        fprintf(stderr, "mmap thread states file: errno %d (%s)\n",
                errno, strerror(errno));
        return;
    }
    else
    {
        if (init_contents)
            memset(ptr, 0, PAGE_SIZE);
        thstates = (thread_states *) ptr;
        printf("opened '%s'\n", THREADSATES_FILE);
    }
}

void thread_states_sync(void)
{
    msync((void*) thstates, PAGE_SIZE, MS_SYNC);
}
