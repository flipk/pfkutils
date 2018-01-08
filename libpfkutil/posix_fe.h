/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */
/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
*/

#ifndef __posix_fe_h__
#define __posix_fe_h__

// stupid redhat
#define __STDC_FORMAT_MACROS 1

#include <pthread.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <inttypes.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>

struct pxfe_timeval : public timeval
{
    pxfe_timeval(void) { tv_sec = 0; tv_usec = 0; }
    pxfe_timeval(time_t s, long u) { set(s,u); }
    pxfe_timeval(const pxfe_timeval &other) {
        tv_sec = other.tv_sec;  tv_usec = other.tv_usec;
    }
    void set(time_t s, long u) { tv_sec = s; tv_usec = u; }
    const pxfe_timeval& operator=(const timeval &rhs) {
        tv_sec = rhs.tv_sec;
        tv_usec = rhs.tv_usec;
        return *this;
    }
    const pxfe_timeval& operator=(const timespec &rhs) {
        tv_sec = rhs.tv_sec;
        tv_usec = rhs.tv_nsec / 1000;
        return *this;
    }
    const pxfe_timeval& operator-=(const pxfe_timeval &rhs) {
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
    const pxfe_timeval& operator+=(const pxfe_timeval &rhs) {
        tv_sec += rhs.tv_sec;
        tv_usec += rhs.tv_usec;
        if (tv_usec > 1000000)
        {
            tv_usec -= 1000000;
            tv_sec += 1;
        }
        return *this;
    }
    const bool operator==(const pxfe_timeval &other) {
        if (tv_sec != other.tv_sec)
            return false;
        if (tv_usec != other.tv_usec)
            return false;
        return true;
    }
    const bool operator!=(const pxfe_timeval &other) {
        return !operator==(other);
    }
    const pxfe_timeval &getNow(void) {
        gettimeofday(this, NULL);
        return *this;
    }
    const std::string Format(const char *format = NULL) {
        if (format == NULL)
            format = "%Y-%m-%d %H:%M:%S";
        time_t seconds = tv_sec;
        tm t;
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
                                       const pxfe_timeval &rhs)
{
    ostr << "pxfe_timeval(" << rhs.tv_sec << "," << rhs.tv_usec << ")";
    return ostr;
}
static inline pxfe_timeval operator-(const pxfe_timeval &lhs,
                                    const pxfe_timeval &rhs) {
    return pxfe_timeval(lhs).operator-=(rhs);
}
static inline pxfe_timeval operator+(const pxfe_timeval &lhs,
                                    const pxfe_timeval &rhs) {
    return pxfe_timeval(lhs).operator+=(rhs);
}
static inline bool operator>(const pxfe_timeval &lhs, const pxfe_timeval &other) {
   if (lhs.tv_sec > other.tv_sec) 
      return true;
   if (lhs.tv_sec < other.tv_sec) 
      return false;
   return lhs.tv_usec > other.tv_usec;
}
static inline bool operator<(const pxfe_timeval &lhs, const pxfe_timeval &other) {
   if (lhs.tv_sec < other.tv_sec) 
      return true;
   if (lhs.tv_sec > other.tv_sec) 
      return false;
   return lhs.tv_usec < other.tv_usec;
}

struct pxfe_timespec : public timespec
{
    pxfe_timespec(void) { tv_sec = 0; tv_nsec = 0; }
    pxfe_timespec(time_t s, long n) { set(s,n); }
    pxfe_timespec(const pxfe_timespec &other) {
        tv_sec = other.tv_sec; tv_nsec = other.tv_nsec;
    }
    void set(time_t s, long n) { tv_sec = s; tv_nsec = n; }
    const pxfe_timespec &operator=(const pxfe_timespec &rhs) {
        tv_sec = rhs.tv_sec;
        tv_nsec = rhs.tv_nsec;
        return *this;
    }
    const pxfe_timespec &operator=(const timeval &rhs) {
        tv_sec = rhs.tv_sec;
        tv_nsec = rhs.tv_usec * 1000;
        return *this;
    }
    const pxfe_timespec &operator-=(const pxfe_timespec &rhs) {
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
    const pxfe_timespec &operator+=(const pxfe_timespec &rhs) {
        tv_sec += rhs.tv_sec;
        tv_nsec += rhs.tv_nsec;
        if (tv_nsec > 1000000000)
        {
            tv_nsec -= 1000000000;
            tv_sec += 1;
        }
        return *this;
    }
    const bool operator==(const pxfe_timespec &rhs) {
        return (tv_sec == rhs.tv_sec) && (tv_nsec == rhs.tv_nsec);
    }
    const bool operator!=(const pxfe_timespec &rhs) {
        return !operator==(rhs);
    }
    const pxfe_timespec &getNow(clockid_t clk_id = CLOCK_REALTIME) {
        clock_gettime(clk_id, this);
        return *this;
    }
    const pxfe_timespec &getMonotonic(void) {
        return getNow(CLOCK_MONOTONIC);
    }
    const std::string Format(const char *format = NULL) {
        if (format == NULL)
            format = "%Y-%m-%d %H:%M:%S";
        time_t seconds = tv_sec;
        tm t;
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
                                       const pxfe_timespec &rhs)
{
    ostr << "pxfe_timespec(" << rhs.tv_sec << "," << rhs.tv_nsec << ")";
    return ostr;
}
static inline pxfe_timespec operator-(const pxfe_timespec &lhs,
                                     const pxfe_timespec &rhs) {
   return pxfe_timespec(lhs).operator-=(rhs);
}
static inline pxfe_timespec operator+(const pxfe_timespec &lhs,
                                     const pxfe_timespec &rhs) {
   return pxfe_timespec(lhs).operator+=(rhs);
}
static inline bool operator>(const pxfe_timespec &lhs,
                             const pxfe_timespec &other) {
   if (lhs.tv_sec > other.tv_sec) 
      return true;
   if (lhs.tv_sec < other.tv_sec) 
      return false;
   return lhs.tv_nsec > other.tv_nsec;
}
static inline bool operator<(const pxfe_timespec &lhs,
                             const pxfe_timespec &other) {
   if (lhs.tv_sec < other.tv_sec) 
      return true;
   if (lhs.tv_sec > other.tv_sec) 
      return false;
   return lhs.tv_nsec < other.tv_nsec;
}

class pxfe_string : public std::string {
public:
    pxfe_string(void) { }
    pxfe_string(const char *s, size_t len) : std::string(s,len) { }
    void * vptr(void) {
        return (void*) c_str();
    }
    const void * vptr(void) const {
        return (const void*) c_str();
    }
    unsigned char * ucptr(void) {
        return (unsigned char *) c_str();
    }
    const unsigned char * ucptr(void) const {
        return (const unsigned char *) c_str();
    }
    uint8_t * u8ptr(void) {
        return (uint8_t *) c_str();
    }
    const uint8_t * u8ptr(void) const {
        return (const uint8_t *) c_str();
    }
    std::string format_hex(void) {
        std::ostringstream out;
        // at() returns a signed char, but the char and unsigned char
        // overloads for ostream operator<< try to output the char.
        // we want a binary to hex conversion that you get with the
        // "int" operator<< overload.  but we can't cast directly from
        // "char" to "int" because that will sign-extend, so cast to
        // unsigned char first.
        for (int i = 0; i < length(); i++)
            out << std::hex << std::setw(2) << std::setfill('0') <<
                (int)((unsigned char)at(i));
        return out.str();
    }
    ssize_t read(int fd, size_t max) {
        resize(max);
        ssize_t cc = ::read(fd, vptr(), max);
        resize(cc > 0 ? cc : 0);
        return cc;
    }
    ssize_t write(int fd) const {
        ssize_t cc = ::write(fd, vptr(), length());
        return cc;
    }
    void operator=(const std::string &other) {
        assign(other.c_str(), other.length());
    }
};

class pxfe_pthread_mutexattr {
    pthread_mutexattr_t  attr;
public:
    pxfe_pthread_mutexattr(void) {
        pthread_mutexattr_init(&attr);
    }
    ~pxfe_pthread_mutexattr(void) {
        pthread_mutexattr_destroy(&attr);
    }
    pthread_mutexattr_t *operator()(void) { return &attr; }
    void setpshared(bool shared = true) {
        pthread_mutexattr_setpshared(
            &attr,
            shared ? PTHREAD_PROCESS_SHARED : PTHREAD_PROCESS_PRIVATE);
    }
    // args: PTHREAD_PRIO_INHERIT, NONE, PROTECT
    void setproto(int proto) {
        pthread_mutexattr_setprotocol(&attr, proto);
    }
    // args: PTHREAD_MUTEX_NORMAL, ERRORCHECK, RECURSIVE, DEFAULT
    void settype(int type) {
        pthread_mutexattr_settype(&attr, type);
    }
#ifdef PTHREAD_MUTEX_ROBUST
    void setrobust(bool robust = true) {
        pthread_mutexattr_setrobust(
            &attr, robust ? PTHREAD_MUTEX_ROBUST : PTHREAD_MUTEX_STALLED);
    }
#else
    void setrobust(bool robust = true) {
        std::cerr << " WARNING : ROBUST MUTEX NOT SUPPORTED" << std::endl;
    }
#endif
    // pthread_mutexattr_setprioceiling
};

class pxfe_pthread_mutex {
    pthread_mutex_t  mutex;
    bool initialized;
public:
    pxfe_pthread_mutexattr attr;
    pxfe_pthread_mutex(void) {
        initialized = false;
    }
    ~pxfe_pthread_mutex(void) {
        if (initialized)
            pthread_mutex_destroy(&mutex);
    }
    void init(void) {
        if (initialized)
            pthread_mutex_destroy(&mutex);
        pthread_mutex_init(&mutex, attr());
        initialized = true;
    }
    pthread_mutex_t *operator()(void) { return initialized ? &mutex : NULL; }
    void lock(void) {
        if (!initialized)
            std::cerr << "pxfe_pthread_mutex::lock: not initialized\n";
        else
            pthread_mutex_lock(&mutex);
    }
    int trylock(void) {
        if (!initialized) {
            std::cerr << "pxfe_pthread_mutex::trylock: not initialized\n";
            return EINVAL;
        }
        // else
        return pthread_mutex_trylock(&mutex);
    }
    void unlock(void) {
        if (!initialized)
            std::cerr << "pxfe_pthread_mutex::unlock: not initialized\n";
        else
            pthread_mutex_unlock(&mutex);
    }
};

class pxfe_pthread_mutex_lock {
    pxfe_pthread_mutex &mut;
    bool locked;
public:
     pxfe_pthread_mutex_lock(pxfe_pthread_mutex &_mut, bool dolock=true)
        : mut(_mut), locked(false) {
        if (dolock) {
            mut.lock();
            locked = true;
        }
    }
    ~pxfe_pthread_mutex_lock(void) {
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

class pxfe_pthread_condattr {
    pthread_condattr_t attr;
public:
    pxfe_pthread_condattr(void) {
        pthread_condattr_init(&attr);
    }
    ~pxfe_pthread_condattr(void) {
        pthread_condattr_destroy(&attr);
    }
    const pthread_condattr_t *operator()(void) { return &attr; }
    void setclock(clockid_t id) {
        pthread_condattr_setclock(&attr,id);
    }
    void setpshared(bool shared = true) {
        pthread_condattr_setpshared(
            &attr, shared ? PTHREAD_PROCESS_SHARED : PTHREAD_PROCESS_PRIVATE);
    }
};

class pxfe_pthread_cond {
    pthread_cond_t  cond;
    bool initialized;
public:
    pxfe_pthread_condattr attr;
    pxfe_pthread_cond(void) {
        initialized = false;
    }
    ~pxfe_pthread_cond(void) {
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
        if (!initialized) {
            std::cerr << "pxfe_pthread_cond::wait: not initialized\n";
            return EINVAL;
        } else if (abstime == NULL)
            return pthread_cond_wait(&cond, mut);
        return pthread_cond_timedwait(&cond, mut, abstime);
    }
    void signal(void) {
        if (!initialized)
            std::cerr << "pxfe_pthread_cond::signal: not initialized\n";
        else
            pthread_cond_signal(&cond);
    }
    void bcast(void) {
        if (!initialized)
            std::cerr << "pxfe_pthread_cond::bcast: not initialized\n";
        else
            pthread_cond_broadcast(&cond);
    }
};

// classical counting semaphore like P and V from school.
class pxfe_semaphore {
    pxfe_pthread_cond cond;
    pxfe_pthread_mutex mut;
    int value;
public:
    pxfe_semaphore(int initial) {
        mut.init();
        cond.init();
    }
    ~pxfe_semaphore(void) { /* what */ }
    void give(void) {
        pxfe_pthread_mutex_lock lock(mut);
        value++;
        lock.unlock();
        cond.signal();
    }
    bool take(timespec *expire = NULL) {
        pxfe_pthread_mutex_lock lock(mut);
        while (value <= 0) {
            if (cond.wait(mut(), expire) < 0)
                return false;
        }
        value--;
        return true;
    }
};

class pxfe_pthread_attr {
    pthread_attr_t _attr;
public:
    pxfe_pthread_attr(void) {
        pthread_attr_init(&_attr);
    }
    ~pxfe_pthread_attr(void) {
        pthread_attr_destroy(&_attr);
    }
    const pthread_attr_t *operator()(void) { return &_attr; }
    void set_detach(bool set=true) {
        int state = set ? PTHREAD_CREATE_DETACHED : PTHREAD_CREATE_JOINABLE;
        pthread_attr_setdetachstate(&_attr, state);
    }
    // pthread_attr_setaffinity_np
    // pthread_attr_setguardsize
    // pthread_attr_setinheritsched
    // pthread_attr_setschedparam
    // pthread_attr_setschedpolicy
    // pthread_attr_setscope
    // pthread_attr_setstack
    // pthread_attr_setstackaddr
    // pthread_attr_setstacksize
};

class pxfe_pipe {
public:
    int readEnd;
    int writeEnd;
    pxfe_pipe(void)
    {
        int fds[2];
        if (pipe(fds) < 0)
            fprintf(stderr, "pxfe_pipe: pipe failed\n");
        readEnd = fds[0];
        writeEnd = fds[1];
    }
    ~pxfe_pipe(void)
    {
        close(writeEnd);
        close(readEnd);
    }
    int read(void *buf, size_t count)
    {
        return ::read(readEnd, buf, count);
    }
    int write(void *buf, size_t count)
    {
        return ::write(writeEnd, buf, count);
    }
};

class pxfe_pthread {
    pxfe_pthread_mutex mut;
    pxfe_pthread_cond cond;
    enum {
        INIT, NEWBORN, RUNNING, STOPPING, ZOMBIE
    } state;
    pthread_t  id;
    void *user_arg;
    static void * _entry(void *thread_obj) {
        pxfe_pthread * th = (pxfe_pthread *)thread_obj;
        pxfe_pthread_mutex_lock lock(th->mut);
        th->state = RUNNING;
        lock.unlock();
        th->cond.signal();
        void * ret = th->entry(th->user_arg);
        lock.lock();
        th->state = ZOMBIE;
        return ret;
    }
protected:
    virtual void * entry(void *arg) = 0;
    virtual void send_stop(void) = 0;
public:
    pxfe_pthread_attr attr;
    pxfe_pthread(void) {
        state = INIT;
        mut.init();
        cond.init();
    }
    ~pxfe_pthread(void) {
        // derived class destructor really should do stop/join
        // before destroying anything else in derived, but it doesn't
        // hurt to repeat it here since stop and join both check the state.
        stopjoin();
    }
    int create(void *_user_arg=NULL) {
        pxfe_pthread_mutex_lock lock(mut);
        if (state != INIT)
            return -1;
        state = NEWBORN;
        lock.unlock();
        attr.set_detach(false); // this class depends on joinable.
        user_arg = _user_arg;
        int ret = pthread_create(&id, attr(), &_entry, this);
        lock.lock();
        if (ret == 0) {
            while (state == NEWBORN) {
                pxfe_timespec ts(5,0);
                ts += pxfe_timespec().getNow();
                cond.wait(mut(), ts());
            }
        } else {
            state = INIT;
        }
        return ret;
    }
    void stop(void) {
        pxfe_pthread_mutex_lock lock(mut);
        if (state != RUNNING)
            return;
        state = STOPPING;
        send_stop();
    }
    void * join(void) {
        pxfe_pthread_mutex_lock lock(mut);
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

class pxfe_fd_set {
    fd_set  fds;
    int     max_fd;
public:
    pxfe_fd_set(void) { zero(); }
    ~pxfe_fd_set(void) { /*nothing for now*/ }
    void zero(void) { FD_ZERO(&fds); max_fd=-1; }
    void set(int fd) { FD_SET(fd, &fds); if (fd > max_fd) max_fd = fd; }
    void clr(int fd) { FD_CLR(fd, &fds); }
    bool is_set(int fd) { return FD_ISSET(fd, &fds) != 0; }
    fd_set *operator()(void) { return max_fd==-1 ? NULL : &fds; }
    int nfds(void) { return max_fd + 1; }
};

struct pxfe_select {
    pxfe_fd_set  rfds;
    pxfe_fd_set  wfds;
    pxfe_fd_set  efds;
    pxfe_timeval   tv;
    pxfe_select(void) { }
    ~pxfe_select(void) { }
    int select_forever(void) {
        return _select(NULL);
    }
    int select(void) {
        return _select(tv());
    }
    int _select(struct timeval *tvp) {
        int n = rfds.nfds(), n2 = wfds.nfds(), n3 = efds.nfds();
        if (n < n2) n = n2;
        if (n < n3) n = n3;
        return ::select(n, rfds(), wfds(), efds(), tvp);
    }
};

class pxfe_ticker : public pxfe_pthread {
    bool paused;
    int closer_pipe_fds[2];
    int pipe_fds[2];
    pxfe_timeval interval;
    /*virtual*/ void * entry(void *arg) {
        char c = 1;
        int clfd = closer_pipe_fds[0];
        pxfe_select   sel;
        while (1) {
            sel.tv = interval;
            sel.rfds.zero();
            sel.rfds.set(clfd);
            if (sel.select() <= 0) {
                if (paused == false)
                    if (write(pipe_fds[1], &c, 1) < 0)
                        fprintf(stderr, "pxfe_ticker: write failed\n");
                continue;
            }
            if (sel.rfds.is_set(clfd)) {
                if (read(clfd, &c, 1) < 0)
                    fprintf(stderr, "pxfe_ticker: read failed\n");
                break;
            }
        }
        return NULL;
    }
    /*virtual*/ void send_stop(void) {
        char c = 1;
        if (write(closer_pipe_fds[1], &c, 1) < 0)
            fprintf(stderr, "pxfe_ticker: write failed\n");
    }
public:
    pxfe_ticker(void) {
        if (pipe(pipe_fds) < 0)
            fprintf(stderr, "pxfe_ticker: pipe 1 failed\n");
        if (pipe(closer_pipe_fds) < 0)
            fprintf(stderr, "pxfe_ticker: pipe 2 failed\n");
        interval.set(1,0);
        paused = false;
    }
    ~pxfe_ticker(void) {
        paused = true;
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
        paused = false;
    }
    void pause(void) { paused = true; }
    void resume(void) { paused = false; }
    int fd(void) { return pipe_fds[0]; }
};

class pxfe_readdir {
    DIR * d;
public:
    pxfe_readdir(void) {
        d = NULL;
    }
    ~pxfe_readdir(void) {
        close();
    }
    void close(void) {
        if (d)
            ::closedir(d);
        d = NULL;
    }
    bool open(const std::string &dirstr) { return open(dirstr.c_str()); }
    bool open(const char *dirname) {
        close();
        d = ::opendir(dirname);
        if (d == NULL)
            return false;
        return true;
    }
    bool read(dirent &de) {
        if (d == NULL)
            return false;
        dirent * result = readdir(d);
        if (result == NULL)
            return false;
        de = *result;
        return true;
    }
    void rewind(void) {
        if (d)
            ::rewinddir(d);
    }
};

struct pxfe_sockaddr_in : public sockaddr_in {
    void init(void) { sin_family = AF_INET; }
    void init_any(uint16_t p) { init(); set_addr(INADDR_ANY); set_port(p); }
    void init(uint32_t a, uint16_t p) { init(); set_addr(a); set_port(p); }
    uint32_t get_addr(void) const { return ntohl(sin_addr.s_addr); }
    void set_addr(uint32_t a) { sin_addr.s_addr = htonl(a); }
    uint16_t get_port(void) const { return ntohs(sin_port); }
    void set_port(uint16_t p) { sin_port = htons(p); }
    sockaddr *operator()() { return (sockaddr *)this; }
    const sockaddr *operator()() const { return (sockaddr *)this; }
};

class pxfe_iputils {
public:
    static bool hostname_to_ipaddr( const std::string &host,
                                    uint32_t * _addr )
    {
        return hostname_to_ipaddr(host.c_str(), _addr);
    }
    static bool hostname_to_ipaddr( const char * host, uint32_t * _addr )
    {
        uint32_t addr;
        if ( ! (inet_aton( host, (in_addr*) &addr )))
        {
            struct hostent * he;
            if (( he = gethostbyname( host )) == NULL )
            {
                const char * reason = "UNKNOWN";
                switch (h_errno)
                {
                case HOST_NOT_FOUND: reason = "host not found";    break;
                case NO_DATA:        reason = "no data returned";  break;
                case NO_RECOVERY:    reason = "name server error"; break;
                case TRY_AGAIN:      reason = "try again";         break;
                }
                fprintf( stderr, "host lookup of %s: %s\n", host, reason );
                return false;
            }
            memcpy( &addr, he->h_addr, he->h_length );
        }
        addr = ntohl(addr);
        *_addr = addr;
        return true;
    }
    static bool parse_port_number( const std::string &s, uint16_t *_port )
    {
        return parse_port_number(s.c_str(), _port);
    }
    static bool parse_port_number( const char * portstr, uint16_t *_port )
    {
        char * endptr = NULL;
        unsigned long val = strtoul(portstr, &endptr, 0);
        if (endptr != NULL && *endptr == 0)
        {
            // consumed the whole input string, thus good integer.
            if (val > 65535)
            {
                fprintf(stderr, "value %lu is outside valid range for "
                        "a port number\n", val);
                return false;
            }
            *_port = (uint16_t) val;
            return true;
        }
        fprintf(stderr, "parse error: expecting a port number, not '%s'\n",
                portstr);
        // string was not an integer
        return false;
    }
};

class pxfe_unix_dgram_socket {
    static int counter;
    std::string path;
    int fd;
    bool init_common(bool new_path) {
        static int counter = 1;
        if (new_path)
        {
            const char * temp_path = getenv("TMP");
            if (temp_path == NULL)
                temp_path = getenv("TEMP");
            if (temp_path == NULL)
                temp_path = getenv("TMPDIR");
            if (temp_path == NULL)
                temp_path = "/tmp";
            std::ostringstream  str;
            str << temp_path << "/udslibtmp." << getpid() << "." << counter;
            counter++;
            path = str.str();
        }
        fd = ::socket(AF_UNIX, SOCK_DGRAM, 0);
        if (fd < 0)
        {
            int e = errno;
            std::cerr << "socket: " << strerror(e) << std::endl;
            return false;
        }
        sockaddr_un sa;
        sa.sun_family = AF_UNIX;
        int len = sizeof(sa.sun_path)-1;
        strncpy(sa.sun_path, path.c_str(), len);
        sa.sun_path[len] = 0;
        if (::bind(fd, (sockaddr *)&sa, sizeof(sa)) < 0)
        {
            int e = errno;
            std::cerr << "bind dgram: " << strerror(e) << std::endl;
            ::close(fd);
            fd = -1;
            return false;
        }
        return true;
    }
public:
    pxfe_unix_dgram_socket(void) {
        path.clear();
        fd = -1;
    }
    ~pxfe_unix_dgram_socket(void) {
        if (fd > 0)
        {
            ::close(fd);
            (void) unlink( path.c_str() );
        }
    }
    bool init(void) { return init_common(true); }
    bool init(const std::string &_path) { 
        path = _path;
        return init_common(false);
    }
    void close(void) {
        if (fd > 0)
        {
            ::close(fd);
            (void) unlink( path.c_str() );
        }
        fd = -1;
    }
    static const int MAX_MSG_LEN = 16384;
    const std::string &getPath(void) const { return path; }
    int getFd(void) const { return fd; }
    // next 3 are for connected-mode sockets
    void connect(const std::string &remote_path) {
        sockaddr_un addr;
        addr.sun_family = AF_UNIX;
        int len = sizeof(addr.sun_path)-1;
        strncpy(addr.sun_path, remote_path.c_str(), len);
        addr.sun_path[len] = 0;
        if (::connect(fd, (sockaddr *)&addr, sizeof(addr)) < 0)
        {
            int e = errno;
            std::cerr << "connect: " << e << ": " << strerror(e) << std::endl;
        }
    }
    bool send(const std::string &msg) {
        if (msg.size() > MAX_MSG_LEN)
        {
            std::cerr << "ERROR unix_dgram_socket send msg size of "
                 << msg.size() << " is greater than max "
                 << MAX_MSG_LEN << std::endl;
            return false;
        }
        if (::send(fd, msg.c_str(), msg.size(), /*flags*/0) < 0)
        {
            int e = errno;
            std::cerr << "send: " << e << ": " << strerror(e) << std::endl;
            return false;
        }
        return true;
    }
    bool recv(std::string &msg) {
        msg.resize(MAX_MSG_LEN);
        ssize_t msglen = ::recv(fd, (void*) msg.c_str(),
                                MAX_MSG_LEN, /*flags*/0);
        if (msglen <= 0)
        {
            int e = errno;
            std::cerr << "recv: " << e << ": " << strerror(e) << std::endl;
            msg.resize(0);
            return false;
        }
        msg.resize(msglen);
        return true;
    }
    // next 2 are for promiscuous datagram sockets
    bool send(const std::string &msg, const std::string &remote_path) {
        if (msg.size() > MAX_MSG_LEN)
        {
            std::cerr << "ERROR unix_dgram_socket send msg size of "
                 << msg.size() << " is greater than max "
                 << MAX_MSG_LEN << std::endl;
            return false;
        }
        sockaddr_un sa;
        sa.sun_family = AF_UNIX;
        int len = sizeof(sa.sun_path)-1;
        strncpy(sa.sun_path, remote_path.c_str(), len);
        sa.sun_path[len] = 0;
        if (::sendto(fd, msg.c_str(), msg.size(), /*flags*/0,
                     (sockaddr *)&sa, sizeof(sa)) < 0)
        {
            int e = errno;
            std::cerr << "sendto: " << e << ": " << strerror(e) << std::endl;
            return false;
        }
        return true;
    }
    bool recv(      std::string &msg,       std::string &remote_path) {
        msg.resize(MAX_MSG_LEN);
        sockaddr_un sa;
        socklen_t salen = sizeof(sa);
        ssize_t msglen = ::recvfrom(fd, (void*) msg.c_str(), MAX_MSG_LEN,
                                    /*flags*/0, (sockaddr *)&sa, &salen);
        if (msglen <= 0)
        {
            int e = errno;
            std::cerr << "recvfrom: " << e << ": " << strerror(e) << std::endl;
            msg.resize(0);
            return false;
        }
        msg.resize(msglen);
        remote_path.assign(sa.sun_path);
        return true;
    }
};

class pxfe_udp_socket {
    int fd;
public:
    pxfe_udp_socket(void) {
        fd = -1;
    }
    ~pxfe_udp_socket(void) {
        if (fd > 0)
            ::close(fd);
    }
    static const int MAX_MSG_LEN = 16384;
    int getFd(void) const { return fd; }
    void setFd(int _fd) {
        if (fd > 0)
            ::close(fd);
        fd = _fd;
    }
    bool init(void) {
        return init(-1);
    }
    bool init(int port) {
        fd = ::socket(AF_INET, SOCK_DGRAM, 0);
        if (fd < 0)
        {
            int e = errno;
            fprintf(stderr, "socket: %d: %s\n", e, strerror(e));
            return false;
        }
        if (port == -1)
            // no bind needed
            return true;
        pxfe_sockaddr_in sa;
        sa.init_any(port);
        if (::bind(fd, sa(), sizeof(sa)) < 0)
        {
            int e = errno;
            fprintf(stderr, "bind: %d: %s\n", e, strerror(e));
            return false;
        }
        return true;
    }
    bool init_proto(int type, int protocol) {
        fd = ::socket(AF_INET, type, protocol);
        if (fd < 0)
        {
            int e = errno;
            fprintf(stderr, "socket: %d: %s\n", e, strerror(e));
            return false;
        }
        return true;
    }
    // next 4 are for connected-mode sockets
    bool connect(uint32_t addr, short port) {
        pxfe_sockaddr_in sa;
        sa.init(addr, port);
        return connect(sa);
    }
    bool connect(const sockaddr_in &sa) {
        if (::connect(fd, (sockaddr *)&sa, sizeof(sa)) < 0)
        {
            int e = errno;
            fprintf(stderr, "connect: %d: %s\n", e, strerror(e));
            return false;
        }
        return true;
    }
    bool send(const std::string &msg) {
        if (msg.size() > MAX_MSG_LEN)
        {
            std::cerr << "ERROR unix_dgram_socket send msg size of "
                 << msg.size() << " is greater than max "
                 << MAX_MSG_LEN << std::endl;
            return false;
        }
        if (::send(fd, msg.c_str(), msg.size(), /*flags*/0) < 0)
        {
            int e = errno;
            std::cerr << "send: " << e << ": " << strerror(e) << std::endl;
            return false;
        }
        return true;
    }
    bool recv(std::string &msg) {
        msg.resize(MAX_MSG_LEN);
        ssize_t msglen = ::recv(fd, (void*) msg.c_str(),
                                MAX_MSG_LEN, /*flags*/0);
        if (msglen <= 0)
        {
            int e = errno;
            std::cerr << "recv: " << e << ": " << strerror(e) << std::endl;
            msg.resize(0);
            return false;
        }
        msg.resize(msglen);
        return true;
    }
    // next 2 are for promiscuous datagram sockets
    bool send(const std::string &msg, const sockaddr_in &sa) {
        if (msg.size() > MAX_MSG_LEN)
        {
            std::cerr << "ERROR unix_dgram_socket send msg size of "
                 << msg.size() << " is greater than max "
                 << MAX_MSG_LEN << std::endl;
            return false;
        }
        if (::sendto(fd, msg.c_str(), msg.size(), /*flags*/0,
                     (sockaddr *)&sa, sizeof(sa)) < 0)
        {
            int e = errno;
            std::cerr << "sendto: " << e << ": " << strerror(e) << std::endl;
            return false;
        }
        return true;
    }
    bool recv(std::string &msg, sockaddr_in &sa) {
        msg.resize(MAX_MSG_LEN);
        socklen_t salen = sizeof(sa);
        ssize_t msglen = ::recvfrom(fd, (void*) msg.c_str(), MAX_MSG_LEN,
                                    /*flags*/0, (sockaddr *)&sa, &salen);
        if (msglen <= 0)
        {
            int e = errno;
            std::cerr << "recvfrom: " << e << ": " << strerror(e) << std::endl;
            msg.resize(0);
            return false;
        }
        msg.resize(msglen);
        return true;
    }
};

template <int protocolNumber>
class _pxfe_stream_socket {
    int fd;
    pxfe_sockaddr_in sa;
    _pxfe_stream_socket(int _fd, const sockaddr_in &_sa) {
        fd = _fd;
        sa = (pxfe_sockaddr_in&)_sa;
    }
public:
    _pxfe_stream_socket(void) {
        fd = -1;
    }
    ~_pxfe_stream_socket(void) {
        if (fd > 0)
            ::close(fd);
    }
    int getFd(void) const { return fd; }
    void setFd(int _fd) {
        if (fd > 0)
            ::close(fd);
        fd = _fd;
    }
    void close(void) {
        if (fd > 0)
            ::close(fd);
        fd = -1;
    }
    static const int MAX_MSG_LEN = 16384;
    // next 2 methods for connecting socket (calling out)
    bool init(void) {
        fd = ::socket(PF_INET, SOCK_STREAM, protocolNumber);
        if (fd < 0)
        {
            int e = errno;
            fprintf(stderr, "pxfe_sctp_stream_socket: init: %d: %s\n",
                    e, strerror(e));
            return false;
        }
        return true;
    }
    bool connect(uint32_t addr, short port) {
        sa.init(addr, port);
        if (::connect(fd, sa(), sizeof(sa)) < 0)
        {
            int e = errno;
            fprintf(stderr, "pxfe_sctp_stream_socket: connect: %d: %s\n",
                    e, strerror(e));
            return false;
        }
        return true;
    }
    // next 4 methods are for listening socket (waiting for call in)
    bool init(uint32_t addr, short port, bool reuse=false) {
        if (init() == false)
            return false;
        sa.init(addr, port);
        if (reuse) {
            int v = 1;
            setsockopt( fd, SOL_SOCKET, SO_REUSEADDR, (void*) &v, sizeof( v ));
        }
        if (::bind(fd, sa(), sizeof(sa)) < 0)
        {
            int e = errno;
            fprintf(stderr, "pxfe_sctp_stream_socket: bind: %d: %s\n",
                    e, strerror(e));
            return false;
        }
        return true;
    }
    bool init(short port, bool reuse=false) {
        return init(INADDR_ANY,port,reuse);
    }
    void listen(void) {
        (void) ::listen(fd, 1);
    }
    // this one returns a connected socket
    _pxfe_stream_socket *accept(void) {
        socklen_t sz = sizeof(sa);
        int fdnew = ::accept(fd, sa(), &sz);
        if (fdnew < 0)
        {
            int e = errno;
            fprintf(stderr, "pxfe_sctp_stream_socket: accept: %d: %s\n",
                    e, strerror(e));
            return NULL;
        }
        _pxfe_stream_socket *s = new _pxfe_stream_socket(fdnew,sa);
        if (s)
            return s;
        ::close(fdnew);
        return NULL;
    }
    uint32_t get_peer_addr(void) const { return sa.get_addr(); }
    // next 2 methods are for connected sockets
    bool send(const std::string &msg) {
        if (msg.size() > MAX_MSG_LEN)
        {
            std::cerr << "ERROR send msg size of "
                      << msg.size() << " is greater than max "
                      << MAX_MSG_LEN << std::endl;
            return false;
        }
        if (::send(fd, msg.c_str(), msg.size(), /*flags*/0) < 0)
        {
            int e = errno;
            std::cerr << "send: " << e << ": " << strerror(e) << std::endl;
            return false;
        }
        return true;
    }
    bool recv(std::string &msg) {
        msg.resize(MAX_MSG_LEN);
        ssize_t msglen = ::recv(fd, (void*) msg.c_str(),
                                MAX_MSG_LEN, /*flags*/0);
        if (msglen < 0)
        {
            int e = errno;
            std::cerr << "recv: " << e << ": " << strerror(e) << std::endl;
            msg.resize(0);
            return false;
        }
        msg.resize(msglen);
        return true;
    }
};

typedef _pxfe_stream_socket<IPPROTO_SCTP> pxfe_sctp_stream_socket;
typedef _pxfe_stream_socket<0> pxfe_tcp_stream_socket;

#endif /* __posix_fe_h__ */
