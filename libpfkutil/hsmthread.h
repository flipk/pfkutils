/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#ifndef __HSMTHREAD_H__
#define __HSMTHREAD_H__

#include <pthread.h>
#include <string>

#include "throwBacktrace.h"
#include  "dll3.h"

namespace HSMThread {

struct ThreadError : public ThrowUtil::ThrowBackTrace
{
    enum ThreadErrorValue {
        RunningInDestructor,
        ThreadStartAlreadyStarted,
        ThreadsStartAlreadyStarted,
        __NUMERRS
    } err;
    static const std::string errStrings[__NUMERRS];
    ThreadError(ThreadErrorValue _e) : err(_e) { }
    const std::string Format(void) const;
};

class Thread;
typedef DLL3::List<Thread,1> ThreadList_t;

class Thread : public ThreadList_t::Links {
    friend class Threads;
    pthread_t id;
    enum { NEWBORN, RUNNING, DEAD } state;
    std::string  name;
    static void * __entry( void * );
    void _entry(void);
    bool joined;
protected:
    virtual void entry(void) = 0;
    virtual void stopReq(void) = 0;
public:
    Thread(const std::string &_name);
    virtual ~Thread(void);
    const std::string &getName(void) { return name; }
    void start(void);
    void stop(void);
    void join(void);
};

class Threads {
    friend class Thread;
    static Threads * _instance;
    enum { NEWBORN, RUNNING } state;
    ThreadList_t  list;
    struct startupSync {
        int numStarted;
        WaitUtil::Waitable  slaveWait;
    };
    struct startupSync  syncPt;
    Threads(void);
    ~Threads(void);
    void _start(void);
    void _cleanup(void);
public:
    void   registerThread(Thread *);
    void unregisterThread(Thread *);
    // TODO : register timer manager
    static Threads * instance(void);
    static void start(void) { instance()->_start(); }
    static void cleanup(void) { instance()->_cleanup(); }
};

}; // namespace HSMThread

#endif /* __HSMTHREAD_H__ */
