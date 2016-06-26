/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

inline thread_slinger_message::thread_slinger_message(void)
{
    _slinger_next = NULL;
    _slinger_pool = NULL;
    refcount = 0;
}

inline thread_slinger_message::~thread_slinger_message(void) ALLOW_THROWS
{
    if (_slinger_next != NULL)
        throw ThreadSlingerError(
            ThreadSlingerError::MessageOnListDestructor);
}

inline void
thread_slinger_message::ref(void)
{
    WaitUtil::Lock lock(&refcountlock);
    refcount++;
}

inline void
thread_slinger_message::deref(void)
{
    WaitUtil::Lock lock(&refcountlock);
    refcount--;
    if (refcount <= 0)
    {
        if (_slinger_pool != NULL)
            _slinger_pool->release(this);
        else
            throw ThreadSlingerError(
                ThreadSlingerError::DerefNoPool);
    }
}

//

template <class T>
void thread_slinger_queue<T>::enqueue(T * msg)
{
    _enqueue(msg);
}

template <class T>
T * thread_slinger_queue<T>::dequeue(int uSecs /*=0*/)
{
    return (T *) _dequeue(uSecs);
}

template <class T>
int thread_slinger_queue<T>::get_count(void)
{
    return _get_count();
}

//static
template <class T>
T * thread_slinger_queue<T>::dequeue(
    thread_slinger_queue<T> * queues[],
    int num_queues,
    int uSecs, int *which_queue /*=NULL*/)
{
    return (T *) _dequeue((_thread_slinger_queue **) queues,
                          num_queues, uSecs, which_queue);
}

//

template <class T>
thread_slinger_pool<T>::thread_slinger_pool(void)
{
    usedCount = freeCount = 0;
    thread_slinger_pools::register_pool(this);
}

//virtual
template <class T>
thread_slinger_pool<T>::~thread_slinger_pool(void)
{
    thread_slinger_pools::unregister_pool(this);
}

template <class T>
void thread_slinger_pool<T>::add(int items)
{
    T * item;
    while (items-- > 0)
    {
        item = new T;
        item->_slinger_pool = this;
        release(item);
    }
}

template <class T>
T * thread_slinger_pool<T>::alloc(int uSecs /*=0*/,
                                  bool grow /*=false*/)
{
    T * ret = q.dequeue(uSecs);
    if (ret)
    {
        WaitUtil::Lock  lock(&statsLockable);
        freeCount--;
    }
    if (ret == NULL && grow)
    {
        ret = new T;
    }
    if (ret)
    {
        ret->_slinger_pool = this;
        WaitUtil::Lock  lock(&statsLockable);
        usedCount++;
        if (nameSet == false)
        {
            msgName = ret->msgName();
            nameSet = true;
        }
    }
    return ret;
}

template <class T>
void thread_slinger_pool<T>::release(thread_slinger_message * m)
{
    T * derived = dynamic_cast<T*>(m);
    release(derived);
}

template <class T>
void thread_slinger_pool<T>::release(T * buf)
{
    if (buf->_slinger_pool != this)
        throw ThreadSlingerError(ThreadSlingerError::MessageNotFromThisPool);
    q.enqueue(buf);
    WaitUtil::Lock  lock(&statsLockable);
    usedCount--;
    freeCount++;
}

template <class T>
void thread_slinger_pool<T>::getCounts(int &used, int &free,
                                       std::string &name)
{
    WaitUtil::Lock  lock(&statsLockable);
    used = usedCount;
    free = freeCount;
    name = msgName;
}
