/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

// TODO : strerror_r workarounds
// http://stackoverflow.com/questions/3051204/strerror-r-returns-trash-when-i-manually-set-errno-during-testing
// TODO : error check the hell out of stuff that isn't being error checked.
// TODO : strtok class
// TODO : merge LockWait and the cond/mutex in here?

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
static inline pfk_timeval operator-(const pfk_timeval &lhs, const pfk_timeval &rhs) {
   bool borrow = false;
   pfk_timeval tmp;
   tmp.tv_sec = lhs.tv_sec;
   tmp.tv_usec = lhs.tv_usec;
   if (rhs.tv_usec > lhs.tv_usec)
      borrow = true;
   tmp.tv_sec -= rhs.tv_sec;
   tmp.tv_usec -= rhs.tv_usec;
   if (borrow)
   {
      tmp.tv_sec -= 1;
      tmp.tv_usec += 1000000;
   }
   return tmp;
}
static inline pfk_timeval operator+(const pfk_timeval &lhs, const pfk_timeval &rhs) {
   pfk_timeval tmp;
   tmp.tv_sec = lhs.tv_sec + rhs.tv_sec;
   tmp.tv_usec = lhs.tv_usec + rhs.tv_usec;
   if (tmp.tv_usec > 1000000)
   {
      tmp.tv_usec -= 1000000;
      tmp.tv_sec += 1;
   }
   return tmp;
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
   bool borrow = false;
   pfk_timespec tmp;
   tmp.tv_sec = lhs.tv_sec;
   tmp.tv_nsec = lhs.tv_nsec;
   if (rhs.tv_nsec > lhs.tv_nsec)
      borrow = true;
   tmp.tv_sec -= rhs.tv_sec;
   tmp.tv_nsec -= rhs.tv_nsec;
   if (borrow)
   {
      tmp.tv_sec -= 1;
      tmp.tv_nsec += 1000000000;
   }
   return tmp;
}
static inline pfk_timespec operator+(const pfk_timespec &lhs,
                                   const pfk_timespec &rhs) {
   pfk_timespec tmp;
   tmp.tv_sec = lhs.tv_sec + rhs.tv_sec;
   tmp.tv_nsec = lhs.tv_nsec + rhs.tv_nsec;
   if (tmp.tv_nsec > 1000000000)
   {
      tmp.tv_nsec -= 1000000000;
      tmp.tv_sec += 1;
   }
   return tmp;
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
    int wait(pthread_mutex_t *mut, timespec *abstime) {
        return pthread_cond_timedwait(&cond, mut, abstime);
    }
    void signal(void) {
        pthread_cond_signal(&cond);
    }
    // timedwait
    // signal
    // broadcast
};

class pfk_pthread {
    pfk_pthread_mutex mut;
    pfk_pthread_cond cond;
    enum {
        INIT, NEWBORN, RUNNING, ZOMBIE
    } state;
    pthread_t  id;
    static void * _entry(void *arg) {
        pfk_pthread * th = (pfk_pthread *)arg;
        th->mut.lock();
        th->state = RUNNING;
        th->mut.unlock();
        th->cond.signal();
        th->entry();
        th->mut.lock();
        th->state = ZOMBIE;
        th->mut.unlock();
        return NULL;
    }
protected:
    virtual void entry(void) = 0;
public:
    pfk_pthread_attr attr;
    pfk_pthread(void) { state = INIT; mut.init(); cond.init(); }
    ~pfk_pthread(void) { join(); }
    int create() {
        mut.lock();
        if (state != INIT) {
            mut.unlock();
            return -1;
        }
        state = NEWBORN;
        mut.unlock();
        int ret = pthread_create(&id, attr(), &_entry, this);
        if (ret == 0) {
            mut.lock();
            while (state == NEWBORN) {
                pfk_timespec ts(5,0);
                ts += pfk_timespec().getNow();
                cond.wait(mut(), ts());
            }
            mut.unlock();
        } else {
            mut.lock();
            state = INIT;
            mut.unlock();
        }
        return ret;
    }
    void * join(void) {
        mut.lock();
        if (state != RUNNING && state != ZOMBIE) {
            mut.unlock();
            return NULL;
        }
        mut.unlock();
        void * ret = NULL;
        pthread_join(id, &ret);
        mut.lock();
        state = INIT;
        mut.unlock();
        return ret;
    }
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

class pfk_readdir {
    DIR * d;
    bool isOk;
public:
    pfk_readdir(const char *dirname) {
        d = ::opendir(dirname);
        if (d == NULL)
            isOk = false;
        else
            isOk = true;
    }
    ~pfk_readdir(void) {
        if (d != NULL)
            closedir(d);
    }
    const bool ok(void) const { return isOk; }
    bool read(dirent &de) {
        dirent * result = NULL;
        int cc = readdir_r(d, &de, &result);
        if (cc != 0 || result == NULL)
            return false;
        return true;
    }
    void rewind(void) {
        rewinddir(d);
    }
};

#endif /* __pfkposix_h__ */
