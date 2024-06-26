
#include "thread2thread2.h"

namespace Thread2Thread2 {

//////////////////////////// ERROR HANDLING ////////////////////////////

const char * t2t2_error_types[] = {

    // please keep this in sync with t2t2_error_t

    "NO_ERROR",

    // the following errors are the result of user error.
    "BUFFER_SIZE_TOO_BIG_FOR_POOL",
    "DOUBLE_FREE",
    "QUEUE_IN_A_SET",
    "QUEUE_SET_EMPTY",
    "ENQUEUE_EMPTY_POINTER",

    // the following errors are most likely internal bugs.
    "LINKS_MAGIC_CORRUPT",
    "LINKS_ADD_ALREADY_ON_LIST",
    "LINKS_REMOVE_NOT_ON_LIST",
    "POOL_RELEASE_ALREADY_ON_LIST",
    "QUEUE_DEQUEUE_NOT_ON_THIS_LIST",
    "QUEUE_ENQUEUE_ALREADY_ON_A_LIST",
};

static_assert((sizeof(t2t2_error_types) / sizeof(char*))
              == (int)t2t2_error_t::NUM_ERRORS,
              "t2t2_error_types is out of sync with enum t2t2_error_t");

static void default_t2t2_assert_handler(t2t2_error_t e,
                                       bool fatal,
                                       const char *filename,
                                       int lineno)
{
    fprintf(stderr,
            "\n\nERROR: Thread2Thread2 ASSERTION %d (%s) at %s:%d\n\n",
            (int)e, t2t2_error_types[(int)e], filename, lineno);
    if (fatal)
        // if you dont like this exiting, CHANGE IT
        exit(1);
}

t2t2_assert_handler_t t2t2_assert_handler = &default_t2t2_assert_handler;

//////////////////////////// T2T2_POOL_STATS ////////////////////////////

t2t2_pool_stats :: t2t2_pool_stats(int _buffer_size /*= 0*/)
{
    init(_buffer_size);
}

void t2t2_pool_stats :: init(int _buffer_size)
{
    buffer_size = _buffer_size;
    total_buffers = 0;
    buffers_in_use = 0;
    alloc_fails = 0;
    grows = 0;
    double_frees = 0;
}

//////////////////////////// __T2T2_MEMORY_BLOCK ////////////////////////////

struct __t2t2_memory_block
{
    // this is required by unique_ptr, because it has
    // a static_assert(sizeof(_Tp)>0) in it.....
    int dummy;
    uint64_t data[0]; // forces entire struct to 8 byte alignment
    __t2t2_memory_block(void)
    {
        dummy = 0;
    }
    void *operator new(size_t struct_sz, int real_size)
    {
        return malloc(struct_sz + real_size);
    }
    void  operator delete(void *ptr)
    {
        free(ptr);
    }

    __T2T2_EVIL_CONSTRUCTORS(__t2t2_memory_block);
    __T2T2_EVIL_NEW(__t2t2_memory_block);
};

//////////////////////////// __T2T2_POOL ////////////////////////////

__t2t2_pool :: __t2t2_pool(int buffer_size,
                         int _num_bufs_init,
                         int _bufs_to_add_when_growing)
    : stats(buffer_size), q()
{
    bufs_to_add_when_growing = _bufs_to_add_when_growing;
    add_bufs(_num_bufs_init);
}

//virtual
__t2t2_pool :: ~__t2t2_pool(void)
{
}

void __t2t2_pool :: add_bufs(int num_bufs)
{
    if (num_bufs <= 0)
        return;
    int real_buffer_size = stats.buffer_size + sizeof(__t2t2_buffer_hdr);
    int memory_block_size = num_bufs * real_buffer_size;
    __t2t2_memory_block * c = new(memory_block_size) __t2t2_memory_block;
    memory_pool.push_back(std::unique_ptr<__t2t2_memory_block>(c));
    uint8_t * ptr = (uint8_t *) c->data;
    for (int ind = 0; ind < num_bufs; ind++)
    {
        __t2t2_buffer_hdr * h = (__t2t2_buffer_hdr *) ptr;
        h->init();
        stats.total_buffers ++;
        q._enqueue(h);
        ptr += real_buffer_size;
    }
}

// wait_ms (see enum wait_flag):
// -2 = T2T2_GROW         : grow if empty (unique to alloc)
// -1 = T2T2_WAIT_FOREVER : wait forever,
//  0 = T2T2_NO_WAIT      : dont wait
// >0                     : wait for some mS
void * __t2t2_pool :: _alloc(int wait_ms)
{
    __t2t2_buffer_hdr * h = NULL;
    if (wait_ms == GROW)
    {
        if (q._empty())
        {
            add_bufs(bufs_to_add_when_growing);
            stats.grows ++;
        }
        h = q._dequeue(0);
    }
    else
    {
        h = q._dequeue(wait_ms);
    }
    if (h == NULL)
    {
        stats.alloc_fails ++;
        return NULL;
    }
    stats.buffers_in_use++;
    h++;
    return h;
}

void __t2t2_pool :: release(void * ptr)
{
    __t2t2_buffer_hdr * h = (__t2t2_buffer_hdr *) ptr;
    h--;
    if (h->list != NULL)
    {
        if (q._onthislist(h))
        {
            __T2T2_ASSERT(DOUBLE_FREE,false);
            stats.double_frees ++;
        }
        else
        {
            __T2T2_ASSERT(POOL_RELEASE_ALREADY_ON_LIST,true);
        }
    }
    // ignoring return value because we've already
    // checked the h->list condition above.
    q._enqueue(h);
    stats.buffers_in_use --;
}

void __t2t2_pool :: get_stats(t2t2_pool_stats &_stats) const
{
    _stats = stats;
}

//////////////////////////// __T2T2_QUEUE ////////////////////////////

__t2t2_queue :: __t2t2_queue(void)
{
    pthread_mutexattr_t mattr;
    pthread_condattr_t  cattr;

    __t2t2_links::init();

    clk_id = CLOCK_REALTIME;

    pthread_mutexattr_init(&mattr);
    pthread_condattr_init(&cattr);
    pthread_condattr_setclock(&cattr, clk_id);

    pthread_mutex_init(&mutex, &mattr);
    pthread_cond_init(&cond, &cattr);

    pthread_mutexattr_destroy(&mattr);
    pthread_condattr_destroy(&cattr);

    psetmutex = NULL;
    psetcond = NULL;
    id = 0;
}

__t2t2_queue :: ~__t2t2_queue(void)
{
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
}

bool __t2t2_queue :: _empty(void)
{
    Lock  l(&mutex);
    return buffers.empty();
}

// -1 : wait forever
//  0 : dont wait, just return
// >0 : wait for some number of mS
__t2t2_buffer_hdr * __t2t2_queue :: _dequeue(int wait_ms)
{
    __t2t2_buffer_hdr * h = NULL;
    Lock  l(&mutex);
    if (psetmutex != NULL)
    {
        __T2T2_ASSERT(QUEUE_IN_A_SET,false);
        return NULL;
    }

    bool first = true;
    __t2t2_timespec  ts;
    // if wait_ms == 0, just short circuit the
    // while loop and check buffers.empty only once.
    bool timed_out = (wait_ms == 0);
    bool empty = buffers.empty();

    while (empty && !timed_out)
    {
        if (wait_ms < 0)
        {
            // we never set timed_out.
            pthread_cond_wait(&cond, &mutex);
        }
        else // wait_ms > 0 (note ==0 was already checked)
        {
            if (first)
            {
                __t2t2_timespec t(wait_ms);
                ts.getNow(clk_id);
                ts += t;
                first = false;
            }
            int ret = pthread_cond_timedwait(&cond, &mutex, &ts);
            if (ret == ETIMEDOUT)
                timed_out = true;
        }
        empty = buffers.empty();
    }
    if (!empty)
    {
        h = buffers.get_head();
        h->ok();
        if (!_validate(h))
            __T2T2_ASSERT(QUEUE_DEQUEUE_NOT_ON_THIS_LIST,true);
        h->remove();
    }
    return h;
}

bool __t2t2_queue :: _enqueue(__t2t2_buffer_hdr *h)
{
    h->ok();
    if (h->list != NULL)
    {
        __T2T2_ASSERT(QUEUE_ENQUEUE_ALREADY_ON_A_LIST,false);
        return false;
    }
    {
        Lock l(&mutex);
        buffers.add_next(h);
        if (psetmutex)
        {
            Lock l2(psetmutex);
            pthread_cond_signal(psetcond);
        }
    }
    pthread_cond_signal(&cond);
    return true;
}

bool __t2t2_queue :: _enqueue_tail(__t2t2_buffer_hdr *h)
{
    h->ok();
    if (h->list != NULL)
    {
        __T2T2_ASSERT(QUEUE_ENQUEUE_ALREADY_ON_A_LIST,false);
        return false;
    }
    {
        Lock l(&mutex);
        buffers.add_prev(h);
        if (psetmutex)
        {
            Lock l2(psetmutex);
            pthread_cond_signal(psetcond);
        }
    }
    pthread_cond_signal(&cond);
    return true;
}

//////////////////////////// __T2T2_QUEUE_SET ////////////////////////////

__t2t2_queue_set ::__t2t2_queue_set(void)
{
    pthread_mutexattr_t mattr;
    pthread_condattr_t  cattr;

    clk_id = CLOCK_REALTIME;

    pthread_mutexattr_init(&mattr);
    pthread_condattr_init(&cattr);
    pthread_condattr_setclock(&cattr, clk_id);

    pthread_mutex_init(&set_mutex, &mattr);
    pthread_cond_init(&set_cond, &cattr);

    pthread_mutexattr_destroy(&mattr);
    pthread_condattr_destroy(&cattr);

    set_size = 0;
}

__t2t2_queue_set :: ~__t2t2_queue_set(void)
{
    __t2t2_queue * q;
    while ((q = qs.get_next()) != qs.head())
        _remove_queue(q);
    pthread_mutex_destroy(&set_mutex);
    pthread_cond_destroy(&set_cond);
}

bool
__t2t2_queue_set :: _add_queue(__t2t2_queue *q, int id)
{
    __t2t2_queue::Lock l(&set_mutex);

    if (q->list != NULL)
    {
        __T2T2_ASSERT(QUEUE_IN_A_SET, false);
        return false;
    }

    // find the correct position in the queues list to
    // add this queue, based on treating "id" as a sorted
    // priority. note, if the qs is empty, tq will end up
    // pointing to the head iself, which is ok, because
    // the add_prev below still does the right thing, adding
    // the first item to the empty list.
    __t2t2_queue * tq;
    for (tq = qs.get_head(); tq != qs.head(); tq = tq->get_next())
        if (tq->id > id)
            break;
    q->set_pmutexpcond(&set_mutex, &set_cond);
    q->id = id;
    tq->add_prev(q);
    set_size ++;
    return true;
}

void
__t2t2_queue_set :: _remove_queue(__t2t2_queue *q)
{
    __t2t2_queue::Lock l(&set_mutex);
    q->remove();
    q->set_pmutexpcond();
    set_size --;
}

// this function assumes set_mutex is locked.
__t2t2_buffer_hdr *
__t2t2_queue_set :: check_qs(int *id)
{
    __t2t2_buffer_hdr * h = NULL;
    for (__t2t2_queue * q = qs.get_head();
         q != qs.head();
         q = q->get_next())
    {
        __t2t2_queue::Lock l(&q->mutex);
        if (q->buffers.empty() == false)
        {
            h = q->buffers.get_head();
            if (!q->_validate(h))
                __T2T2_ASSERT(QUEUE_DEQUEUE_NOT_ON_THIS_LIST,true);
            h->remove();
            if (id)
                *id = q->id;
            break;
        }
    }
    return h;
}

__t2t2_buffer_hdr *
__t2t2_queue_set :: _dequeue(int wait_ms, int *id)
{
    __t2t2_buffer_hdr * h = NULL;
    bool first = true;
    __t2t2_timespec  ts;
    // if wait_ms == 0, just short circuit the
    // while loop and check_qs() only once.
    bool timed_out = (wait_ms == 0);

    if (qs.empty())
    {
        __T2T2_ASSERT(QUEUE_SET_EMPTY,false);
        if (id)
            *id = -1;
        return NULL;
    }

    __t2t2_queue::Lock  l(&set_mutex);

    h = check_qs(id);
    while (!h && !timed_out)
    {
        if (wait_ms < 0)
        {
            // never set timed_out.
            pthread_cond_wait(&set_cond, &set_mutex);
        }
        else // wait_ms > 0 (note ==0 was already checked)
        {
            if (first)
            {
                __t2t2_timespec t(wait_ms);
                ts.getNow(clk_id);
                ts += t;
                first = false;
            }
            int ret = pthread_cond_timedwait(&set_cond, &set_mutex, &ts);
            if (ret == ETIMEDOUT)
                timed_out = true;
        }
        // always check again after cond wait returns,
        // if there's a race on expiry vs enqueue,
        // always win on the side of the enqueue.
        h = check_qs(id);
    }
    return h;
}

}; // namespace Thread2Thread2

///////////////////////// STREAM OPS /////////////////////////

// this has to be outside the namespace
std::ostream &
operator<<(std::ostream &strm,
           const Thread2Thread2::t2t2_pool_stats &stats)
{
    strm << "bufsz " << stats.buffer_size
         << " total " << stats.total_buffers
         << " inuse " << stats.buffers_in_use
         << " allocfails " << stats.alloc_fails
         << " grows " << stats.grows
         << " doublefrees " << stats.double_frees;
    return strm;
}
