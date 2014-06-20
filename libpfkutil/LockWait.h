/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#ifndef __LOCKABLE_H__
#define __LOCKABLE_H__

#include <pthread.h>
#include <time.h>

class Lockable {
    friend class Waiter;
    friend class Lock;
    pthread_mutex_t  lockableMutex;
    void   lock(void);
    void unlock(void);
public:
    Lockable(void);
    ~Lockable(void);
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

// since a waitable IS a lockable, don't derive from both.
class Waitable : public Lockable {
    friend class Waiter;
    pthread_cond_t  waitableCond;
public:
    Waitable(void);
    ~Waitable(void);
    void waiterSignal(void);
    void waiterBroadcast(void);
};

class Waiter : public Lock {
    Waitable * wobj;
public:
    Waiter(Waitable * _wobj);
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
    pthread_mutex_init(&lockableMutex, &attr);
    pthread_mutexattr_destroy(&attr);
}

inline Lockable::~Lockable(void)
{
    pthread_mutex_destroy(&lockableMutex);
}

inline void
Lockable::lock(void)
{
    pthread_mutex_lock  (&lockableMutex);
}

inline void
Lockable::unlock(void)
{
    pthread_mutex_unlock(&lockableMutex);
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
    pthread_cond_init( &waitableCond, &cattr );
    pthread_condattr_destroy( &cattr );
}

inline Waitable::~Waitable(void)
{
    pthread_cond_destroy( &waitableCond );
}

inline void
Waitable::waiterSignal(void)
{
    pthread_cond_signal(&waitableCond);
}

inline void
Waitable::waiterBroadcast(void)
{
    pthread_cond_broadcast(&waitableCond);
}

inline Waiter::Waiter(Waitable * _wobj)
    : Lock(_wobj), wobj(_wobj)
{
}

inline Waiter::~Waiter(void)
{
}

inline void
Waiter::wait(void)
{
    pthread_cond_wait(&wobj->waitableCond, &lobj->lockableMutex);
}

// return false if timeout, true if signaled
inline bool
Waiter::wait(struct timespec *expire)
{
    int ret = pthread_cond_timedwait(&wobj->waitableCond,
                                     &lobj->lockableMutex, expire);
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
