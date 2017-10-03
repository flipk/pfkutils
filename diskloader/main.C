
#include "config.h"
#include "file_obj.H"

#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t fils_mutex;
file_obj  fils[MAX_NUM_FILES];
int running_count = 0;

void * worker( void * arg );

int
main()
{
    int ind;
    for (ind = 0; ind < MAX_NUM_FILES; ind++)
        fils[ind].init(ind);

    pthread_mutexattr_t mattr;
    pthread_mutexattr_init(&mattr);
    pthread_mutex_init(&fils_mutex, &mattr);
    pthread_mutexattr_destroy(&mattr);

    pthread_attr_t  pattr;
    pthread_attr_init( &pattr );
    pthread_t id;
    for (ind = 0; ind < MAX_THREADS; ind++)
        pthread_create(&id, &pattr, worker, NULL);
    pthread_attr_destroy( &pattr );

    sleep(1);
    while (running_count > 0)
        sleep(1);

    return 0;
}

void *
worker( void * arg )
{
    int ind, iter;

    pthread_mutex_lock( &fils_mutex );
    running_count++;
    pthread_mutex_unlock( &fils_mutex );

    for (iter = 0; iter < MAX_ITERATIONS; iter++)
    {
        file_obj *f;
        bool try_again;

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
            f->create();
        }
        else
        {
            f->verify();
            if ((random() % 100) > 50)
                f->destroy();
        }

        f->busy = false;
    }

    pthread_mutex_lock( &fils_mutex );
    running_count--;
    pthread_mutex_unlock( &fils_mutex );
}
