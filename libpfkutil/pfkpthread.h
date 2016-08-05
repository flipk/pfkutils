/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

// TODO : strerror_r workarounds
// http://stackoverflow.com/questions/3051204/strerror-r-returns-trash-when-i-manually-set-errno-during-testing
// TODO : error check the hell out of stuff that isn't being error checked.
// TODO : strtok class

#ifndef __pfkpthread_h__
#define __pfkpthread_h__

#include <pthread.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <inttypes.h>
#include <dirent.h>
#include "myTimeval.h"

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

struct pfk_pthread {
    pfk_pthread_attr attr;
    pfk_pthread_mutex mut;
    pfk_pthread_cond cond;
    bool started;
    pthread_t  id;
    pfk_pthread(void) { mut.init(); cond.init(); }
    ~pfk_pthread(void) { }
    int create() {
        started = false;
        int ret = pthread_create(&id, attr(), &_entry, this);
        if (ret == 0) {
            while (!started) {
                myTimespec ts(5,0);
                ts += myTimespec().getNow();
                cond.wait(mut(), &ts);
            }
        }
        return ret;
    }
    void * join(void) {
        void * ret = NULL;
        pthread_join(id, &ret);
        return ret;
    }
    virtual void entry(void) = 0;
private:
    static void * _entry(void *arg) {
        pfk_pthread * th = (pfk_pthread *)arg;
        th->started = true;
        th->cond.signal();
        th->entry();
        return NULL;
    }
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
    myTimeval   tv;
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

#endif /* __pfkpthread_h__ */
