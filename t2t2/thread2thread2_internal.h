
#ifndef __T2T2_INCLUDE_INTERNAL__

#error "This file is meant to be included only by thread2thread2.h"

#elif __T2T2_INCLUDE_INTERNAL__ == 1

// hopefully we're already inside namespace Thread2Thread2

//////////////////////////// SAFETY STUFF ////////////////////////////

#define __T2T2_EVIL_CONSTRUCTORS(T)              \
    private:                                    \
    T(T&&) = delete;                            \
    T(const T&) = delete;                       \
    T &operator=(const T&) = delete;            \
    T &operator=(T&) = delete

#define __T2T2_EVIL_NEW(T)                       \
    static void * operator new(size_t sz) = delete

#define __T2T2_EVIL_DEFAULT_CONSTRUCTOR(T)       \
    private:                                    \
    T(void) = delete;

//////////////////////////// TRAITS STUFF ////////////////////////////

// note that largest_type not only calculates the size
// of the largest type, but also verifies all the derivedTs
// are actually derived from the BaseT.
template <typename BaseT, typename... Ts>
struct largest_type;

template <typename BaseT>
struct largest_type<BaseT>
{
    using type = BaseT;
    static const int size = sizeof(type);
};

template <typename BaseT, typename derivedT>
struct largest_type<BaseT, derivedT>
{
    static_assert(std::is_base_of<BaseT, derivedT>::value == true,
                  "derivedTs must be derived from BaseT");
    using type = derivedT;
    static const int size = sizeof(type);
};

template <typename BaseT, typename T, typename U, typename... Ts>
struct largest_type<BaseT, T, U, Ts...>
{
    using type =
        typename largest_type<BaseT,
        typename std::conditional<
            (sizeof(U) <= sizeof(T)), T, U
            >::type, Ts...
        >::type;
    static const int size = sizeof(type);
};

//////////////////////////// ERROR HANDLING ////////////////////////////

#define __T2T2_ASSERT(err,fatal) \
    t2t2_assert_handler(t2t2_error_t::err,fatal, __FILE__, __LINE__)

//////////////////////////// __T2T2_LINKS ////////////////////////////

// this can't really have a constructor because it gets pointer-cast
// all over the place.
template <class T>
struct __t2t2_links
{
    __t2t2_links<T> * next;
    __t2t2_links<T> * prev;
    __t2t2_links<T> * list;
    static const uint32_t LINKS_MAGIC = 0x1e3909f2;
    uint32_t  magic;
    bool ok(void) const
    {
        // if this macro is not defined, this function
        // becomes effectively a no-op, and the compiler will
        // optimize it away completely.
        if (magic != LINKS_MAGIC)
        {
            __T2T2_ASSERT(LINKS_MAGIC_CORRUPT,true);
            // should not be reached, if assert(fatal=true) does it's job...
            return false;
        }
        return true;
    }
    void init(void)
    {
        next = prev = this;
        list = NULL;
        magic = LINKS_MAGIC;
    }
    // a pool should be a stack to keep caches hot, so
    // push to the same end you pop from.
    void add_next(T *item)
    {
        ok();
        if (item->list != NULL)
        {
            __T2T2_ASSERT(LINKS_ADD_ALREADY_ON_LIST,true);
        }
        item->next = next;
        item->prev = this;
        next->prev = item;
        next = item;
        item->list = this;
    }
    // a queue should be a fifo to keep msgs in order, so
    // push to the back of the list and pop from the front.
    void add_prev(T *item)
    {
        ok();
        if (item->list != NULL)
        {
            __T2T2_ASSERT(LINKS_ADD_ALREADY_ON_LIST,true);
        }
        item->next = this;
        item->prev = prev;
        prev->next = item;
        prev = item;
        item->list = this;
    }
    bool validate(T *item)
    {
        return (ok() && (item->list == this));
    }
    void remove(void)
    {
        ok();
        if (list == NULL)
        {
            __T2T2_ASSERT(LINKS_REMOVE_NOT_ON_LIST,true);
        }
        list = NULL;
        next->prev = prev;
        prev->next = next;
        next = prev = this;
    }
    T * get_next(void)
    {
        ok();
        return (T*) next;
    }
};

//////////////////////////// __T2T2_BUFFER_HDR ////////////////////////////

struct __t2t2_buffer_hdr : public __t2t2_links<__t2t2_buffer_hdr>
{
    void init(void)
    {
        __t2t2_links::init();
    }
} __attribute__ ((aligned (sizeof(void*))));

static_assert(std::is_trivial<__t2t2_buffer_hdr>::value == true,
              "class __t2t2_buffer_hdr must always be trivial");

///////////////////////// __T2T2_LINKS_HEAD /////////////////////////

// this does have a constructor because it gets properly instantiated
// as a normal class member.  a links head is a specialization of a
// links, because "next" is actually the head of the list, "prev"
// is the tail, and it has an "empty" primitive. when the list head
// is empty, next and prev point to head(); also the first item's
// prev and the last item's next both point to head.head().
template <class T>
struct __t2t2_links_head : public __t2t2_links<T>
{
    __t2t2_links_head(void) { __t2t2_links<T>::init(); }
    bool empty(void) const
    {
        __t2t2_links<T>::ok();
        return ((__t2t2_links<T>::next == this) &&
                (__t2t2_links<T>::prev == this));
    }
    T * head(void)
    {
        return (T*) this;
    }
    T * get_head(void)
    {
        __t2t2_links<T>::ok();
        return (T*) __t2t2_links<T>::next;
    }
    T * get_tail(void)
    {
        __t2t2_links<T>::ok();
        return (T*) __t2t2_links<T>::prev;
    }
};

//////////////////////////// __T2T2_TIMESPEC ////////////////////////////

struct __t2t2_timespec : public timespec
{
    __t2t2_timespec(void) { }
    __t2t2_timespec(int ms)
    {
        tv_sec = ms / 1000;
        tv_nsec = (ms % 1000) * 1000000;
    }
    void getNow(clockid_t clk_id)
    {
        clock_gettime(clk_id, this);
    }
    __t2t2_timespec &operator+=(const timespec &rhs)
    {
        tv_sec += rhs.tv_sec;
        tv_nsec += rhs.tv_nsec;
        if (tv_nsec > 1000000000)
        {
            tv_nsec -= 1000000000;
            tv_sec += 1;
        }
        return *this;
    }
};

//////////////////////////// __T2T2_QUEUE ////////////////////////////

class __t2t2_queue : public __t2t2_links<__t2t2_queue>
{
    pthread_mutex_t   mutex;
    pthread_cond_t    cond;

    // these two pointers must only be accessed
    // or changed with &mutex locked.
    pthread_mutex_t * psetmutex;
    pthread_cond_t  * psetcond;

    clockid_t         clk_id;
    __t2t2_links_head<__t2t2_buffer_hdr> buffers;
    class Lock {
        pthread_mutex_t *m;
    public:
        Lock(pthread_mutex_t *_m) : m(_m) { pthread_mutex_lock(m); }
        ~Lock(void) { pthread_mutex_unlock(m); }
    };
    friend class __t2t2_queue_set;
    int id;
    void set_pmutexpcond(pthread_mutex_t *nm = NULL,
                         pthread_cond_t  *nc = NULL)
    {
        Lock l(&mutex);
        psetmutex = nm;
        psetcond = nc;
    }
    bool _validate(__t2t2_buffer_hdr *h) { return buffers.validate(h); }
public:
    __t2t2_queue(void);
    ~__t2t2_queue(void);

    bool _empty(void);

    // -1 = T2T2_WAIT_FOREVER : wait forever
    //  0 = T2T2_NO_WAIT      : dont wait, just return
    // >0                     : wait for some number of mS
    __t2t2_buffer_hdr *_dequeue(int wait_ms);
    // a pool should be a stack, to keep caches hotter.
    bool _enqueue(__t2t2_buffer_hdr *h);
    // a queue should be a fifo, to keep msgs in order.
    bool _enqueue_tail(__t2t2_buffer_hdr *h);

    // return true if the buffer hdr is already on this
    // buffers list.
    bool _onthislist(__t2t2_buffer_hdr *h) {
        return (h->list == buffers.head());
    }

    __T2T2_EVIL_CONSTRUCTORS(__t2t2_queue);
    __T2T2_EVIL_NEW(__t2t2_queue);
};

//////////////////////////// __T2T2_QUEUE_SET ////////////////////////////

class __t2t2_queue_set
{
    pthread_mutex_t   set_mutex;
    pthread_cond_t    set_cond;
    clockid_t         clk_id;
    __t2t2_links_head<__t2t2_queue> qs;
    int set_size;
    __t2t2_buffer_hdr * check_qs(int *id);
public:
    __t2t2_queue_set(void);
    ~__t2t2_queue_set(void);
    bool _add_queue(__t2t2_queue *q, int id);
    void _remove_queue(__t2t2_queue *q);
    int get_set_size(void) const { return set_size; }
    __t2t2_buffer_hdr * _dequeue(int wait_ms, int *id);
};

//////////////////////////// __T2T2_POOL ////////////////////////////

struct __t2t2_memory_block; // forward

/** base class for all t2t2_pool template objects. */
class __t2t2_pool
{
protected:
    t2t2_pool_stats  stats;
    int bufs_to_add_when_growing;
    std::list<std::unique_ptr<__t2t2_memory_block>> memory_pool;
    __t2t2_queue q;
    __t2t2_pool(int buffer_size,
               int _num_bufs_init,
               int _bufs_to_add_when_growing);
    virtual ~__t2t2_pool(void);
public:
    int get_buffer_size(void) const { return stats.buffer_size; }
    /** add more buffers to this pool.
     * \param num_bufs  the number of buffers to add to the pool. */
    void add_bufs(int num_bufs);
    // wait (see enum wait_flag):
    // -2 = T2T2_GROW         : grow if empty
    // -1 = T2T2_WAIT_FOREVER : wait forever,
    //  0 = T2T2_NO_WAIT      : dont wait
    // >0                     : wait for some mS
    void * _alloc(int wait_ms);
    void release(void * ptr);
    /** retrieve statistics about this pool */
    void get_stats(t2t2_pool_stats &_stats) const;

    __T2T2_EVIL_CONSTRUCTORS(__t2t2_pool);
    __T2T2_EVIL_DEFAULT_CONSTRUCTOR(__t2t2_pool);
};

//////////////////////////////////////////////////////////////////

#elif __T2T2_INCLUDE_INTERNAL__ == 2

//////////////////////////// T2T2_POOL<> ////////////////////////////

template <class BaseT, class... derivedTs>
template <class T, typename... ConstructorArgs>
bool t2t2_pool<BaseT,derivedTs...> :: alloc(
    pxfe_shared_ptr<T> * ptr, int wait_ms,
    ConstructorArgs&&... args)
{
    static_assert(std::is_base_of<t2t2_message_base<BaseT>,
                  BaseT>::value == true,
                  "allocated type must be derived from t2t2_message_base");
    static_assert(std::is_base_of<BaseT, T>::value == true,
                  "allocated type must be derived from base type");
    static_assert(buffer_size >= sizeof(T),
                  "allocated type must fit in pool buffer size, please "
                  "specify all message types in t2t2_pool<>!");

    T * t = new(this,wait_ms)
        T(std::forward<ConstructorArgs>(args)...);
    ptr->reset(t);
    return (t != NULL);
}

//////////////////////////// T2T2_QUEUE<> ////////////////////////////

template <class BaseT>
t2t2_queue<BaseT> :: t2t2_queue(void)
    : q()
{
}

template <class BaseT>
template <class T>
bool t2t2_queue<BaseT> :: enqueue(pxfe_shared_ptr<T> &_msg)
{
    bool ret = false;
    static_assert(std::is_base_of<BaseT, T>::value == true,
                  "enqueued type must be derived from "
                  "base type of the queue");
    BaseT * msg = _msg._take();
    if (msg)
    {
        __t2t2_buffer_hdr * h = (__t2t2_buffer_hdr *) msg;
        h--;
        h->ok();
        ret = q._enqueue_tail(h);
    }
    else
    {
        __T2T2_ASSERT(ENQUEUE_EMPTY_POINTER,false);
    }
    return ret;
}


template <class BaseT>
bool t2t2_queue<BaseT> :: empty(void)
{
    return q._empty();
}

template <class BaseT>
pxfe_shared_ptr<BaseT>   t2t2_queue<BaseT> :: dequeue(int wait_ms)
{
    pxfe_shared_ptr<BaseT>  ret;
    __t2t2_buffer_hdr * h = q._dequeue(wait_ms);
    if (h)
    {
        h++;
        ret._give((BaseT*) h);
    }
    return ret;
}

//////////////////////////// T2T2_QUEUE_SET<> ////////////////////////////

template <class BaseT>
t2t2_queue_set<BaseT> :: t2t2_queue_set(void)
    : qs()
{
}

template <class BaseT>
t2t2_queue_set<BaseT> :: ~t2t2_queue_set(void)
{
}

template <class BaseT>
bool t2t2_queue_set<BaseT> :: add_queue(t2t2_queue<BaseT> *q, int id)
{
    // __t2t2_queue_set does its own locking.
    return qs._add_queue(&q->q, id);
}

template <class BaseT>
void t2t2_queue_set<BaseT> :: remove_queue(t2t2_queue<BaseT> *q)
{
    // __t2t2_queue_set does its own locking.
    qs._remove_queue(&q->q);
}

template <class BaseT>
pxfe_shared_ptr<BaseT> t2t2_queue_set<BaseT> :: dequeue(int wait_ms,
                                                      int *id /*= NULL*/)
{
    pxfe_shared_ptr<BaseT>  ret;
    // __t2t2_queue_set does its own locking.
    __t2t2_buffer_hdr * h = qs._dequeue(wait_ms,id);
    if (h)
    {
        h++;
        ret._give((BaseT*) h);
    }
    return ret;
}

//////////////////////////// T2T2_MESSAGE_BASE<> ////////////////////////////

// NOTE
//        before you ask, I am aware of the "placement new" operator
//        syntax for tricking C++ into constructing a void*ptr into
//        a type, and also the trick of calling obj->TYPE::~TYPE() to
//        destruct it.
// e.g.
//          T * obj = (T*) allocate_memory(sizeof(T));
//          new (obj) T(std::forward<ConstructorArgs>(args)...);
// and
//          obj->T::~T();
// BUT
//        I am not using that trick, because the TYPE::~TYPE()
//        technique does not invoke virtual destructors!  That's kind
//        of important!  This does that properly.

template <class BaseT>
//static class method
void * t2t2_message_base<BaseT> :: operator new(
    size_t wanted_sz,
    __t2t2_pool *pool,
    int wait_ms) throw ()
{
    if (wanted_sz > pool->get_buffer_size())
    {
        __T2T2_ASSERT(BUFFER_SIZE_TOO_BIG_FOR_POOL, false);
        return NULL;
    }
    void * ret = pool->_alloc(wait_ms);
    if (ret != NULL)
    {
        // there's a GCC bug!! for some reason, gcc 10.2.1
        // absolutely REFUSES to write to obj->__pool here
        // unless i make this volatile!
        // (big clue: it works at gcc -O0 but fails at -O3!!
        // another big clue: adding a printf of obj->__pool
        // after the assignment also makes it work!)
        // i believe having operator new actually convert
        // the memory pointer to the desired type and writing
        // to it (before the constructor runs!) is an unusual
        // situation in the gcc world.
        volatile BaseT * obj = (volatile BaseT*) ret;
        obj->__pool = pool;
    }
    return ret;
}

template <class BaseT>
//static class method
void t2t2_message_base<BaseT> :: operator delete(void *ptr)
{
    BaseT * obj = (BaseT*) ptr;
    obj->__pool->release(ptr);
}

//////////////////////////////////////////////////////////////////

#endif /* __T2T2_INCLUDE_INTERNAL__ */
