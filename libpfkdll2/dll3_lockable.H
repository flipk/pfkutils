
#ifndef __DLL3_LOCKABLE_H__
#define __DLL3_LOCKABLE_H__

namespace DLL3 {

struct LockableError {
    enum LockableErrValue {
        MUTEX_LOCKED_IN_DESTRUCTOR,
        RECURSION_ERROR
    } err;
    LockableError(LockableErrValue _e) : err(_e) { }
};

#define LOCKABLERR(e) throw LockableError(LockableError::e)

class Lockable {
    friend class Lock;
    pthread_mutex_t  mutex;
    bool locked;
    void   lock(void) throw ();
    void unlock(void) throw ();
public:
    Lockable(void) throw ();
    ~Lockable(void) throw (LockableError);
    bool isLocked(void) throw ();
};

class Lock {
    Lockable *obj;
    int lockCount;
public:
    Lock( Lockable *_obj, bool doLock=true )  throw ();
    ~Lock(void) throw ();
    void   lock(void) throw ();
    void unlock(void) throw (LockableError);
};

// TCC impl below this line

inline Lockable::Lockable(void) throw ()
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutex_init(&mutex, &attr);
    pthread_mutexattr_destroy(&attr);
    locked = false;
}

inline Lockable::~Lockable(void) throw (LockableError)
{
    if (locked)
        LOCKABLERR(MUTEX_LOCKED_IN_DESTRUCTOR);
    pthread_mutex_destroy(&mutex);
}

inline void Lockable :: lock(void) throw ()
{
    pthread_mutex_lock  (&mutex);
    locked = true;
}

inline void Lockable :: unlock(void) throw ()
{
    locked = false;
    pthread_mutex_unlock(&mutex);
}

inline bool Lockable :: isLocked(void) throw ()
{
    return locked;
}

inline Lock :: Lock( Lockable *_obj, bool doLock/*=true*/ ) throw ()
    : obj(_obj)
{
    lockCount = 0;
    if (doLock)
    {
        lockCount++;
        obj->lock();
    }
}

inline Lock :: ~Lock(void) throw ()
{
    if (lockCount > 0)
        obj->unlock();
}

inline void Lock :: lock(void) throw ()
{
    if (lockCount++ == 0)
        obj->lock();
}

inline void Lock :: unlock(void) throw (LockableError)
{
    if (lockCount <= 0)
        LOCKABLERR(RECURSION_ERROR);
    if (--lockCount == 0)
        obj->unlock();
}

#undef  LOCKABLERR

}; // namespace DLL3

#endif /* __DLL3_LOCKABLE_H__ */
