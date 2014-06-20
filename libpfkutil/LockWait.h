/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#ifndef __LOCKABLE_H__
#define __LOCKABLE_H__

#include <pthread.h>
#include <time.h>

class Lockable {
    friend class Waiter;
    pthread_mutex_t  mutex;
public:
    Lockable(void);
    ~Lockable(void);
    void   lock(void);
    void unlock(void);
};

class Lock {
    friend class Waiter;
    Lockable *lobj;
    bool iLocked;
public:
    Lock( Lockable *_lobj, bool dolock=true );
    ~Lock(void);
    void lock(void);
    void unlock(void);
};

class Waitable {
    friend class Waiter;
    pthread_cond_t  cond;
public:
    Waitable(void);
    ~Waitable(void);
    void waiterSignal(void);
    void waiterBroadcast(void);
};

class Waiter : public Lock {
    Waitable * wobj;
public:
    Waiter(Lockable * _lobj, Waitable * _wobj);
    ~Waiter(void);
    void wait(void);
    // return false if timeout, true if signaled
    bool wait(struct timespec *expire);
    // return false if timeout, true if signaled
    bool wait(int sec, int nsec);
};

// inline impl below this line

inline Lockable::Lockable(void)
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutex_init(&mutex, &attr);
    pthread_mutexattr_destroy(&attr);
}

inline Lockable::~Lockable(void)
{
    pthread_mutex_destroy(&mutex);
}

inline void
Lockable::lock(void)
{
    pthread_mutex_lock  (&mutex);
}

inline void
Lockable::unlock(void)
{
    pthread_mutex_unlock(&mutex);
}

inline Lock::Lock( Lockable *_lobj, bool dolock /*=true*/ )
    : lobj(_lobj)
{
    if (dolock)
        lobj->lock();
    iLocked = dolock;
}

inline Lock::~Lock(void)
{
    if (iLocked)
        lobj->unlock();
}

inline void
Lock::lock(void)
{
    lobj->lock();
    iLocked = true;
}

inline void
Lock::unlock(void)
{
    iLocked = false;
    lobj->unlock();
}

inline Waitable::Waitable(void)
{
    pthread_condattr_t   cattr;
    pthread_condattr_init( &cattr );
    pthread_cond_init( &cond, &cattr );
    pthread_condattr_destroy( &cattr );
}

inline Waitable::~Waitable(void)
{
    pthread_cond_destroy( &cond );
}

inline void
Waitable::waiterSignal(void)
{
    pthread_cond_signal(&cond);
}

inline void
Waitable::waiterBroadcast(void)
{
    pthread_cond_broadcast(&cond);
}

inline Waiter::Waiter(Lockable * _lobj, Waitable * _wobj)
    : Lock(_lobj), wobj(_wobj)
{
}

inline Waiter::~Waiter(void)
{
}

inline void
Waiter::wait(void)
{
    pthread_cond_wait(&wobj->cond, &lobj->mutex);
}

// return false if timeout, true if signaled
inline bool
Waiter::wait(struct timespec *expire)
{
    int ret = pthread_cond_timedwait(&wobj->cond, &lobj->mutex, expire);
    if (ret != 0)
        return false;
    return true;
}

// return false if timeout, true if signaled
inline bool
Waiter::wait(int sec, int nsec)
{
    if (nsec > 1000000000)
    {
        sec += nsec / 1000000000;
        nsec = nsec % 1000000000;
    }
    struct timespec  expire;
    clock_gettime(CLOCK_REALTIME, &expire);
    expire.tv_sec += sec;
    expire.tv_nsec += nsec;
    if (expire.tv_nsec > 1000000000)
    {
        expire.tv_nsec -= 1000000000;
        expire.tv_sec += 1;
    }
    return wait(&expire);
}

#endif /* __LOCKABLE_H__ */
