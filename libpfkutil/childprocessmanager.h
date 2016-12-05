/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#ifndef __childprocessmanager_h__
#define __childprocessmanager_h__

#include <signal.h>
#include <pthread.h>
#include <unistd.h>

#include <map>
#include <vector>

#include "LockWait.h"

namespace ChildProcessManager {

typedef std::vector<const char*> commandVector;

class Manager;

// this class is a little brittle.
// don't delete it while a child is still running, it doesn't handle that.
// don't re-use it (createChild twice).
// if you want to kill it using signalChild, please stick around and
// wait for processExited to be called before deleting it, because
// deleting it before that can cause an exception.
// it has to be deleted and reconstructed in order to use it again,
// because when the child exits the fds are closed and not reinit'd.
// also there is a race between getOpen and when the manager thread
// deregisters the descriptor. best to have a bool "done" in your
// derived class and check for that (like cpm_test does) rather than
// depending on getOpen.

class Handle {
    friend class Manager;
    int fromChildPipe[2];
    int toChildPipe[2];
    pid_t pid;
    bool open;
    bool inManager;
    int fds[2];
protected:
    Handle(void);
    virtual ~Handle(void);
public:
    commandVector cmd; // user initializes this please
    pid_t getPid(void) { if (!open) return -1; return pid; }
    bool getOpen(void) { return open; }
    bool createChild(void);
    size_t writeInput(const char *buffer, size_t len) {
        if (!open) return -1;
        return ::write(fds[1], buffer, len);
    }
    void signalChild(int sig) {
        if (!open) return;
        kill(pid, sig);
    }
    void closeChildInput(void) {
        if (!open) return;
        close(fds[1]);
        fds[1] = toChildPipe[1] = -1;
    }
    virtual void handleOutput(const char *buffer, size_t len) = 0;
    virtual void processExited(int status) = 0;
};

class Manager {
    static Manager * _instance;
    Manager(void);
    ~Manager(void);
    struct sigaction sigChildOact;
    static void sigChildHandler(int);
    typedef std::map<pid_t,Handle*> HandleMap;
    HandleMap openHandles;
    WaitUtil::Lockable handleLock;
    int signalFds[2];
    int rebuildFds[2];
    struct signalMsg {
        pid_t pid;
        int status;
    };
    pthread_t  notifyThreadId;
    static void * notifyThread(void *arg);
    void _notifyThread(void);
public:
    static Manager * instance(void);
    static void cleanup(void);
    // return false if failure
    bool createChild(Handle *handle);
    
};

}; /* namespace ChildProcessManager */

#endif /* __childprocessmanager_h__ */
