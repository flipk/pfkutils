#if 0
set -e -x
g++ -g3 -DCHILDPROCESSMANAGERTESTMAIN childprocessmanager.cc LockWait.cc -o cpm -lpthread
exit 0
#endif

/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#include "childprocessmanager.h"
#include "bufprintf.h"

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <pthread.h>

#include <iostream>

using namespace std;

/************************ ChildProcessHandle ******************************/

ChildProcessHandle :: ChildProcessHandle(void)
{
    pipe(fromChildPipe);
    pipe(toChildPipe);
    open = false;
    fds[0] = fromChildPipe[0];
    fds[1] = toChildPipe[1];
}

ChildProcessHandle :: ~ChildProcessHandle(void)
{
    open = false;
#define CLOSEFD(fd) if (fd != -1) { close(fd); fd = -1; }
    CLOSEFD(fromChildPipe[0]);
    CLOSEFD(fromChildPipe[1]);
    CLOSEFD(toChildPipe[0]);
    CLOSEFD(toChildPipe[1]);
}

bool
ChildProcessHandle :: createChild(void)
{
    return ChildProcessManager::instance()->createChild(this);
}

/************************ ChildProcessManager ******************************/

ChildProcessManager * ChildProcessManager::_instance = NULL;

ChildProcessManager * ChildProcessManager::instance(void)
{
    if (_instance == NULL)
        _instance = new ChildProcessManager();
    return _instance;
}

void
ChildProcessManager :: cleanup(void)
{
    if (_instance != NULL)
    {
        delete _instance;
        _instance = NULL;
    }
}

ChildProcessManager :: ChildProcessManager(void)
{
    struct sigaction sa;
    sa.sa_handler = &ChildProcessManager::sigChildHandler;
    sigfillset(&sa.sa_mask);
    sa.sa_flags = SA_NOCLDSTOP | SA_RESTART;
    sigaction(SIGCHLD, &sa, &sigChildOact);
    pipe(signalFds);
    pipe(rebuildFds);
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&notifyThreadId, &attr,
                   &ChildProcessManager::notifyThread, (void*) this);
    pthread_attr_destroy(&attr);
}

ChildProcessManager :: ~ChildProcessManager(void)
{
    char dummy = 2; // code 2 means die
    write(rebuildFds[1], &dummy, 1);
    sigaction(SIGCHLD, &sigChildOact, NULL);
    close(signalFds[0]);
    close(signalFds[1]);
    close(rebuildFds[0]);
    close(rebuildFds[1]);
}

bool
ChildProcessManager :: createChild(ChildProcessHandle *handle)
{
    int forkErrorPipe[2];
    pipe(forkErrorPipe);

    // since we're forking for the purpose of
    // only exec, use vfork instead of fork.
    // it is more efficient. there are restrictions
    // to what the child can do (before exec) though.
    handle->pid = vfork();
    if (handle->pid < 0)
    {
        int e = errno;
        cerr << "fork: " << e << ": " << strerror(errno) << endl;
        return false;
    }

    if (handle->pid == 0)
    {
        // child

        // move pipe ends we do use to the
        // fd numbers where they're used by
        // the child.

        dup2(handle->toChildPipe[0], 0);
        dup2(handle->fromChildPipe[1], 1);
        dup2(handle->fromChildPipe[1], 2);
        dup2(forkErrorPipe[1], 3);

        // don't allow the child to inhert any
        // "interesting" file descriptors.
        for (int i = 3; i < sysconf(_SC_OPEN_MAX); i++)
            if (i != forkErrorPipe[1])
                close(i);

        // mark the 'fork error' pipe as close-on-exec,
        // so if the exec succeeds, the parent gets a zero
        // read, but if it fails, it gets a 1-read.
        fcntl(forkErrorPipe[1], F_SETFD, FD_CLOEXEC);

        execvp(handle->cmd[0], (char *const*)handle->cmd.data());

        // dont print stuff! we're vforked, that can screw
        // up the parent process' address space.

        // send the errno to parent.
        int e = errno;
        write(forkErrorPipe[1], &e, sizeof(int));

        // call _exit because it isn't correct for a vforked
        // child to call the atexit handlers, that can screw
        // up crap in the parent.
        _exit(99);
    }
    //parent

    // close pipe ends we don't use.
    CLOSEFD(handle->fromChildPipe[1]);
    CLOSEFD(handle->toChildPipe[0]);
    close(forkErrorPipe[1]);

    bool ret = false;
    int e;
    int cc = read(forkErrorPipe[0], &e, sizeof(e));
    close(forkErrorPipe[0]);
    if (cc == 0)
    {
        // zero read means the pipe was closed-on-exec,
        // so child success.
        handle->open = true;
        {
            WaitUtil::Lock key(&handleLock);
            openHandles[handle->pid] = handle;
        } // key destroyed here
        // wake up notify thread so it knows to
        // rebuild its fd_sets to include the 
        // new handle.
        char dummy = 1; // code 1 means rebuild
        (void) write(rebuildFds[1], &dummy, 1);
        ret = true;
    }
    else
    {
        // nonzero read means exec failed in the child.
        cerr << "execvp " << handle->cmd[0]
             << " failed with error " << e << ": "
             << strerror(e) << endl;
    }

    return ret;
}

//static
void
ChildProcessManager :: sigChildHandler(int s)
{
    struct signalMsg msg;
    do {
        msg.pid = waitpid(/*wait for any child*/-1,
                          &msg.status, WNOHANG);

        if (msg.pid > 0)
        {
            if (0) // debug
            {
                Bufprintf<80>  bufp;
                bufp.print("sig handler got pid %d died, status %d\n",
                           msg.pid, msg.status);
                bufp.write(1);
            }
            if (_instance != NULL)
                // dont do any data structure manipulations here;
                // it is difficult to mutex between threads and 
                // signal handlers. (it can be done with sigprocmask but
                // that is expensive.) instead send a message to the
                // notify thread and let it deal with it in thread
                // context.
                (void) write(_instance->signalFds[1], &msg, sizeof(msg));
        }

    } while (msg.pid > 0);
}

//static
void *
ChildProcessManager :: notifyThread(void *arg)
{
    ChildProcessManager * mgr = (ChildProcessManager *) arg;
    mgr->_notifyThread();
    return NULL;
}

void
ChildProcessManager :: _notifyThread(void)
{
    bool done = false;
    std::vector<ChildProcessHandle*> handles;
    char buffer[4096];
    int cc;
    fd_set rfds;
    int maxfd;
    HandleMap::iterator it;
    ChildProcessHandle * h;
    struct signalMsg msg;
    char dummy;

    while (!done)
    {
        FD_ZERO(&rfds);
        maxfd = signalFds[0];
        FD_SET(signalFds[0], &rfds);
        if (maxfd < rebuildFds[0])
            maxfd = rebuildFds[0];
        FD_SET(rebuildFds[0], &rfds);

        { // add fds for all open handles to the fd_set.
            WaitUtil::Lock key(&handleLock);
            for (it = openHandles.begin();
                 it != openHandles.end();
                 it++)
            {
                h = it->second;
                if (h->open)
                {
                    if (maxfd < h->fds[0])
                        maxfd = h->fds[0];
                    FD_SET(h->fds[0], &rfds);
                }
            }
        } // key destroyed here

        select(maxfd+1, &rfds, NULL, NULL, NULL);

        if (FD_ISSET(rebuildFds[0], &rfds))
        {
            if (read(rebuildFds[0], &dummy, 1) != 1)
                done = true;
            else
                if (dummy == 2) // die code
                    // must be the manager destructor
                    done = true;

            if (0) // debug
            {
                Bufprintf<80> bufp;
                bufp.print("thread awakened on rebuild fd, done = %d\n",
                           done);
                bufp.write(1);
            }
            // otherwise this is just meant to make sure we rebuild
            // our fd_sets because the openHandles map has changed.
        }
        if (!done && FD_ISSET(signalFds[0], &rfds))
        {
            // the sigchld handler has sent us a pid and status.
            cc = read(signalFds[0], &msg, sizeof(msg));
            if (cc <= 0)
                done = true;
            else
            {
                if (0) // debug
                {
                    Bufprintf<80> bufp;
                    bufp.print("got msg pid %d died status %d\n",
                               msg.pid, msg.status);
                    bufp.write(1);
                }
                WaitUtil::Lock key(&handleLock);
                it = openHandles.find(msg.pid);
                if (it != openHandles.end())
                {
                    h = it->second;
                    h->processExited(msg.status);
                    h->open = false;
                    openHandles.erase(it);
                }
            } // key destroyed here
        }
        if (!done)
        { // check all open handles
            // dont hold the key while calling h->handleOutput.
            // build the list of all handles that need servicing
            // first, then release the key.
            {
                WaitUtil::Lock key(&handleLock);
                for (it = openHandles.begin();
                     it != openHandles.end();
                     it++)
                {
                    h = it->second;
                    if (FD_ISSET(h->fds[0], &rfds))
                        handles.push_back(h);
                }
            } // key destroyed here
            for (int ind = 0; ind < handles.size(); ind++)
            {
                h = handles[ind];
                cc = read(h->fds[0], buffer, sizeof(buffer));
                if (cc > 0)
                    // call user's virtual method to handle the data.
                    h->handleOutput(buffer, cc);
            }
            handles.clear();
        }
    }
}

#ifdef CHILDPROCESSMANAGERTESTMAIN

class TestHandle : public ChildProcessHandle {
    int inst;
public:
    TestHandle(int _inst) : inst(_inst) {
        if (0) // debug
        {
            Bufprintf<80> bufp;
            bufp.print("constructing testhandle inst %d\n", inst);
            bufp.write(1);
        }
        if (inst == 1)
            cmd.push_back("cat");
        else
            cmd.push_back("sh");
        cmd.push_back(NULL);
    }
    virtual ~TestHandle(void) {
        if (0) // debug
        {
            Bufprintf<80> bufp;
            bufp.print("~TestHandle destructor inst %d\n", inst);
            bufp.write(1);
        }
    }
    /*virtual*/ void handleOutput(const char *buffer, size_t len) {
        cout << "data from inst " << inst << endl;
        write(1, buffer, len);
    }
    /*virtual*/ void processExited(int status) {
        cout << "processExited called for inst " << inst << endl;
    }
};

int
main()
{

    // sigpipe is a fucking douchefucker. fuck him.
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;//SA_RESTART;
    sigaction(SIGPIPE, &sa, NULL);

    cout << "my pid is " << getpid() << endl;
    ChildProcessManager::instance();
    {
        TestHandle hdl1(1);
        TestHandle hdl2(2);
        hdl1.createChild();
        hdl2.createChild();
        char buf[256];
        int cc;

        while (hdl1.getOpen() || hdl2.getOpen())
        {
            cc = read(0, buf, sizeof(buf));
            if  (cc == 0)
                break;
            hdl1.writeInput(buf, cc);
            hdl2.writeInput(buf, cc);
        }
        if (hdl1.getOpen())
            hdl1.closeChildInput();
        if (hdl2.getOpen())
            hdl2.closeChildInput();
        while (hdl1.getOpen() && hdl2.getOpen())
            usleep(1);
    }
    ChildProcessManager::cleanup();
    return 0;
}

#endif
