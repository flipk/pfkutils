/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

// TODO : strerror_r workarounds
// http://stackoverflow.com/questions/3051204/strerror-r-returns-trash-when-i-manually-set-errno-during-testing
// TODO : poison file

#ifndef __pfkpthread_h__
#define __pfkpthread_h__

#include <pthread.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
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

struct pfk_pthread {
    pfk_pthread_attr attr;
    pthread_t  id;
    int create() {
        return pthread_create(&id, attr(), &_entry, this);
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

#endif /* __pfkpthread_h__ */
