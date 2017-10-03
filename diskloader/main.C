
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
        printf("creations %5d validations %5d deletions %5d\n",
               creations, validations, deletions);
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

void *
worker( void * arg )
{
    int ind, iter;
    threadstats * stats = (threadstats *) arg;

    pthread_mutex_lock( &fils_mutex );
    running_count++;
    pthread_mutex_unlock( &fils_mutex );

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

            pthread_mutex_lock( &fils_mutex );
            if (f->busy)
                try_again = true;
            else
                f->busy = true;
            pthread_mutex_unlock( &fils_mutex );

        } while (try_again);

        if (f->exists == false)
        {
            f->create(MAX_FILE_SIZE);
            stats->creations++;
        }
        else
        {
            f->verify();
            stats->validations++;
            if ((random() % 100) > 50)
            {
                f->destroy();
                stats->deletions++;
            }
        }

        f->busy = false;
    }

    pthread_mutex_lock( &fils_mutex );
    running_count--;
    pthread_mutex_unlock( &fils_mutex );
}
