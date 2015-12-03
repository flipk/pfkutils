
#include "file_obj.H"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>

uint32_t MAX_NUM_FILES;
uint32_t MAX_FILE_SIZE;
uint32_t MAX_ITERATIONS;
uint32_t MAX_THREADS;

uint32_t seconds = 0;
uint32_t total_files = 0;
uint64_t total_bytes = 0;

struct threadstats {
    threadstats(void) {
        init();
    }
    void init(void) {
        creations = 0;
        validations = 0;
        deletions = 0;
    }
    uint64_t creations;
    uint64_t validations;
    uint64_t deletions;
    void operator+=(const threadstats &other) {
        creations += other.creations;
        validations += other.validations;
        deletions += other.deletions;
    }
    void operator-=(const threadstats &other) {
        creations -= other.creations;
        validations -= other.validations;
        deletions -= other.deletions;
    }
    void operator=(const threadstats &other) {
        creations = other.creations;
        validations = other.validations;
        deletions = other.deletions;
    }
    void print(void) {
        printf("%5d: f %8d b %11lld c %5lld v %5lld d %5lld\n",
               seconds, total_files, total_bytes,
               creations, validations, deletions);
        seconds ++;
    }
};

pthread_mutex_t fils_mutex;
file_obj * fils;
threadstats * allstats;
int running_count = 0;
bool stopping = false;

void * worker( void * arg );


void
signal_handler(int sig)
{
    stopping = true;
}

int
main(int argc, char ** argv)
{
    if (argc != 5)
    {
        printf("usage: diskloader FILES FILESIZE ITERATIONS THREADS\n");
        return 1;
    }

    MAX_NUM_FILES = atoi(argv[1]);
    MAX_FILE_SIZE = atoi(argv[2]);
    MAX_ITERATIONS = atoi(argv[3]);
    MAX_THREADS = atoi(argv[4]);

#define SCALE(size,unit) \
    if (size > 10000) { size /= 1000; unit = 'K'; } \
    if (size > 10000) { size /= 1000; unit = 'M'; } \
    if (size > 10000) { size /= 1000; unit = 'G'; }

    {
        uint64_t filesizeval;
        uint64_t totalsizeval;
        char fileunit = ' ';
        char totalunit = ' ';

        filesizeval = ((uint64_t)MAX_FILE_SIZE / 2);
        totalsizeval = ((uint64_t)MAX_NUM_FILES * 2 / 3) * filesizeval;

        SCALE(filesizeval,fileunit);
        SCALE(totalsizeval,totalunit);

        printf("%d files avgsize %lld %c total %lld %c bytes %d wraps\n",
               MAX_NUM_FILES * 2 / 3,
               filesizeval, fileunit,
               totalsizeval, totalunit,
               (MAX_THREADS * MAX_ITERATIONS) / MAX_NUM_FILES);
    }

    sleep(1);

    int ind;

    fils = new file_obj[MAX_NUM_FILES];
    allstats = new threadstats[MAX_THREADS];

    signal(SIGINT, signal_handler);
    signal(SIGQUIT, signal_handler);
    signal(SIGTERM, signal_handler);

    for (ind = 0; ind < MAX_NUM_FILES; ind++)
        fils[ind].init(ind);

    pthread_mutexattr_t mattr;
    pthread_mutexattr_init(&mattr);
    pthread_mutex_init(&fils_mutex, &mattr);
    pthread_mutexattr_destroy(&mattr);

    stopping = false;

    file_obj::create_directories(MAX_NUM_FILES);

    pthread_attr_t  pattr;
    pthread_attr_init( &pattr );
    pthread_t id;
    for (ind = 0; ind < MAX_THREADS; ind++)
        pthread_create(&id, &pattr, worker,
                       (void*) &allstats[ind]);
    pthread_attr_destroy( &pattr );

    sleep(1);
    threadstats now, last;
    while (running_count > 0)
    {
        sleep(1);
        now.init();
        for (ind = 0; ind < MAX_THREADS; ind++)
            now += allstats[ind];
        threadstats diff = now;
        diff -= last;
        diff.print();
        last = now;
    }
    delete[] allstats;
    delete[] fils;

    file_obj::destroy_directories(MAX_NUM_FILES);

    return 0;
}

#define   lock() pthread_mutex_lock  ( &fils_mutex )
#define unlock() pthread_mutex_unlock( &fils_mutex )

void *
worker( void * arg )
{
    int ind, iter;
    threadstats * stats = (threadstats *) arg;

    lock();
    running_count++;
    unlock();

    for (iter = 0; iter < MAX_ITERATIONS; iter++)
    {
        file_obj *f;
        bool try_again;

        if (stopping)
            break;

        do {

            ind = random() % MAX_NUM_FILES;
            f = &fils[ind];
            try_again = false;

            lock();
            if (f->busy)
                try_again = true;
            else
                f->busy = true;
            unlock();

        } while (try_again);

        if (f->exists == false)
        {
            f->create(MAX_FILE_SIZE);
            stats->creations++;
            lock();
            total_files++;
            total_bytes += f->size;
            unlock();
        }
        else
        {
            f->verify();
            stats->validations++;
            if ((random() % 100) > 50)
            {
                f->destroy();
                stats->deletions++;
                lock();
                total_files--;
                total_bytes -= f->size;
                unlock();
            }
        }

        f->busy = false;
    }

    lock();
    running_count--;
    unlock();
}