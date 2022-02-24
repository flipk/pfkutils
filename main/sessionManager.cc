#if 0
set -e -x
g++ -DpfkSessionMgr_main=main sessionManager.cc -o pfkSessionMgr
exit 0
#endif
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

#include <iostream>
#include <sstream>
#include <string>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <inttypes.h>
#include <X11/Xlib.h>

#include "sessionManager.h"

using namespace std;

void
usage(void)
{
    cout << 
"pfkSessionMgr -s 'child 1 command string' 'child 2 command string' [etc]\n"
"pfkSessionMgr [-p pid] -c [stop | restart]\n"
        ;
}

struct Command {
    string cmd;
    vector<const char *> args;
    pid_t startedPid;
    pid_t pid;
    int status;
    Command(const string &_cmd);
    void start(void);
    void kill(void);
};

typedef vector<Command*> CommandList;

static bool startProcesses(void);
static bool openDisplay(void);

static CommandList commands;

extern "C" int
pfkSessionMgr_main(int argc, char ** argv)
{
    pid_t pid = -1;
    enum { OP_NONE, OP_BAD, OP_START, OP_STOP, OP_RESTART } op = OP_NONE;

    char * pidVar = getenv(PFK_SESS_MGR_ENV_VAR_NAME);
    if (pidVar != NULL)
        pid = strtol(pidVar, NULL, 10);

    int argCtr = 1;
    while (argCtr < argc && op != OP_BAD)
    {
        string arg(argv[argCtr++]);
        if (arg == "-p")
        {
            if (argCtr == argc)
                op = OP_BAD;
            else
                pid = strtol(argv[argCtr++], NULL, 10);
        }
        else if (arg == "-s")
        {
            if (op != OP_NONE || argCtr == argc)
                op = OP_BAD;
            else
            {
                op = OP_START;
                while (argCtr != argc)
                    commands.push_back(new Command(string(argv[argCtr++])));
            }
        }
        else if (arg == "-c")
        {
            if (op != OP_NONE || argCtr == argc)
                op = OP_BAD;
            else
            {
                string cmd(argv[argCtr++]);
                if (cmd == "stop")
                    op = OP_STOP;
                else if (cmd == "restart")
                    op = OP_RESTART;
                else
                    op = OP_BAD;
            }
        }
    }

    switch (op)
    {
    case OP_START:
        if (pid != -1)
            op = OP_BAD;
        break;
    case OP_STOP:
    case OP_RESTART:
        if (pid == -1)
            op = OP_BAD;
        break;
    default:
        ;//no compiler warning
    }

    {
        ostringstream setEnv;
        setEnv << getpid();
        setenv(PFK_SESS_MGR_ENV_VAR_NAME,
               setEnv.str().c_str(), 1);
    }

    switch (op)
    {
    case OP_START:
        if (startProcesses() == false)
            return 1;
        break;
    case OP_STOP:
        kill(pid, PFK_SESS_MGR_STOP_SIG);
        break;
    case OP_RESTART:
        kill(pid, PFK_SESS_MGR_RESTART_SIG);
        break;
    default:
        usage();
        return 1;
    }

    return 0;
}

bool doStop = false;
bool doRestart = false;

static void sighand(int s)
{
    pid_t pid = -1;
    int status;
    switch (s)
    {
    case PFK_SESS_MGR_STOP_SIG:
        doStop = true;
        break;
    case PFK_SESS_MGR_RESTART_SIG:
        doRestart = true;
        break;
    case SIGCHLD:
        do {
            pid = waitpid(/*wait for any child*/-1, &status, WNOHANG);
            if (pid > 0)
            {
                for (uint32_t ind = 0; ind < commands.size(); ind++)
                {
                    Command * cmd = commands[ind];
                    if (cmd->pid == pid)
                    {
                        cmd->pid = -1;
                        cmd->status = status;
                        break;
                    }
                }
            }
        } while (pid > 0);
        break;
    }
}

Command :: Command(const string &_cmd)
    : cmd("exec "),
      startedPid(-1),
      pid(-1),
      status(-1)
{
    cmd += _cmd;
    char * shell = getenv("SHELL");
    args.push_back(shell);
    args.push_back("-c");
    args.push_back(cmd.c_str());
    args.push_back(NULL);
}

void
Command :: start(void)
{
    startedPid = pid = vfork();
    if (pid < 0)
    {
        int e = errno;
        cerr << "fork: " << e << ": " << strerror(errno) << endl;
        return;
    }
    if (pid == 0)
    {
        // child

        // don't allow the child to inhert any
        // "interesting" file descriptors.
        int maxfd = sysconf(_SC_OPEN_MAX);
        for (int i = 3; i < maxfd; i++)
            close(i);

        execvp(args[0], (char *const*)args.data());

        cout << "FAIL TO EXEC error " << errno << endl;

        // call _exit because it isn't correct for a vforked
        // child to call the atexit handlers, that can screw
        // up crap in the parent.
        _exit(99);
    }
    //parent
    cout << "started pid " << pid << ": " << cmd << endl;
}

void
Command :: kill(void)
{
    int count = 0;
    if (pid <= 0)
        return;
    while (pid > 0)
    {
        if (count == 0)
        {
            cout << "sending SIGTERM to pid " << pid << endl;
            ::kill(pid, SIGTERM);
        }
        if (count == 20) // 2 seconds
        {
            cout << "sending SIGKILL to pid " << pid << endl;
            ::kill(pid, SIGKILL);
        }
        if (count == 30) // 3 seconds
        {
            cout << "giving up on pid " << pid << endl;
            break; // give up
        }
        // if the child dies while we're waiting, the SIGCHLD
        // will actually interrupt and shorten this
        // sleep, so we won't actually wait the whole
        // 0.1 seconds. the response will be nearly instant.
        usleep( 100000 );
        count++;
    }
    cout << "pid " << startedPid << " wait status " << status << endl;
}

static bool
startProcesses(void)
{
    struct sigaction sa;
    sa.sa_handler = &sighand;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    sigaction(PFK_SESS_MGR_STOP_SIG,    &sa, NULL);
    sigaction(PFK_SESS_MGR_RESTART_SIG, &sa, NULL);
    sigaction(SIGCHLD, &sa, NULL);

    if (openDisplay() == false)
        return false;

    uint32_t ind;
    while (doStop == false)
    {
        for (ind = 0; ind < commands.size(); ind++)
            commands[ind]->start();

        doRestart = false;
        while (doStop == false && doRestart == false)
        {
            for (ind = 0; ind < commands.size(); ind++)
            {
                Command * cmd = commands[ind];
                if (cmd->pid == -1)
                {
                    cout << "pid " << cmd->startedPid << " died!\n";
                    // it died! restart
                    cmd->start();
                }
            }
            // a 5 second poll interval may seem like a long
            // time to wait to discover a process has died; but the
            // neat thing about this sleep is a SIGCHLD
            // or SIGUSRx will interrupt it, causing near
            // immediate response to the signal.
            sleep(5);
        }

        for (ind = 0; ind < commands.size(); ind++)
            commands[ind]->kill();
    }

    return true;
}

static pthread_t  displayMonitorThread;

static void * displayMonitorThreadMain( void * arg );

static bool
openDisplay(void)
{
    char * displayVar = getenv("DISPLAY");

    if (displayVar == NULL)
    {
        printf("DISPLAY variable not set\n");
        return false;
    }

    Display * d = XOpenDisplay(displayVar);
    if (d == NULL)
    {
        printf("unable to open DISPLAY\n");
        return false;
    }

    pthread_create(&displayMonitorThread, /*attr*/NULL,
                   &displayMonitorThreadMain, (void*) d);

    return true;
}

static int xerrorhandler(Display *d, XErrorEvent *evt)
{
    // this will probably never fire because we didn't
    // register for any events and we created no windows.
    printf("got error handler type %d\n", evt->type);
    return 0;
}

static int xioerrorhandler(Display *d)
{
    // this will fire if the server dies.
    printf("pfkSessionMgr : got IO error, killing all children\n");
    doStop = true;
    sleep(10); // give pids time to die
    exit(1);
}

static void *
displayMonitorThreadMain( void * arg )
{
    Display * d = (Display *) arg;

    XSetErrorHandler(&xerrorhandler);
    XSetIOErrorHandler(&xioerrorhandler);

    while (1)
    {
        XEvent  evt;
        XNextEvent(d, &evt);
        printf("got X event type %d\n", evt.type);
    }

    return NULL;
}
