/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

// TODO: make library of commands to pass thru from children
//        - query current file name
//        - roll over file now
//        - close file
//        - open new file
//        - query names of all files currently in existence
// TODO: make command line interface to library calls

#include "options.h"
#include "logfile.h"
#include "bufprintf.h"
#include "pfkposix.h"
#include "libpfkscriptutil.h"

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

    if (opts.isRemoteCmd)
    {
        pfkscript_ctrl   ctrl;
        if (!ctrl.ok())
        {
            cerr << "unable to contact parent pfkscript\n";
            return 1;
        }
        bool zipping;
        string oldpath, newpath;
        if (opts.remoteCmd == "close")
        {
            ctrl.closeNow(oldpath, zipping);
            cout << "oldpath: " << oldpath << endl;
            cout << "zipping: " << (zipping ? "yes" : "no") << endl;
        }
        else if (opts.remoteCmd == "open") 
        {
            ctrl.openNow(newpath);
            cout << "newpath: " << newpath << endl;
        }
        else if (opts.remoteCmd == "rollover")
        {
            ctrl.rolloverNow(oldpath, zipping, newpath);
            cout << "oldpath: " << oldpath << endl;
            cout << "zipping: " << (zipping ? "yes" : "no") << endl;
            cout << "newpath: " << newpath << endl;
        }
        else if (opts.remoteCmd == "getfile")
        {
            ctrl.getFile(newpath);
            cout << "path: " << newpath << endl;
        }
        else
        {
            cerr << "unrecognized remote command\n";
            opts.printHelp();
            return 1;
        }

        return 0;
    }

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

    bool use_ctrl_sock = false;
    unix_dgram_socket control_sock;
    if (control_sock.ok())
    {
        use_ctrl_sock = true;
        setenv(pfkscript_ctrl::env_var_name,
               control_sock.getPath().c_str(), 1);
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        printError("fork");
        return 1;
    }
    if (pid == 0)
    {
        if (login_tty(slave_fd) < 0)
        {
            printError("login_tty");
            exit(98);
        }

        // close all fds i don't need. and i only
        // need 0, 1, 2. i really dont want to pass
        // any of my networking fds on to my children,
        // who knows what problems that may hide.
        int maxfd = sysconf(_SC_OPEN_MAX);
        for (int fd = 3; fd < maxfd; fd++)
            close(fd);

        char ppid[16];
        sprintf(ppid,"%d",getppid());
        setenv("IN_PFKSCRIPT", ppid, 1);

        execvp(opts.command[0], (char *const*)opts.command.data());

        printError("execvp");

        _exit(99);
    }

    // parent
    close(slave_fd);

    pfk_select  sel;
    bool done = false;
    char buffer[4096];
    int buflen;

    fcntl(master_fd, F_SETFL, 
          fcntl(master_fd, F_GETFL, 0) | O_NONBLOCK);

    struct termios  old_tios, new_tios;
    bool sttyWasRun = false;
    if (isatty(0) && !opts.backgroundSpecified && !opts.noReadSpecified)
    {
        if (tcgetattr(0, &old_tios) < 0)
        {
            char * errstring = strerror(errno);
            cerr << "failed to get termios: " << errstring << endl;
            return -1;
        }
        new_tios = old_tios;
//       raw    same as -ignbrk -brkint -ignpar -parmrk  -inpck  -istrip  -inlcr
//              -igncr  -icrnl  -ixon  -ixoff -icanon -opost -isig -iuclc -ixany
//              -imaxbel -xcase min 1 time 0
        new_tios.c_iflag &= ~(IGNBRK | BRKINT | IGNPAR | INPCK   | IXANY |
                              PARMRK | ISTRIP | INPCK  | INLCR   | IGNCR |
                              ICRNL  | IXON   | IXOFF  | IMAXBEL | IUCLC);
        new_tios.c_oflag &= ~(OPOST);
        new_tios.c_lflag &= ~(XCASE | ICANON | ISIG | ECHO);
        new_tios.c_cc[VMIN] = 1;
        new_tios.c_cc[VTIME] = 0;
        if (tcsetattr(0, TCSANOW, &new_tios) < 0)
        {
            char * errstring = strerror(errno);
            cerr << "failed to set termios: " << errstring << endl;
            return -1;
        }
        sttyWasRun = true;
    }

    // TODO forward window size changes to child PTY

    pfk_ticker   ticker;

    ticker.start(1,0);
    while (!done)
    {
        int cc;

        sel.rfds.zero();
        sel.rfds.set(master_fd);
        if (!opts.backgroundSpecified && !opts.noReadSpecified)
            sel.rfds.set(0);
        if (opts.listenPortSpecified)
            sel.rfds.set(listenPortFd);
        if (listenDataPortFd != -1)
            sel.rfds.set(listenDataPortFd);
        if (use_ctrl_sock)
            sel.rfds.set(control_sock.getFd());
        sel.rfds.set(ticker.fd());
        sel.tv.set(1,0);

        cc = sel.select();

        if (cc <= 0)
            continue;

        if (sel.rfds.isset(ticker.fd()))
        {
            char c;
            (void) read(ticker.fd(), &c, 1);
            logfile.periodic();
        }

        if (sel.rfds.isset(master_fd));
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
        if (sel.rfds.isset(0))
        {
            buflen = read(0, buffer, sizeof(buffer));
            if (buflen > 0)
            {
                (void) write(master_fd, buffer, buflen);
            }
        }
        if (use_ctrl_sock && sel.rfds.isset(control_sock.getFd()))
        {
            string buf, remote_path;
            if (control_sock.recv(buf, remote_path))
            {
                pfkscript_msg * msg = (pfkscript_msg *) buf.c_str();
                switch (msg->type)
                {
                case PFKSCRIPT_CMD_GET_FILE_PATH:
                {
                    msg->type = PFKSCRIPT_RESP_GET_FILE_PATH;
                    bool isOpen = logfile.isOpen();
                    int tail = 0;
                    if (isOpen)
                    {
                        tail = sizeof(msg->u.resp_get_file_path.path)-1;
                        strncpy(msg->u.resp_get_file_path.path,
                                logfile.getFilename().c_str(), tail);
                    }
                    msg->u.resp_get_file_path.path[tail] = 0;
                    control_sock.send(buf, remote_path);
                    break;
                }
                case PFKSCRIPT_CMD_ROLLOVER_NOW:
                {
                    msg->type = PFKSCRIPT_RESP_ROLLOVER_NOW;
                    int tail = 0;
                    if (logfile.isOpen())
                    {
                        tail = sizeof(msg->u.resp_rollover_now.oldpath)-1;
                        strncpy(msg->u.resp_rollover_now.oldpath,
                                logfile.getFilename().c_str(), tail);
                    }
                    msg->u.resp_rollover_now.oldpath[tail] = 0;
                    msg->u.resp_rollover_now.zipping = logfile.rolloverNow();
                    tail = 0;
                    if (logfile.isOpen())
                    {
                        tail = sizeof(msg->u.resp_rollover_now.newpath)-1;
                        strncpy(msg->u.resp_rollover_now.newpath,
                                logfile.getFilename().c_str(), tail);
                    }
                    msg->u.resp_rollover_now.newpath[tail] = 0;
                    control_sock.send(buf, remote_path);
                    break;
                }
                case PFKSCRIPT_CMD_CLOSE_NOW:
                {
                    msg->type = PFKSCRIPT_RESP_CLOSE_NOW;
                    int tail = 0;
                    if (logfile.isOpen())
                    {
                        tail = sizeof(msg->u.resp_rollover_now.oldpath)-1;
                        strncpy(msg->u.resp_rollover_now.oldpath,
                                logfile.getFilename().c_str(), tail);
                    }
                    msg->u.resp_rollover_now.oldpath[tail] = 0;
                    msg->u.resp_rollover_now.zipping = logfile.closeNow();
                    msg->u.resp_rollover_now.newpath[0] = 0;
                    control_sock.send(buf, remote_path);
                    break;
                }
                case PFKSCRIPT_CMD_OPEN_NOW:
                {
                    msg->type = PFKSCRIPT_RESP_OPEN_NOW;
                    logfile.openNow();
                    int tail = 0;
                    if (logfile.isOpen())
                    {
                        tail = sizeof(msg->u.resp_get_file_path.path)-1;
                        strncpy(msg->u.resp_get_file_path.path,
                                logfile.getFilename().c_str(), tail);
                    }
                    msg->u.resp_get_file_path.path[tail] = 0;
                    control_sock.send(buf, remote_path);
                    break;
                }
                default:
                    cerr << "unknown msg type " << msg->type
                         << " received on ctrl sock" << endl;
                }
            }
        }
        if (listenPortFd > 0 && sel.rfds.isset(listenPortFd))
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
                    fclose(fd); // also closes listenDataPortFd
                }
                listenDataPortFd = newfd;
            }
        }
        if (listenDataPortFd > 0 && sel.rfds.isset(listenDataPortFd))
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
    ticker.stopjoin();

    if (listenDataPortFd > 0)
        close(listenDataPortFd);
    if (listenPortFd > 0)
        close(listenPortFd);
    if (opts.backgroundSpecified)
        unlink(opts.pidFile.c_str());

    if (sttyWasRun)
    {
        if (tcsetattr(0, TCSANOW, &old_tios) < 0)
        {
            char * errstring = strerror(errno);
            cerr << "failed to set termios: " << errstring << endl;
            return -1;
        }
    }

    return 0;
}
