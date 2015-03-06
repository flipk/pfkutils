
#include "hsmthread.h"

#include <unistd.h>
#include <vector>

using namespace HSMThread;
using namespace DLL3;
using namespace WaitUtil;

#define THROW(e) throw ThreadError(ThreadError::e)

const std::string ThreadError::errStrings[__NUMERRS] = {
    "thread still running during ~Thread destructor",
    "Thread::start called when thread already started"
    "Threads::start called when thread already started"
};

const std::string
ThreadError::Format(void) const
{
    std::string ret = "THREAD ERROR: ";
    ret += errStrings[err];
    ret += " at:\n";
    ret += BackTrace::Format();
    return ret;
}

Thread::Thread(const std::string &_name)
    : name(_name)
{
    state = NEWBORN;
    Threads::instance()->registerThread(this);
    joined = false;
}

//virtual
Thread::~Thread(void)
{
    if (state == RUNNING)
        THROW(RunningInDestructor);
    join();
    Threads::instance()->unregisterThread(this);
}

void
Thread::start(void)
{
    if (state != NEWBORN)
        THROW(ThreadStartAlreadyStarted);

    pthread_attr_t   attr;

    pthread_attr_init( &attr );
    pthread_create( &id, &attr, &__entry, (void*) this );
    pthread_attr_destroy( &attr );
}

void
Thread::stop(void)
{
    if (state == RUNNING)
        stopReq();
}

void
Thread::join(void)
{
    if (joined == true)
        return;
    void * dummy;
    if (state != NEWBORN)
    {
        pthread_join(id, &dummy);
        joined = true;
    }
}

//static
void *
Thread::__entry(void * arg)
{
    Thread * thr = (Thread *) arg;
    thr->_entry();
    return NULL;
}

void
Thread::_entry(void)
{
    state = RUNNING;
    {
        Threads::startupSync  *syncPt;
        syncPt = &Threads::instance()->syncPt;
        Waiter waiter(&syncPt->slaveWait);
        syncPt->numStarted++;
        waiter.wait();
    }
    entry();
    state = DEAD;
}

Threads * Threads::_instance = NULL;

void
Threads::registerThread(Thread *t)
{
    Lock  lock(&list);
    list.add_tail(t);
}

void
Threads::unregisterThread(Thread *t)
{
    Lock  lock(&list);
    list.remove(t);
}

//static
Threads *
Threads::instance(void)
{
    if (_instance == NULL)
        _instance = new Threads;
    return _instance;
}

Threads::Threads(void)
{
}

Threads::~Threads(void)
{
}

void
Threads::_start(void)
{
    if (state == RUNNING)
        THROW(ThreadsStartAlreadyStarted);

    syncPt.numStarted = 0;
    int numThreads = 0;

    {
        Lock lock(&list);
        for (Thread * t = list.get_head();
             t != NULL;
             t = list.get_next(t))
        {
            t->start();
            numThreads++;
        }
    }

    while (1)
    {
        usleep(1);
        Lock lock(&syncPt.slaveWait);
        if (syncPt.numStarted == numThreads)
            break;
    }

    // all threads have called in, release them.
    syncPt.slaveWait.waiterBroadcast();

    state = RUNNING;
}

void
Threads::_cleanup(void)
{
    std::vector<Thread*> deleteList;
    size_t ind;

    Lock lock(&list);
    for (Thread * t = list.get_head();
         t != NULL;
         t = list.get_next(t))
    {
        t->stop();
        deleteList.push_back(t);
    }
    lock.unlock();

    for (ind = 0; ind < deleteList.size(); ind++)
        deleteList[ind]->join();
}
