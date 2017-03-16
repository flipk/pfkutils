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

#ifndef __pfkposix_h__
#define __pfkposix_h__

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
#include <stdlib.h>
#include <iostream>
#include <sstream>
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

class pfk_pthread_mutex {
    pthread_mutex_t  mutex;
    bool initialized;
public:
    pfk_pthread_mutexattr attr;
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
    pthread_mutex_t *operator()(void) { return initialized ? &mutex : NULL; }
    void lock(void) { pthread_mutex_lock(&mutex); }
    void trylock(void) { pthread_mutex_trylock(&mutex); }
    void unlock(void) { pthread_mutex_unlock(&mutex); }
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
    void setpshared(bool shared = true) {
        pthread_condattr_setpshared(
            &attr, shared ? PTHREAD_PROCESS_SHARED : PTHREAD_PROCESS_PRIVATE);
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
    bool take(timespec *expire = NULL) {
        pfk_pthread_mutex_lock lock(mut);
        while (value <= 0) {
            if (cond.wait(mut(), expire) < 0)
                return false;
        }
        value--;
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

class pfk_pipe {
public:
    int readEnd;
    int writeEnd;
    pfk_pipe(void)
    {
        int fds[2];
        if (pipe(fds) < 0)
            fprintf(stderr, "pfk_pipe: pipe failed\n");
        readEnd = fds[0];
        writeEnd = fds[1];
    }
    ~pfk_pipe(void)
    {
        close(writeEnd);
        close(readEnd);
    }
};

class pfk_pthread {
    pfk_pthread_mutex mut;
    pfk_pthread_cond cond;
    enum {
        INIT, NEWBORN, RUNNING, STOPPING, ZOMBIE
    } state;
    pthread_t  id;
    void *user_arg;
    static void * _entry(void *thread_obj) {
        pfk_pthread * th = (pfk_pthread *)thread_obj;
        pfk_pthread_mutex_lock lock(th->mut);
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
    pfk_pthread_attr attr;
    pfk_pthread(void) {
        state = INIT;
        mut.init();
        cond.init();
    }
    ~pfk_pthread(void) {
        // derived class destructor really should do stop/join
        // before destroying anything else in derived, but it doesn't
        // hurt to repeat it here since stop and join both check the state.
        stopjoin();
    }
    int create(void *_user_arg=NULL) {
        pfk_pthread_mutex_lock lock(mut);
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
    /*virtual*/ void * entry(void *arg) {
        char c = 1;
        int clfd = closer_pipe_fds[0];
        pfk_select   sel;
        while (1) {
            sel.tv = interval;
            sel.rfds.zero();
            sel.rfds.set(clfd);
            if (sel.select() <= 0) {
                if (write(pipe_fds[1], &c, 1) < 0)
                    fprintf(stderr, "pfk_ticker: write failed\n");
                continue;
            }
            if (sel.rfds.isset(clfd)) {
                if (read(clfd, &c, 1) < 0)
                    fprintf(stderr, "pfk_ticker: read failed\n");
                break;
            }
        }
        return NULL;
    }
    /*virtual*/ void send_stop(void) {
        char c = 1;
        if (write(closer_pipe_fds[1], &c, 1) < 0)
            fprintf(stderr, "pfk_ticker: write failed\n");
    }
public:
    pfk_ticker(void) {
        if (pipe(pipe_fds) < 0)
            fprintf(stderr, "pfk_ticker: pipe 1 failed\n");
        if (pipe(closer_pipe_fds) < 0)
            fprintf(stderr, "pfk_ticker: pipe 2 failed\n");
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

class pfk_unix_dgram_socket {
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
    pfk_unix_dgram_socket(void) {
        path.clear();
        fd = -1;
    }
    ~pfk_unix_dgram_socket(void) {
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

class pfk_udp_socket {
    int fd;
public:
    pfk_udp_socket(void) {
        fd = -1;
    }
    ~pfk_udp_socket(void) {
        if (fd > 0)
            ::close(fd);
    }
    static const int MAX_MSG_LEN = 16384;
    int getFd(void) const { return fd; }
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
        sockaddr_in sa;
        sa.sin_family = AF_INET;
        sa.sin_port = htons((short)port);
        sa.sin_addr.s_addr = INADDR_ANY;
        if (::bind(fd, (sockaddr *)&sa, sizeof(sa)) < 0)
        {
            int e = errno;
            fprintf(stderr, "bind: %d: %s\n", e, strerror(e));
            return false;
        }
        return true;
    }
    // next 4 are for connected-mode sockets
    bool connect(uint32_t addr, short port) {
        sockaddr_in sa;
        sa.sin_family = AF_INET;
        sa.sin_port = htons(port);
        sa.sin_addr.s_addr = htonl(addr);
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
class _pfk_stream_socket {
    int fd;
    sockaddr_in sa;
    _pfk_stream_socket(int _fd, const sockaddr_in &_sa) {
        fd = _fd;
        sa = _sa;
    }
public:
    _pfk_stream_socket(void) {
        fd = -1;
    }
    ~_pfk_stream_socket(void) {
        if (fd > 0)
            ::close(fd);
    }
    int getFd(void) const { return fd; }
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
            fprintf(stderr, "pfk_sctp_stream_socket: init: %d: %s\n",
                    e, strerror(e));
            return false;
        }
        return true;
    }
    bool connect(uint32_t addr, short port) {
        sa.sin_family = AF_INET;
        sa.sin_port = htons(port);
        sa.sin_addr.s_addr = htonl(addr);
        if (::connect(fd, (sockaddr *)&sa, sizeof(sa)) < 0)
        {
            int e = errno;
            fprintf(stderr, "pfk_sctp_stream_socket: connect: %d: %s\n",
                    e, strerror(e));
            return false;
        }
        return true;
    }
    // next 4 methods are for listening socket (waiting for call in)
    bool init(uint32_t addr, short port) {
        if (init() == false)
            return false;
        sa.sin_family = AF_INET;
        sa.sin_port = htons(port);
        sa.sin_addr.s_addr = htonl(addr);
        if (::bind(fd, (sockaddr *)&sa, sizeof(sa)) < 0)
        {
            int e = errno;
            fprintf(stderr, "pfk_sctp_stream_socket: bind: %d: %s\n",
                    e, strerror(e));
            return false;
        }
        return true;
    }
    bool init(short port) {
        return init(INADDR_ANY,port);
    }
    void listen(void) {
        (void) ::listen(fd, 1);
    }
    // this one returns a connected socket
    _pfk_stream_socket *accept(void) {
        socklen_t sz = sizeof(sa);
        int fdnew = ::accept(fd, (sockaddr *)&sa, &sz);
        if (fdnew < 0)
        {
            int e = errno;
            fprintf(stderr, "pfk_sctp_stream_socket: accept: %d: %s\n",
                    e, strerror(e));
            return NULL;
        }
        _pfk_stream_socket *s = new _pfk_stream_socket(fdnew,sa);
        if (s)
            return s;
        ::close(fdnew);
        return NULL;
    }
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
};

typedef _pfk_stream_socket<IPPROTO_SCTP> pfk_sctp_stream_socket;
typedef _pfk_stream_socket<0> pfk_tcp_stream_socket;

#endif /* __pfkposix_h__ */
