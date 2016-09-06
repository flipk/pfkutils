/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#ifndef __HSMTHREAD_H__
#define __HSMTHREAD_H__

#include <pthread.h>
#include <string>

#include "BackTrace.h"
#include  "dll3.h"

#ifdef __GNUC__
# if __GNUC__ >= 6
#  define ALLOW_THROWS noexcept(false)
# else
#  define ALLOW_THROWS
# endif
#else
# define ALLOW_THROWS
#endif

namespace HSMThread {

/** thread-related errors that may be encountered */
struct ThreadError : public BackTraceUtil::BackTrace
{
    /** list of thread-related errors that might be thrown */
    enum ThreadErrorValue {
        RunningInDestructor,        //!< destructor, but thread is still running
        ThreadStartAlreadyStarted,  //!< Thread::start called twice
        ThreadsStartAlreadyStarted, //!< Threads::start called twice
        __NUMERRS
    } err;
    static const std::string errStrings[__NUMERRS];
    ThreadError(ThreadErrorValue _e) : err(_e) { }
    /** utility function to format printable string of error and backtrace */
    /*virtual*/ const std::string _Format(void) const;
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
    virtual ~Thread(void) ALLOW_THROWS;
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
