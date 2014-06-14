
struct LockableError {
    enum LockableErrValue {
        MUTEX_LOCKED_IN_DESTRUCTOR
    } err;
    LockableError(LockableErrValue _e) : err(_e) { }
};

#define LOCKABLERR(e) LockableError(LockableError::e)

class Lockable {
    pthread_mutex_t  mutex;
    bool locked;
public:
    Lockable(void) throw ();
    ~Lockable(void) throw (LockableError);
    void   lock(void) throw ();
    void unlock(void) throw ();
    bool isLocked(void) throw ();
};

class Lock {
    Lockable *obj;
public:
    Lock( Lockable *_obj )  throw ();
    ~Lock(void) throw ();
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
        throw LOCKABLERR(MUTEX_LOCKED_IN_DESTRUCTOR);
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

inline Lock :: Lock( Lockable *_obj ) throw ()
    : obj(_obj)
{
    obj->lock();
}

inline Lock :: ~Lock(void) throw ()
{
    obj->unlock();
}

#undef  LOCKABLERR
