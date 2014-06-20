/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#ifndef __PRIO_WORKQ_H__
#define __PRIO_WORKQ_H__

#include <inttypes.h>
#include <pthread.h>

#include "LockWait.h"

class prioWorkQJob {
    friend class prioWorkQ;
    prioWorkQJob * next;
    int prio;
public:
    prioWorkQJob(int _prio) {
        prio = _prio;
    }
    virtual ~prioWorkQJob(void) { /*placeholder*/ }
    virtual void job(void) = 0;
};

class prioWorkQ : public Lockable, public Waitable {
public:
    static const int NUM_PRIOS = 32;
private:
    prioWorkQJob * prioQueue_heads[NUM_PRIOS];
    prioWorkQJob * prioQueue_tails[NUM_PRIOS];
public:
    prioWorkQ(void);
    ~prioWorkQ(void);
    void push(prioWorkQJob *job);
    bool runOne(bool wait); // return false if empty
};

#endif /* __PRIO_WORKQ_H__ */
