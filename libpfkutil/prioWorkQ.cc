#if 0
set -e -x
g++ prioWorkQ.cc -D__TEST__ -o pwqtst -lpthread
./pwqtst
exit 0
#endif

#include "prioWorkQ.h"
#include <iostream>

using namespace std;

prioWorkQ :: prioWorkQ(void)
{
    for (int p = 0; p < NUM_PRIOS; p++)
    {
        prioQueue_heads[p] = NULL;
        prioQueue_tails[p] = NULL;
    }
    pthread_mutexattr_t  mattr;
    pthread_mutexattr_init( &mattr );
    pthread_mutex_init( &mutex, &mattr );
    pthread_mutexattr_destroy( &mattr );
    pthread_condattr_t  cattr;
    pthread_condattr_init( &cattr );
    pthread_cond_init( &cond, &cattr );
    pthread_condattr_destroy( &cattr );
}

prioWorkQ :: ~prioWorkQ(void)
{
    for (int p = 0; p < NUM_PRIOS; p++)
        if (prioQueue_heads[p] != NULL)
            cerr << "prio queue " << p << " not empty!!" << endl;
    pthread_mutex_destroy( &mutex );
    pthread_cond_destroy( &cond );
}

void
prioWorkQ :: push(prioWorkQJob * job)
{
    int prio = job->prio;
    job->next = NULL;
    lock();
    if (prioQueue_tails[prio] == NULL)
    {
        prioQueue_heads[prio] = prioQueue_tails[prio] = job;
    }
    else
    {
        prioQueue_tails[prio]->next = job;
        prioQueue_tails[prio] = job;
    }
    unlock();
    pthread_cond_signal(&cond);
}

bool
prioWorkQ :: runOne(bool wait)
{
    lock();
    while (1)
    {
        for (int prio = 0; prio < NUM_PRIOS; prio++)
        {
            if (prioQueue_heads[prio] != NULL)
            {
                prioWorkQJob * job = prioQueue_heads[prio];
                prioQueue_heads[prio] = job->next;
                if (job->next == NULL)
                    prioQueue_tails[prio] = NULL;
                unlock();
                job->job();
                delete job;
                return true;
            }
        }
        if (wait == false)
        {
            unlock();
            return false;
        }
        pthread_cond_wait(&cond, &mutex);
    }
}

#ifdef __TEST__

#include <unistd.h>

class myWorkJob : public prioWorkQJob {
    int a;
public:
    myWorkJob(int _prio, int _a) : prioWorkQJob(_prio) {
        a = _a;
    }
    /*virtual*/ void job(void) {
        cout << "executing job a=" << a << endl;
    }
};

void *
run_work_queue(void *arg)
{
    prioWorkQ * workq = (prioWorkQ *) arg;

    while (1)
        workq->runOne(true);

    return NULL;
}

int
main()
{
    prioWorkQ  workq;

    pthread_t id;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_create(&id, &attr, 
                   run_work_queue, &workq);
    pthread_attr_destroy(&attr);

    workq.push(new myWorkJob(15,1));
    workq.push(new myWorkJob(15,2));
    workq.push(new myWorkJob(15,3));
    workq.push(new myWorkJob(15,4));
    workq.push(new myWorkJob(5,5));
    workq.push(new myWorkJob(5,6));
    workq.push(new myWorkJob(5,7));
    workq.push(new myWorkJob(25,8));
    workq.push(new myWorkJob(25,9));

    sleep(1);

    return 0;
}

#endif /* __TEST__ */
