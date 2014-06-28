/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#ifndef __LOCKABLE_H__
#define __LOCKABLE_H__

#include <pthread.h>
#include <time.h>

#include "throwBacktrace.h"

namespace WaitUtil {

struct LockableError : ThrowUtil::ThrowBackTrace {
    enum LockableErrValue {
        MUTEX_LOCKED_IN_DESTRUCTOR,
        RECURSION_ERROR,
        __NUMERRS
    } err;
    static const std::string errStrings[__NUMERRS];
    LockableError(LockableErrValue _e) : err(_e) { }
    const std::string Format(void) const;
};

#define LOCKABLERR(e) throw LockableError(LockableError::e)

class Lockable {
    friend class Waiter;
    friend class Lock;
    pthread_mutex_t  lockableMutex;
    bool   locked;
    void   lock(void) throw ();
    void unlock(void) throw ();
public:
    Lockable(void) throw ();
    ~Lockable(void) throw (LockableError);
    bool isLocked() throw ();
};

class Lock {
    friend class Waiter;
    Lockable *lobj;
    int lockCount;
public:
    Lock( Lockable *_lobj, bool dolock=true );
    ~Lock(void);
    void lock(void) throw ();
    void unlock(void) throw (LockableError);
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

class Semaphore {
    int value;
    Waitable semawait;
public:
    Semaphore(void);
    ~Semaphore(void);
    void init(int init_val);
    void give(void);
    bool take(struct timespec * expire); //return false if timeout
    bool take(void) { return take(NULL); }
};

// inline impl below this line

inline Lockable::Lockable(void) throw ()
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutex_init(&lockableMutex, &attr);
    pthread_mutexattr_destroy(&attr);
    locked = false;
}

inline Lockable::~Lockable(void) throw (LockableError)
{
    if (locked)
        LOCKABLERR(MUTEX_LOCKED_IN_DESTRUCTOR);
    pthread_mutex_destroy(&lockableMutex);
}

inline void
Lockable::lock(void) throw ()
{
    pthread_mutex_lock  (&lockableMutex);
    locked = true;
}

inline void
Lockable::unlock(void) throw ()
{
    locked = false;
    pthread_mutex_unlock(&lockableMutex);
}

inline bool
Lockable::isLocked(void) throw ()
{
    return locked;
}

inline Lock::Lock( Lockable *_lobj, bool dolock /*=true*/ )
    : lobj(_lobj)
{
    lockCount = 0;
    if (dolock)
    {
        lockCount++;
        lobj->lock();
    }
}

inline Lock::~Lock(void)
{
    if (lockCount > 0)
        lobj->unlock();
}

inline void
Lock::lock(void) throw ()
{
    if (lockCount++ == 0)
        lobj->lock();
}

inline void
Lock::unlock(void) throw (LockableError)
{
    if (lockCount <= 0)
        LOCKABLERR(RECURSION_ERROR);
    if (--lockCount == 0)
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

inline Semaphore::Semaphore(void)
{
    value = 0;
}

inline Semaphore::~Semaphore(void)
{
}

inline void
Semaphore::init(int init_val)
{
    value = init_val;
}

inline void
Semaphore::give(void)
{
    {
        Lock  lock(&semawait);
        value++;
    }
    semawait.waiterSignal();
}

inline bool
Semaphore::take(struct timespec * expire)
{
    Waiter  waiter(&semawait);
    while (value <= 0)
    {
        if (expire)
        {
            if (waiter.wait(expire) == false)
                return false;
        }
        else
            waiter.wait();
    }
    value--;
    return true;
}

#undef   LOCKABLERR

}; // namespace HSMWait

#endif /* __LOCKABLE_H__ */
