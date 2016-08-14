/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

// TODO : strerror_r workarounds
// http://stackoverflow.com/questions/3051204/strerror-r-returns-trash-when-i-manually-set-errno-during-testing
// TODO : error check the hell out of stuff that isn't being error checked.
// TODO : strtok class
// TODO : revamp dll3 to use this interface instead of LockWait

#ifndef __pfkposix_h__
#define __pfkposix_h__

#include <pthread.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <inttypes.h>
#include <dirent.h>
#include <stdio.h>
#include <iostream>
#include <string>

struct pfk_timeval : public timeval
{
    pfk_timeval(void) { tv_sec = 0; tv_usec = 0; }
    pfk_timeval(time_t s, long u) { set(s,u); }
    pfk_timeval(const pfk_timeval &other) {
        tv_sec = other.tv_sec;  tv_usec = other.tv_usec;
    }
    void set(time_t s, long u) { tv_sec = s; tv_usec = u; }
    const pfk_timeval& operator=(const timeval &rhs) {
        tv_sec = rhs.tv_sec;
        tv_usec = rhs.tv_usec;
        return *this;
    }
    const pfk_timeval& operator=(const timespec &rhs) {
        tv_sec = rhs.tv_sec;
        tv_usec = rhs.tv_nsec / 1000;
        return *this;
    }
    const pfk_timeval& operator-=(const pfk_timeval &rhs) {
        bool borrow = false;
        if (rhs.tv_usec > tv_usec)
            borrow = true;
        tv_sec -= rhs.tv_sec;
        tv_usec -= rhs.tv_usec;
        if (borrow)
        {
            tv_sec -= 1;
            tv_usec += 1000000;
        }
        return *this;
    }
    const pfk_timeval& operator+=(const pfk_timeval &rhs) {
        tv_sec += rhs.tv_sec;
        tv_usec += rhs.tv_usec;
        if (tv_usec > 1000000)
        {
            tv_usec -= 1000000;
            tv_sec += 1;
        }
        return *this;
    }
    const bool operator==(const pfk_timeval &other) {
        if (tv_sec != other.tv_sec)
            return false;
        if (tv_usec != other.tv_usec)
            return false;
        return true;
    }
    const bool operator!=(const pfk_timeval &other) {
        return !operator==(other);
    }
    const pfk_timeval &getNow(void) {
        gettimeofday(this, NULL);
        return *this;
    }
    const std::string Format(const char *format = NULL) {
        if (format == NULL)
            format = "%Y-%m-%d %H:%M:%S";
        time_t seconds = tv_sec;
        struct tm t;
        localtime_r(&seconds, &t);
        char ymdhms[128], ms[12];
        strftime(ymdhms,sizeof(ymdhms),format,&t);
        ymdhms[sizeof(ymdhms)-1] = 0;
        snprintf(ms,sizeof(ms),"%06ld", tv_usec);
        ms[sizeof(ms)-1] = 0;
        return std::string(ymdhms) + "." + ms;
    }
    uint32_t msecs(void) {
        return (tv_sec * 1000) + (tv_usec / 1000);
    }
    uint64_t usecs(void) {
        return ((uint64_t)tv_sec * 1000000) + tv_usec;
    }
    uint64_t nsecs(void) {
        return ((uint64_t)tv_sec * 1000000000) + (tv_usec * 1000);
    }
    timeval *operator()(void) { return this; }
};
static inline std::ostream& operator<<(std::ostream& ostr,
                                       const pfk_timeval &rhs)
{
    ostr << "pfk_timeval(" << rhs.tv_sec << "," << rhs.tv_usec << ")";
    return ostr;
}
static inline pfk_timeval operator-(const pfk_timeval &lhs,
                                    const pfk_timeval &rhs) {
    return pfk_timeval(lhs).operator-=(rhs);
}
static inline pfk_timeval operator+(const pfk_timeval &lhs,
                                    const pfk_timeval &rhs) {
    return pfk_timeval(lhs).operator+=(rhs);
}
static inline bool operator>(const pfk_timeval &lhs, const pfk_timeval &other) {
   if (lhs.tv_sec > other.tv_sec) 
      return true;
   if (lhs.tv_sec < other.tv_sec) 
      return false;
   return lhs.tv_usec > other.tv_usec;
}
static inline bool operator<(const pfk_timeval &lhs, const pfk_timeval &other) {
   if (lhs.tv_sec < other.tv_sec) 
      return true;
   if (lhs.tv_sec > other.tv_sec) 
      return false;
   return lhs.tv_usec < other.tv_usec;
}

struct pfk_timespec : public timespec
{
    pfk_timespec(void) { tv_sec = 0; tv_nsec = 0; }
    pfk_timespec(time_t s, long n) { set(s,n); }
    pfk_timespec(const pfk_timespec &other) {
        tv_sec = other.tv_sec; tv_nsec = other.tv_nsec;
    }
    void set(time_t s, long n) { tv_sec = s; tv_nsec = n; }
    const pfk_timespec &operator=(const pfk_timespec &rhs) {
        tv_sec = rhs.tv_sec;
        tv_nsec = rhs.tv_nsec;
        return *this;
    }
    const pfk_timespec &operator=(const timeval &rhs) {
        tv_sec = rhs.tv_sec;
        tv_nsec = rhs.tv_usec * 1000;
        return *this;
    }
    const pfk_timespec &operator-=(const pfk_timespec &rhs) {
        bool borrow = false;
        if (rhs.tv_nsec > tv_nsec)
            borrow = true;
        tv_sec -= rhs.tv_sec;
        tv_nsec -= rhs.tv_nsec;
        if (borrow)
        {
            tv_sec -= 1;
            tv_nsec += 1000000000;
        }
        return *this;
    }
    const pfk_timespec &operator+=(const pfk_timespec &rhs) {
        tv_sec += rhs.tv_sec;
        tv_nsec += rhs.tv_nsec;
        if (tv_nsec > 1000000000)
        {
            tv_nsec -= 1000000000;
            tv_sec += 1;
        }
        return *this;
    }
    const bool operator==(const pfk_timespec &rhs) {
        return (tv_sec == rhs.tv_sec) && (tv_nsec == rhs.tv_nsec);
    }
    const bool operator!=(const pfk_timespec &rhs) {
        return !operator==(rhs);
    }
    const pfk_timespec &getNow(clockid_t clk_id = CLOCK_REALTIME) {
        clock_gettime(clk_id, this);
        return *this;
    }
    const pfk_timespec &getMonotonic(void) {
        return getNow(CLOCK_MONOTONIC);
    }
    const std::string Format(const char *format = NULL) {
        if (format == NULL)
            format = "%Y-%m-%d %H:%M:%S";
        time_t seconds = tv_sec;
        struct tm t;
        localtime_r(&seconds, &t);
        char ymdhms[128], ns[12];
        strftime(ymdhms,sizeof(ymdhms),format,&t);
        ymdhms[sizeof(ymdhms)-1] = 0;
        snprintf(ns,sizeof(ns),"%09ld", tv_nsec);
        ns[sizeof(ns)-1] = 0;
        return std::string(ymdhms) + "." + ns;
    }
    uint32_t msecs(void) {
        return (tv_sec * 1000) + (tv_nsec / 1000000);
    }
    uint64_t usecs(void) {
        return ((uint64_t)tv_sec * 1000000) + (tv_nsec / 1000);
    }
    uint64_t nsecs(void) {
        return ((uint64_t)tv_sec * 1000000000) + tv_nsec;
    }
    timespec *operator()(void) { return this; }
};
static inline std::ostream& operator<<(std::ostream& ostr,
                                       const pfk_timespec &rhs)
{
    ostr << "pfk_timespec(" << rhs.tv_sec << "," << rhs.tv_nsec << ")";
    return ostr;
}
static inline pfk_timespec operator-(const pfk_timespec &lhs,
                                     const pfk_timespec &rhs) {
   return pfk_timespec(lhs).operator-=(rhs);
}
static inline pfk_timespec operator+(const pfk_timespec &lhs,
                                     const pfk_timespec &rhs) {
   return pfk_timespec(lhs).operator+=(rhs);
}
static inline bool operator>(const pfk_timespec &lhs,
                             const pfk_timespec &other) {
   if (lhs.tv_sec > other.tv_sec) 
      return true;
   if (lhs.tv_sec < other.tv_sec) 
      return false;
   return lhs.tv_nsec > other.tv_nsec;
}
static inline bool operator<(const pfk_timespec &lhs,
                             const pfk_timespec &other) {
   if (lhs.tv_sec < other.tv_sec) 
      return true;
   if (lhs.tv_sec > other.tv_sec) 
      return false;
   return lhs.tv_nsec < other.tv_nsec;
}

class pfk_pthread_condattr {
    pthread_condattr_t attr;
public:
    pfk_pthread_condattr(void) {
        pthread_condattr_init(&attr);
    }
    ~pfk_pthread_condattr(void) {
        pthread_condattr_destroy(&attr);
    }
    const pthread_condattr_t *operator()(void) { return &attr; }
    void setclock(clockid_t id) {
        pthread_condattr_setclock(&attr,id);
    }
    void setpshared(bool shared) {
        pthread_condattr_setpshared(
            &attr,
            shared ? PTHREAD_PROCESS_SHARED : PTHREAD_PROCESS_PRIVATE);
    }
};

class pfk_pthread_mutexattr {
    pthread_mutexattr_t  attr;
public:
    pfk_pthread_mutexattr(void) {
        pthread_mutexattr_init(&attr);
    }
    ~pfk_pthread_mutexattr(void) {
        pthread_mutexattr_destroy(&attr);
    }
    pthread_mutexattr_t *operator()(void) { return &attr; }
    // pthread_mutexattr_setprioceiling
    // pthread_mutexattr_setprotocol
    // pthread_mutexattr_setpshared
    // pthread_mutexattr_setrobust
    // pthread_mutexattr_settype
};

class pfk_pthread_mutex {
    pthread_mutex_t  mutex;
    bool initialized;
public:
    pfk_pthread_mutexattr  attr;
    pfk_pthread_mutex(void) {
        initialized = false;
    }
    ~pfk_pthread_mutex(void) {
        if (initialized)
            pthread_mutex_destroy(&mutex);
    }
    void init(void) {
        if (initialized)
            pthread_mutex_destroy(&mutex);
        pthread_mutex_init(&mutex, attr());
        initialized = true;
    }
    void lock(void) { pthread_mutex_lock(&mutex); }
    void trylock(void) { pthread_mutex_trylock(&mutex); }
    void unlock(void) { pthread_mutex_unlock(&mutex); }
    pthread_mutex_t *operator()(void) { return initialized ? &mutex : NULL; }
};

class pfk_pthread_mutex_lock {
    pfk_pthread_mutex &mut;
    bool locked;
public:
     pfk_pthread_mutex_lock(pfk_pthread_mutex &_mut, bool dolock=true)
        : mut(_mut), locked(false) {
        if (dolock) {
            mut.lock();
            locked = true;
        }
    }
    ~pfk_pthread_mutex_lock(void) {
        if (locked) {
            mut.unlock();
        }
    }
    bool lock(void) {
        if (locked)
            return false;
        mut.lock();
        locked = true;
        return true;
    }
    bool unlock(void) {
        if (!locked)
            return false;
        mut.unlock();
        locked = false;
        return true;
    }
};

class pfk_pthread_cond {
    pthread_cond_t  cond;
    bool initialized;
public:
    pfk_pthread_condattr attr;
    pfk_pthread_cond(void) {
        initialized = false;
    }
    ~pfk_pthread_cond(void) {
        if (initialized)
            pthread_cond_destroy(&cond);
    }
    void init(void) {
        if (initialized)
            pthread_cond_destroy(&cond);
        pthread_cond_init(&cond, attr());
        initialized = true;
    }
    pthread_cond_t *operator()(void) { return initialized ? &cond : NULL; }
    int wait(pthread_mutex_t *mut, timespec *abstime = NULL) {
        if (abstime == NULL)
            return pthread_cond_wait(&cond, mut);
        return pthread_cond_timedwait(&cond, mut, abstime);
    }
    void signal(void) {
        pthread_cond_signal(&cond);
    }
    void bcast(void) {
        pthread_cond_broadcast(&cond);
    }
};

// classical counting semaphore like P and V from school.
class pfk_semaphore {
    pfk_pthread_cond cond;
    pfk_pthread_mutex mut;
    int value;
public:
    pfk_semaphore(int initial) {
        mut.init();
        cond.init();
    }
    ~pfk_semaphore(void) { /* what */ }
    void give(void) {
        pfk_pthread_mutex_lock lock(mut);
        value++;
        lock.unlock();
        cond.signal();
    }
    bool take(struct timespec *expire = NULL) {
        pfk_pthread_mutex_lock lock(mut);
        while (value <= 0) {
            if (cond.wait(mut(), expire) < 0)
                return false;
        }
        return true;
    }
};

class pfk_pthread_attr {
    pthread_attr_t _attr;
public:
    pfk_pthread_attr(void) {
        pthread_attr_init(&_attr);
    }
    ~pfk_pthread_attr(void) {
        pthread_attr_destroy(&_attr);
    }
    void set_detach(bool set=true) {
        int state = set ? PTHREAD_CREATE_DETACHED : PTHREAD_CREATE_JOINABLE;
        pthread_attr_setdetachstate(&_attr, state);
    }
    const pthread_attr_t *operator()(void) { return &_attr; }
// pthread_attr_setaffinity_np
// pthread_attr_setguardsize
// pthread_attr_setschedparam
// pthread_attr_setstackaddr
// pthread_attr_setstacksize
};

class pfk_pthread {
    pfk_pthread_mutex mut;
    pfk_pthread_cond cond;
    enum {
        INIT, NEWBORN, RUNNING, STOPPING, ZOMBIE
    } state;
    pthread_t  id;
    static void * _entry(void *arg) {
        pfk_pthread * th = (pfk_pthread *)arg;
        pfk_pthread_mutex_lock lock(th->mut);
        th->state = RUNNING;
        lock.unlock();
        th->cond.signal();
        void * ret = th->entry();
        lock.lock();
        th->state = ZOMBIE;
        return ret;
    }
protected:
    virtual void * entry(void) = 0;
    virtual void send_stop(void) = 0;
public:
    pfk_pthread_attr attr;
    pfk_pthread(void) { state = INIT; mut.init(); cond.init(); }
    ~pfk_pthread(void) {
        // derived class destructor really should do stop/join
        // before destroying anything else in derived, but it doesn't
        // hurt to repeat it here since stop and join both check the state.
        stopjoin();
    }
    int create(void) {
        pfk_pthread_mutex_lock lock(mut);
        if (state != INIT)
            return -1;
        state = NEWBORN;
        lock.unlock();
        attr.set_detach(false); // this class depends on joinable.
        int ret = pthread_create(&id, attr(), &_entry, this);
        lock.lock();
        if (ret == 0) {
            while (state == NEWBORN) {
                pfk_timespec ts(5,0);
                ts += pfk_timespec().getNow();
                cond.wait(mut(), ts());
            }
        } else {
            state = INIT;
        }
        return ret;
    }
    void stop(void) {
        pfk_pthread_mutex_lock lock(mut);
        if (state != RUNNING)
            return;
        state = STOPPING;
        send_stop();
    }
    void * join(void) {
        pfk_pthread_mutex_lock lock(mut);
        if (state != STOPPING && state != ZOMBIE)
            return NULL;
        lock.unlock();
        void * ret = NULL;
        pthread_join(id, &ret);
        lock.lock();
        state = INIT;
        return ret;
    }
    void * stopjoin(void) { stop(); return join(); }
    const bool running(void) const { return (state != INIT); }
};

class pfk_fd_set {
    fd_set  fds;
    int     max_fd;
public:
    pfk_fd_set(void) { zero(); }
    ~pfk_fd_set(void) { /*nothing for now*/ }
    void zero(void) { FD_ZERO(&fds); max_fd=-1; }
    void set(int fd) { FD_SET(fd, &fds); if (fd > max_fd) max_fd = fd; }
    void clr(int fd) { FD_CLR(fd, &fds); }
    bool isset(int fd) { return FD_ISSET(fd, &fds) != 0; }
    fd_set *operator()(void) { return max_fd==-1 ? NULL : &fds; }
    int nfds(void) { return max_fd + 1; }
};

struct pfk_select {
    pfk_fd_set  rfds;
    pfk_fd_set  wfds;
    pfk_fd_set  efds;
    pfk_timeval   tv;
    pfk_select(void) { }
    ~pfk_select(void) { }
    int select(void) {
        int n = rfds.nfds(), n2 = wfds.nfds(), n3 = efds.nfds();
        if (n < n2) n = n2;
        if (n < n3) n = n3;
        return ::select(n, rfds(), wfds(), efds(), tv());
    }
};

class pfk_ticker : public pfk_pthread {
    int closer_pipe_fds[2];
    int pipe_fds[2];
    pfk_timeval interval;
    /*virtual*/ void * entry(void) {
        char c = 1;
        int clfd = closer_pipe_fds[0];
        pfk_select   sel;
        while (1) {
            sel.tv = interval;
            sel.rfds.zero();
            sel.rfds.set(clfd);
            if (sel.select() <= 0) {
                (void) write(pipe_fds[1], &c, 1);
                continue;
            }
            if (sel.rfds.isset(clfd)) {
                (void) read(clfd, &c, 1);
                break;
            }
        }
        return NULL;
    }
    /*virtual*/ void send_stop(void) {
        char c = 1;
        (void) write(closer_pipe_fds[1], &c, 1);
    }
public:
    pfk_ticker(void) {
        pipe(pipe_fds);
        pipe(closer_pipe_fds);
        interval.set(1,0);
    }
    ~pfk_ticker(void) {
        stopjoin();
        close(closer_pipe_fds[0]);
        close(closer_pipe_fds[1]);
        close(pipe_fds[0]);
        close(pipe_fds[1]);
    }
    void start(time_t s, long us) {
        interval.set(s,us);
        if (!running())
            create();
    }
    int fd(void) { return pipe_fds[0]; }
};

class pfk_readdir {
    DIR * d;
public:
    pfk_readdir(void) {
        d = NULL;
    }
    ~pfk_readdir(void) {
        if (d != NULL)
            ::closedir(d);
    }
    bool open(const std::string &dirstr) { return open(dirstr.c_str()); }
    bool open(const char *dirname) {
        if (d)
            ::closedir(d);
        d = ::opendir(dirname);
        if (d == NULL)
            return false;
        return true;
    }
    bool read(dirent &de) {
        if (d == NULL)
            return false;
        dirent * result = NULL;
        int cc = ::readdir_r(d, &de, &result);
        if (cc != 0 || result == NULL)
            return false;
        return true;
    }
    void rewind(void) {
        if (d)
            ::rewinddir(d);
    }
};

#endif /* __pfkposix_h__ */
