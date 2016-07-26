/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#include "options.h"
#include "logfile.h"
#include "bufprintf.h"

#include <stdio.h>
#include <pty.h>
#include <unistd.h>
#include <utmp.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>

#include <iostream>
#include <fstream>

using namespace std;

extern "C" int
pfkscript_main(int argc, char ** argv)
{
    Options opts(argc-1,argv+1);

    if (opts.isError)
    {
        opts.printHelp();
        return 1;
    }

    if (0) // debug
        opts.printOptions();

    LogFile   logfile(opts);

    if (logfile.isError)
        // assume LogFile already printed something informative
        return 1;

    int master_fd, slave_fd;
    char ttyname[256];

    if (openpty(&master_fd, &slave_fd, ttyname,
                NULL, NULL) < 0)
    {
        printError("openpty");
        return 1;
    }

    // sigpipe is a fucking douchefucker. fuck him.
    {
        struct sigaction sa;
        sa.sa_handler = SIG_IGN;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sigaction(SIGPIPE, &sa, NULL);
    }

    if (opts.backgroundSpecified)
    {
        if (daemon(1,0) < 0)
            printf("warning: unable to daemonize\n");
        // when daemon returns, we are the new child
        ofstream outfile(opts.pidFile.c_str(),
                         ios::out | ios::trunc);
        int pid = getpid();
        outfile << pid << endl;
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        printError("fork");
        return 1;
    }
    if (pid == 0)
    {
        // child
        close(master_fd);
        if (login_tty(slave_fd) < 0)
        {
            printError("login_tty");
            exit(98);
        }

        char ppid[16];
        sprintf(ppid,"%d",getppid());
        setenv("IN_PFKSCRIPT", ppid, 1);

        execvp(opts.command[0], (char *const*)opts.command.data());

        printError("execvp");

        _exit(99);
    }

    // parent
    close(slave_fd);

    fd_set rfds;
    bool done = false;
    char buffer[4096];
    int buflen;

    fcntl(master_fd, F_SETFL, 
          fcntl(master_fd, F_GETFL, 0) | O_NONBLOCK);

    // gross but effective
    if (!opts.backgroundSpecified && !opts.noReadSpecified)
        system("stty raw -echo");

    // TODO forward window size changes to child PTY

    while (!done)
    {
        struct timeval tv;
        int cc;
        int maxfd;

        FD_ZERO(&rfds);
        maxfd = master_fd;
        FD_SET(master_fd, &rfds);
        if (!opts.backgroundSpecified && !opts.noReadSpecified)
            FD_SET(0, &rfds);

        tv.tv_sec = 1;
        tv.tv_usec = 0;
        cc = select(maxfd+1, &rfds, NULL, NULL, &tv);
    
        if (cc == 0)
            logfile.periodic();

        if (FD_ISSET(master_fd, &rfds))
        {
            buflen = read(master_fd, buffer, sizeof(buffer));
            if (0) // debug
            {
                int e = errno;
                Bufprintf<80> prt;
                prt.print("read on fd %d returned %d errno %d\n",
                          master_fd, buflen, e);
                prt.write(1);
                errno = e;
            }
            if (buflen == 0)
            {
                cout << "zero read from master fd\n";
                done = true;
            }
            else if (buflen < 0)
            {
                // apparently EIO is normal, how a pty
                // closes! wat
                if (errno != EIO && errno != EAGAIN)
                    printError("read");
                // EAGAIN happens sometimes when the child process
                //  dies and causes SIGCHLD
                if (errno != EAGAIN)
                    done = true;
            }
            else
            {
                logfile.addData(buffer,buflen);
                if (!opts.backgroundSpecified && !opts.noOutputSpecified)
                    (void) write(1, buffer, buflen);
            }
        }
        if (FD_ISSET(0, &rfds))
        {
            buflen = read(0, buffer, sizeof(buffer));
// only log what's output, for now;
// in 99% of the cases, what's input is echoed back on output,
// so we dont want that output occuring in the log file twice.
//            if (buflen > 0)
//                logfile.addData(buffer,buflen);
            if (buflen > 0)
                (void) write(master_fd, buffer, buflen);
        }
    }

    if (opts.backgroundSpecified)
        unlink(opts.pidFile.c_str());

    // gross but effective
    if (!opts.backgroundSpecified && !opts.noReadSpecified)
        system("stty cooked");

    return 0;
}
