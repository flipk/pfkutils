/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

//
// TODO: create command port, pass to children via env var
// TODO: make library of commands to pass thru from children
//        - roll over file now
//        - close file
//        - open new file
//        - query current file name
//        - query names of all files currently in existence
// TODO: make command line interface to library calls
// 

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
#include <sys/socket.h>
#include <arpa/inet.h>

#include "ticker.h"

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

    int listenPortFd = -1;
    int listenDataPortFd = -1;

    if (opts.listenPortSpecified)
    {
        listenPortFd = socket(AF_INET, SOCK_STREAM, 0);
        int v = 1;
        setsockopt( listenPortFd, SOL_SOCKET, SO_REUSEADDR,
                    (void*) &v, sizeof( v ));
        struct sockaddr_in sa;
        sa.sin_family = AF_INET;
        sa.sin_port = htons(opts.listenPort);
        sa.sin_addr.s_addr = INADDR_ANY;
        if (bind(listenPortFd, (struct sockaddr *)&sa, sizeof(sa)) < 0)
        {
            int e = errno;
            cerr << "unable to bind to port " << opts.listenPort
                 << ": " << strerror(e) << endl;
            return 1;
        }
        listen(listenPortFd,1);
    }

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

    bool sttyWasRun = false;
    // gross but effective
    if (isatty(0) && !opts.backgroundSpecified && !opts.noReadSpecified)
    {
        system("stty raw -echo");
        sttyWasRun = true;
    }

    // TODO forward window size changes to child PTY

    Ticker   ticker;
    int ticker_fd = ticker.get_fd();

    ticker.start();
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
        if (opts.listenPortSpecified)
        {
            FD_SET(listenPortFd, &rfds);
            if (listenPortFd > maxfd)
                maxfd = listenPortFd;
        }
        if (listenDataPortFd != -1)
        {
            FD_SET(listenDataPortFd, &rfds);
            if (listenDataPortFd > maxfd)
                maxfd = listenDataPortFd;
        }
        FD_SET(ticker_fd, &rfds);
        if (ticker_fd > maxfd)
            maxfd = ticker_fd;

        tv.tv_sec = 1;
        tv.tv_usec = 0;
        cc = select(maxfd+1, &rfds, NULL, NULL, &tv);

        if (cc <= 0)
            continue;

        if (FD_ISSET(ticker_fd, &rfds))
        {
            char c;
            (void) read(ticker_fd, &c, 1);
            logfile.periodic();
        }

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
                if (listenDataPortFd != -1)
                    (void) write(listenDataPortFd, buffer, buflen);
            }
        }
        if (FD_ISSET(0, &rfds))
        {
            buflen = read(0, buffer, sizeof(buffer));
            if (buflen > 0)
            {
                (void) write(master_fd, buffer, buflen);
            }
        }
        if (listenPortFd > 0 && FD_ISSET(listenPortFd, &rfds))
        {
            struct sockaddr_in sa;
            socklen_t len = sizeof(sa);
            int newfd = accept(listenPortFd, (struct sockaddr *)&sa,
                               &len);
            if (newfd > 0)
            {
                if (listenDataPortFd != -1)
                {
                    FILE * fd = fdopen(listenDataPortFd, "w");
                    fprintf(fd,
                            "\r\n\r\n\r\n"
                            " *** remote port takeover ***"
                            "\r\n\r\n\r\n");
                    fclose(fd);
                }
                listenDataPortFd = newfd;
            }
        }
        if (listenDataPortFd != -1 && FD_ISSET(listenDataPortFd, &rfds))
        {
            // discard data, just so the fd doesn't get stuffed up;
            // also, detect remote closures.
            buflen = read(listenDataPortFd, buffer, sizeof(buffer));
            if (buflen <= 0)
            {
                close(listenDataPortFd);
                listenDataPortFd = -1;
            }
        }
    }
    ticker.stop();

    if (opts.backgroundSpecified)
        unlink(opts.pidFile.c_str());

    // gross but effective
    if (sttyWasRun)
        system("stty echo cooked");

    return 0;
}
