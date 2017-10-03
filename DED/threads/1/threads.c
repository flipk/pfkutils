
/*
 * non-pre-emptive threads.
 */

#include "th_int.h"

/*
 * TODO: 
 *   - figure out why th_sw cannot be inline with -O2 -Wall:
 *        In function `th_select':
 *        threads.c:975: warning: argument `nrfds' might be
 *                                clobbered by `longjmp' or `vfork'
 *   - implement tsleep/wakeup
 */

static const char *pidstates[] = {
    "Nonexistant", "Ready", "Suspended",
    "Current", "Sleep", "TSleep", "IOWait"
};

#ifndef FD_COPY
#define FD_COPY(from, to) bcopy(from, to, sizeof(*to))
#endif

/*
 * this array is indexed by file-descriptor; the value of 
 * the element is the thread that owns that file descriptor.
 * defn. of owns:  the thread which is asleep waiting for
 * activity on this fd.
 */

static thread_t *descriptors[MAX_FDS];

int     thread_ticks;
int     numthreads;
static int     nfds;
static fd_set  readfds, writefds;

/*
 * one bit is set in this mask for each priority 
 * which has a thread at that priority ready to run.
 * the algorithm that determines which thread is next to
 * run works by finding the highest-numbered bit in this
 * mask that is set, and dequeuing from the readyq linked list
 * that is indexed by that number. 
 */

static unsigned int priomask[NUM_PRIOS / 32];

/*
 * all threads that exist have a pointer in this array, indexed
 * by the thread-id.
 */

static thread_t *threads[MAX_THREADS];

/*
 * these pointers are head/tail pointers to a linked list, indexed
 * by the thread priority. the above mask has one bit for each of
 * these lists. threads for priority X are removed from readyq[X]
 * and added to readyq_end[X].
 */

static thread_t *readyq[NUM_PRIOS];
static thread_t *readyq_end[NUM_PRIOS];

/*
 * all threads which are sleeping on something are on this list;
 * the tick count within each structure is the time that event
 * must wake up after the one in front of it in the list does.
 */
static thread_t *sleepq;

/*
 * this always points to the currently-running thread,
 * or NULL before or after threads are running.
 */
static thread_t *current;

/*
 * when all threads have died, longjmp here to exit the engine.
 */
static jmp_buf exit_thread;
static void   *free_later;

/*
 * inlineamania.
 */

__inline static void        enqueue           (thread_t *);
__inline static thread_t *  dequeue           (void);
__inline static void        thremove          (thread_t *);
__inline static int         find_max_fd       (void);
__inline static void        expire_timer      (void);
__inline static void        timer_tick        (void);
__inline static void        wakeup_io_threads (int cc, fd_set *, fd_set *);
__inline static void        _th_sleep         (int ticks);
__inline static void        _th_unsleep       (thread_t *);
/**/     static void        th_sw             (enum threadstate);
/**/     static void        idler             (void *);
/**/     static void        th_start          (void);

/*
 * enqueue the task supplied onto the ready queue
 * which is appropriate for its priority.
 */

__inline
static void
enqueue( thread_t *t )
{
    int ind, bit, prio;

    t->state = TH_READY;
    t->next = NULL;
    prio = t->prio;

    ind = prio / 32;
    bit = prio % 32;

    priomask[ind] |= (1 << bit);

    if (readyq[prio] == NULL)
    {
        readyq[prio] = t;
        readyq_end[prio] = t;
        return;
    }

    readyq_end[prio]->next = t;
    readyq_end[prio] = t;
}

/*
 * pull the highest prio thread off the list and return it.
 */
 
__inline
static thread_t *
dequeue(void)
{
    thread_t *ret;
    int ind, bit, prio;
    unsigned int cur;

    for (ind = (NUM_PRIOS / 32) - 1; ind >= 0; ind--)
        if (priomask[ind] != 0)
            break;

    if (ind == -1)
        return NULL;

    for (cur = priomask[ind], bit = 31;
         bit >= 0;
         cur <<= 1, bit--)
    {
        if (cur & 0x80000000)
            break;
    }

    prio = 32 * ind + bit;

    ret = readyq[prio];
    if (ret == NULL)
        return NULL;

    readyq[prio] = ret->next;
    if (ret->next == NULL)
    {
        readyq_end[prio] = NULL;
        priomask[ind] &= ~(1 << bit);
    }

    ret->next = NULL;
    return ret;
}

/*
 * remove the specified thread from the ready list.
 * if its not on the ready list, silently fail.
 */

__inline
static void
thremove( thread_t *t )
{
    thread_t *cur, *old;
    int ind, bit, prio;

    prio = t->prio;
    ind = prio / 32;
    bit = prio % 32;

    if (!(priomask[ind] & (1 << bit)))
        return;

    if (readyq[prio] == t)
    {
        readyq[prio] = t->next;
        if (t->next == NULL)
        {
            readyq_end[prio] = NULL;
            priomask[ind] &= ~(1 << bit);
        }
        return;
    }

    for (old = NULL, cur = readyq[prio];
         cur != NULL && cur != t;
         old = cur, cur = cur->next);

    if (cur == NULL)
        return;

    old->next = cur->next;
    if (readyq_end[prio] == cur)
        readyq_end[prio] = old;

    return;
}

/* 
 * examine the two global fd_sets to find the number
 * of the largest fd in the sets.  actually returns
 * that rounded up to the next multiple of 32.
 */

__inline
static int
find_max_fd(void)
{
    int i, hm, cur;

    hm = howmany(FD_SETSIZE, NFDBITS);

    for (i=hm-1; i >= 0; i--)
    {
        cur = readfds.fds_bits[i] | writefds.fds_bits[i];

        if (cur != 0)
            break;
    }

    i++;

    return i * 32;
}

#define INSERT                                                          \
        t = descriptors[fd + bit];                                      \
        if (t != NULL && t->state != TH_READY)                          \
        {                                                               \
                enqueue(t);                                             \
                                                                        \
                if (t->selectfds && t->numselectfds < t->maxselectfds)  \
                {                                                       \
                        int ind = t->numselectfds++;                    \
                        t->selectfds[ind] = fd + bit;                   \
                        if (w->fds_bits[i] & mask)                      \
                                t->selectfds[ind] |= SELECT_FOR_WRITE;  \
                }                                                       \
        }

/*
 * examine the supplied fd_sets; if any bits are set,
 * find the thread owning that fd, and move it to the
 * readyq.  if it was on the sleepq, cancel its sleeping.
 */

__inline
static void
wakeup_io_threads ( int cc, fd_set *r, *w )
{
    int i, bit, mask, fd, cur, hm;
    thread_t *t;

    hm = howmany(FD_SETSIZE, NFDBITS);
    /*
     * walk the fd_sets provided, looking for
     * file descriptors that are awake.
     * look up tids in the descriptors[] array
     * and resume the owners of those descriptors.
     */

    for (i=0, fd = 0; cc > 0; i++, cc--)
    {
        /*
         * race ahead to find the next long which has a bit set.
         */
        for (;;)
        {
            cur = r->fds_bits[i] | w->fds_bits[i];

            if (cur != 0)
                break;

            if (++i == hm)
                return;

            fd += 32;
        }

        /*
         * go a bit at a time thru this long
         * finding each 1.
         */
        for (mask = 1, bit = 0;
             bit < 32;
             mask <<= 1, bit++)
        {
            if (cur & mask)
            {
                INSERT;
            }
        }
    }
}

/* 
 * pull off all threads which are due,
 * and make them runnable.
 */

__inline
static void
expire_timer( void )
{
    thread_t *new;

    while (sleepq != NULL && sleepq->ticks <= 0)
    {
        new = sleepq;
        sleepq = new->next;
        new->ticks = 0;
        enqueue(new);
    }
}

/*
 * note about timers:
 * there is a "ticks" value in struct _thread; 
 * however, "ticks" represents the number of ticks
 * this one should wake up after the one in front of it
 * in the sleepq list does.
 *
 * are you confused by the above sentence?
 */

__inline
static void
timer_tick( void )
{
    if (sleepq != NULL)
    {
        sleepq->ticks--;
        if (sleepq->ticks <= 0)
            expire_timer();
    }
}

/*
 * check all file descriptors.
 * wake up threads that need it.
 * return 0 if nothing woke up, return 1 if something did.
 */

__inline
static int
check_fds( int timeout )
{
    struct timeval to;
    fd_set r, w;
    int cc;

    to.tv_sec = 0;
    to.tv_usec = timeout;

    FD_COPY(&readfds, &r);
    FD_COPY(&writefds, &w);

    cc = select(nfds, &r, &w, NULL, &to);
    if (cc > 0)
    {
        wakeup_io_threads(cc, &r, &w);
        return 1;
    }

    return 0;
}

/*
 * switch threads; if the next thread to run is lower than 
 * us, short-circuit the switch and just return.
 */

static void
th_sw( enum threadstate newstate )
{
    thread_t *old;

    if (newstate == TH_READY)
    {
        enqueue(current);
    }

    current->state = newstate;

    /*
     * remember where we said we would free this later? 
     * its later.
     */

    if (free_later != NULL)
    {
        free( free_later );
        free_later = NULL;
    }

    /*
     * poll all file descriptors.
     * if something high enough priority woke up,
     * we'll switch to it now.
     */
    (void)check_fds(0);

    old = current;
    current = dequeue();
    current->state = TH_CURR;

    /*
     * if we are still the highest priority, 
     * optimize out the context switch.
     */

    if (old == current)
        return;

    /* 
     * store the context of the thread switched from,
     * and restore the context of the thread switched to.
     */

    if (!(_setjmp(old->jb)))
        _longjmp(current->jb, 1);
}

static void
idler( void *dummy )
{
    while (numthreads > 1)
    {
        if (check_fds(33334) == 0)
            timer_tick();

        thread_ticks++;
        th_yield();
    }
}

void
th_init( void )
{
    bzero(descriptors, sizeof(descriptors));
    bzero(threads, sizeof(threads));
    numthreads = 0;
    free_later = NULL;
    sleepq = current = NULL;
    thread_ticks = 0;

    bzero(readyq,     sizeof(readyq));
    bzero(readyq_end, sizeof(readyq_end));
    bzero(priomask,   sizeof(priomask));

    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    nfds = 0;

    /*
     * create the lowest-priority thread.
     * if you want an idler of your own instead,
     * use priority 1.
     */

    th_create(idler, NULL, 0, "idler");
}

void
th_loop(void)
{
    if (!_setjmp(exit_thread))
    {
        current = dequeue();

        if (current == NULL)
            return;

        current->state = TH_CURR;
        _longjmp(current->jb, 1);
        /* NOTREACHED */
    }

    if (free_later != NULL)
    {
        free( free_later );
        free_later = NULL;
    }
}

int
th_create( void (*func)(void *), void *arg, int prio, char *name )
{
    thread_t *new;
    int i;

    new = (thread_t *)malloc( sizeof( thread_t ));
    if (new == NULL)
    {
        fprintf(stderr, "th_create: out of threads\n");
        return -1;
    }

    new->stack = (void*)malloc( STACK_SIZE );
    if (new->stack == NULL)
    {
        free( new );
        fprintf(stderr, "th_create: out of stacks for thread\n");
        return -1;
    }

    new->prio = prio;
    new->func = func;
    new->arg = arg;
    new->name = strdup(name);
    new->ticks = 0;
    new->numselectfds = 0;
    new->selectfds = NULL;

    for (i=0; i < MAX_THREADS; i++)
    {
        if (threads[i] == NULL)
            break;
    }

    if (i == MAX_THREADS)
    {
        fprintf(stderr, "th_create: out of thread slots\n");
        free( new->stack );
        free( new );
        return -1;
    }

    threads[i] = new;
    new->tid = i;

    /*
     * machine-dependent portion.  poke the new stack
     * and program counter into the context buffer.
     * on x86, 0 just so happens to be %eip, and 
     * 2 just so happens to be %esp.
     *
     * allow 8 bytes of stack slop to allow for space
     * for previous frame pointers. debuggers and other
     * things seem to enjoy that.
     */

    _setjmp(new->jb);

    /* jmpbuf code is highly OS and machine-dependent. */

#if defined(__FreeBSD__)
    /*
     * on x86 freebsd, slot 0 is %pc, and slot 2 is %esp.
     */
    new->jb->_jb[0] = (int)th_start;
    new->jb->_jb[2] = (int)new->stack + STACK_SIZE - 8;
#elif defined(sun)
    /*
     * on sparc/sunos, 
     * pc is stored in slot 3, and sp is in slot 2.
     */
    new->jb[3] = (int)th_start;
    new->jb[2] = (int)new->stack + STACK_SIZE - 8;
#else
#error Need stack/pc setup for this OS.
#endif

    numthreads++;

    enqueue(new);

    /*
     * if threading is currently running, and the new
     * thread is higher priority than the currently running creator,
     * the new thread must run immediately.
     */

    if (current != NULL && current->prio < new->prio)
        th_sw(TH_READY);

    return new->tid;
}

static void
th_start(void)
{
    (current->func)(current->arg);

    th_kill(0);
    /*NOTREACHED*/
}

void
th_kill( int tid )
{
    thread_t *target;

    if (tid == 0 || current->tid == tid)
    {
        target = current;
        tid = target->tid;

        /*
         * a queued freeing system; can't delete stack we're
         * running on, so we'll free the stack later,
         * when we're not using it.
         */
        if (free_later != NULL)
        {
            free( free_later );
        }

        free_later = target->stack;
    }
    else
    {
        target = threads[tid];
        if (target == NULL)
        {
            fprintf(stderr,
                    "th_kill: tid %d doesn't exist\n", tid);
            return;
        }
        free( target->stack );
    }

    free(target->name);
    threads[tid] = NULL;
    free( target );
    numthreads--;

    if (target != current)
        return;

    current = dequeue();

    /*
     * if all threads are dead (including the idler!)
     * this means exit.
     */
    if (current == NULL)
        _longjmp(exit_thread, 1);

    current->state = TH_CURR;
    _longjmp(current->jb, 1);
}

void
th_yield(void)
{
    th_sw(TH_READY);
}

int
th_tid(void)
{
    return current->tid;
}

void
th_suspend( int tid )
{
    thread_t *target;

    if (tid == 0)
    {
        /* easy way to suspend ourselves */
        th_sw(TH_SUSPENDED);
        return;
    }
    else
    {
        target = threads[tid];
    }

    if (target == NULL)
    {
        fprintf(stderr, "th_suspend: tid %d doesn't exist.\n", tid);
        return;
    }

    /*
     * only a thread which is ready 
     * can be suspended.
     */

    if (target->state != TH_READY)
    {
        fprintf(stderr, 
                "th_suspend: tid %d is already in state '%s'\n",
                tid, pidstates[threads[tid]->state]);
        return;
    }

    thremove(target);
    target->state = TH_SUSPENDED;
}

void
th_resume( int tid )
{
    thread_t *target;

    if (tid == 0)
    {
        fprintf(stderr, 
                "th_resume: cannot resume self -- already running\n");
        return;
    }

    target = threads[tid];

    if (target == NULL)
    {
        fprintf(stderr, "th_resume: tid %d does not exist\n", tid);
        return;
    }

    /*
     * this is a really handy way to cut sleeps and iowaits short.
     */

    if ((target->state != TH_SUSPENDED) &&
        (target->state != TH_IOWAIT) &&
        (target->state != TH_SLEEP))
    {
        fprintf(stderr, "th_resume: tid %d is not suspended\n", tid);
        return;
    }

    enqueue(target);

    /*
     * if we just resumed a task which is
     * higher prio than ourselves, it must run next.
     */
    if (target->prio > current->prio)
        th_sw(TH_READY);
}

__inline
static void
_th_sleep( int ticks )
{
    thread_t *t, *ot;

    /* 
     * t->ticks at each point in the list is the number of
     * ticks t should expire after the event in front of it.
     * thus, each time a tick happens, only the first tick in
     * the list must be decremented.
     */

    /*
     * if sleepq is empty, it's trivial.
     */
    if (sleepq == NULL)
    {
        sleepq = current;
        current->next = NULL;
    } else {
        /*
         * walk the list looking for an event
         * that should happen after us, decrement
         * our tick count as we go.
         */

        for (ot = NULL, t = sleepq;
             t != NULL && ticks >= t->ticks;
             ot = t, t = t->next)
        {
            ticks -= t->ticks;
        }

        /*
         * we found some spot.
         */

        if (t != NULL)
        {
            /*
             * adjust the tick count of the event after us.
             */

            t->ticks -= ticks;

            /*
             * and insert ourselves into the list.
             */

            if (ot == NULL)
            {
                                /*
                                 * we are at the head of the list.
                                 */

                sleepq = current;
            } else {
                                /*
                                 * we are in the middle somewhere.
                                 */

                ot->next = current;
            }

            current->next = t;
        } else {
            /*
             * our spot is at the end of the list.
             */

            ot->next = current;
            current->next = NULL;
        }
    }

    current->ticks = ticks;
}

__inline
static void
_th_unsleep( thread_t *ut )
{
    thread_t *t, *ot;

    for (ot = NULL, t = sleepq; 
         t != NULL;
         ot = t, t = t->next)
    {
        if (t == ut)
            break;
    }

    if (t == NULL)
        return;

    if (ot == NULL)
    {
        sleepq = t->next;
        if (sleepq != NULL)
        {
            sleepq->ticks += t->ticks;
        }
    } else {
        ot->next = t->next;
        if (ot->next != NULL)
        {
            ot->next->ticks += t->ticks;
        }
    }

    t->next = NULL;
    t->ticks = 0;
}

void
th_sleep( int ticks )
{
    if (ticks <= 0)
        return;

    _th_sleep(ticks);
    th_sw(TH_SLEEP);
    if (current->ticks > 0)
        _th_unsleep(current);
}

int
th_read( int fd, char *buf, int size )
{
    /*
     * put the thread to sleep, marking the specified descriptor
     * as owned by us.  when data arrives, the idler task will
     * wake us up again. then we can do the read.
         */

    if (fd < 0)
    {
        fprintf(stderr, "th_read: negative fd passed in\n");
        return -1;
    }

    if (fd > MAX_FDS)
    {
        fprintf(stderr, "th_read: fd = %d but I can only handle "
                "fds less than %d!\n", fd, MAX_FDS);
        return -1;
    }

    descriptors[fd] = current;
    FD_SET(fd, &readfds);
    nfds = find_max_fd();

    th_sw(TH_IOWAIT);

    FD_CLR(fd, &readfds);
    nfds = find_max_fd();
    descriptors[fd] = NULL;

    return read(fd, buf, size);
}

int
th_write( int fd, char *buf, int size )
{
    /*
     * put the thread to sleep, marking the specified descriptor
     * as owned by us.  when there is room for more data, the idler
     * task will wake us up again. then we can do the write.
         */

    if (fd < 0)
    {
        fprintf(stderr, "th_read: negative fd passed in\n");
        return -1;
    }

    if (fd > MAX_FDS)
    {
        fprintf(stderr, "th_read: fd = %d but I can only handle "
                "fds less than %d!\n", fd, MAX_FDS);
        return -1;
    }

    descriptors[fd] = current;
    FD_SET(fd, &writefds);
    nfds = find_max_fd();

    th_sw(TH_IOWAIT);

    FD_CLR(fd, &writefds);
    nfds = find_max_fd();
    descriptors[fd] = NULL;

    return write(fd, buf, size);
}

int
th_select( int nrfds, int *rfds,
           int nwfds, int *wfds,
           int nofds, int *ofds, int ticks)
{
    int i;
    int ret;

    /*
     * put the thread to sleep, marking all fds listed as 
     * owned by us.  also put ourselves on the sleep queue,
     * using the given tick counter.
     * when one of the fds becomes active, the idler will
     * wake us up.  if the timeout comes first, we can tell
     * by observing that current->ticks will be zero.
     * return the list of fds that were found to be
     * awake back to the caller.
     */

    if (ticks == 0)
    {
        fd_set r, w;
        struct timeval to;
        int cc, nfds, ret, hm, fd, bit;
        long mask, cur;

        /*
         * optimization: if the caller is just polling,
         * don't bother switching context to the idler.
         * just do it here and return the result.
         */

        hm = howmany(FD_SETSIZE, NFDBITS);

        FD_ZERO(&r);
        FD_ZERO(&w);

        nfds = 0;

        for (i=0; i < nrfds; i++)
        {
            FD_SET(rfds[i], &r);
            if (rfds[i] >= nfds)
                nfds = rfds[i] + 1;
        }

        for (i=0; i < nwfds; i++)
        {
            FD_SET(wfds[i], &w);
            if (wfds[i] >= nfds)
                nfds = wfds[i] + 1;
        }
        to.tv_sec = 0;
        to.tv_usec = 0;

        cc = select(nfds, &r, &w, NULL, &to);

        ret = 0;

        for (i=0, fd=0; cc > 0; i++, cc--)
        {
            for (;; fd += 32)
            {
                cur = r.fds_bits[i] | w.fds_bits[i];
                if (cur != 0)
                    break;
                if (++i == hm)
                    return ret;
            }
            for (mask = 1, bit = 0;
                 bit < 32;
                 bit++, mask <<= 1)
            {
                if (cur & mask)
                {
                    ofds[ret++] = fd + bit;
                }
            }
        }

        return ret;
    }

#define INSTALL(count, list)                                            \
        for (i=0; i < count; i++)                                       \
        {                                                               \
                if (list[i] > MAX_FDS)                                  \
                {                                                       \
                        while (i-- > 0)                                 \
                                descriptors[list[i]] = NULL;            \
                        fprintf(stderr,                                 \
                                "th_select: fd larger than %d was "     \
                               "passed in\n", MAX_FDS);                 \
                        i = 0;                                          \
                        break;                                          \
                }                                                       \
                descriptors[list[i]] = current;                         \
        }

    if (nrfds > 0)
    {
        INSTALL(nrfds, rfds);
        if (i == 0)
            return -1;
    }

    if (nwfds > 0)
    {
        INSTALL(nwfds, wfds);
        if (i == 0)
        {
            while (nrfds-- > 0)
                descriptors[rfds[i]] = NULL;
            return -1;
        }
    }

#undef INSTALL

    for (i = 0; i < nrfds; i++)
        FD_SET(rfds[i], &readfds);

    for (i = 0; i < nwfds; i++)
        FD_SET(wfds[i], &writefds);

    nfds = find_max_fd();

    current->selectfds = ofds;
    current->numselectfds = 0;
    current->maxselectfds = nofds;

    if (ticks > 0)
    {
        _th_sleep(ticks);
    } 

    th_sw(TH_IOWAIT);

    if (current->ticks > 0)
    {
        _th_unsleep(current);
    }

    current->selectfds = NULL;
    ret = current->numselectfds;
    current->numselectfds = 0;
    current->maxselectfds = 0;

    for (i = 0; i < nrfds; i++)
    {
        FD_CLR(rfds[i], &readfds);
        descriptors[rfds[i]] = NULL;
    }

    for (i = 0; i < nwfds; i++)
    {
        FD_CLR(wfds[i], &writefds);
        descriptors[wfds[i]] = NULL;
    }

    nfds = find_max_fd();

    return ret;
}
