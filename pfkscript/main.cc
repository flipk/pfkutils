/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */
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

#include "options.h"
#include "logfile.h"
#include "bufprintf.h"
#include "posix_fe.h"
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
#include <fstream>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <sys/wait.h>

using namespace std;

class Pfkscript_program {
    static Pfkscript_program *instance;
    const Options opts;
    LogFile logfile;
    int listenPortFd;
    int listenDataPortFd;
    bool use_ctrl_sock;
    pxfe_timeval  start_time;
    pxfe_timeval  stop_time;
    pxfe_unix_dgram_socket control_sock;
    struct termios old_tios;
    bool sttyWasRun;
    pxfe_ticker ticker;
    pxfe_pipe winch_pipe;
    int master_fd;
    bool wait_status_collected;
    int wait_status;
    pid_t child_pid;
    struct remoteResponseInfo {
        PfkscriptMsg   msg;
        string  remote_path;
        remoteResponseInfo(const PfkscriptMsg &_msg, const string &_remote_path)
            : msg(_msg), remote_path(_remote_path) { }
        remoteResponseInfo(void) { }
    };
    typedef map<string/*zippingFile*/,remoteResponseInfo> remotePending_t;
    remotePending_t remoteResponsePending;

    int do_remote_cmd(void)
    {
        pfkscript_ctrl   ctrl;
        if (!ctrl.ok())
        {
            cerr << "unable to contact parent pfkscript\n";
            return 1;
        }
        bool zipped;
        string oldpath, newpath;
        if (opts.remoteCmd == "close")
        {
            ctrl.closeNow(oldpath, zipped);
            cout << "oldpath: " << oldpath << endl;
            cout << "zipped: " << (zipped ? "yes" : "no") << endl;
        }
        else if (opts.remoteCmd == "open") 
        {
            ctrl.openNow(newpath);
            cout << "newpath: " << newpath << endl;
        }
        else if (opts.remoteCmd == "rollover")
        {
            ctrl.rolloverNow(oldpath, zipped, newpath);
            cout << "oldpath: " << oldpath << endl;
            cout << "zipped: " << (zipped ? "yes" : "no") << endl;
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

    bool makeListenPort(void)
    {
        listenPortFd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
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
            return false;
        }
        listen(listenPortFd,1);
        return true;
    }

    void block_sigpipe(void)
    {
        // sigpipe is a fucking douchefucker. fuck him.
        struct sigaction sa;
        sa.sa_handler = SIG_IGN;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sigaction(SIGPIPE, &sa, NULL);
    }

    static void sigchild_handler(int s)
    {
        if (instance)
        {
            wait(&instance->wait_status);
            instance->wait_status_collected = true;
        }
        else
        {
            int dummy;
            wait(&dummy);
            printf("\r\n\r\nCHILD PROCESS EXITED WITH STATUS: %d\r\n\r\n",
                   dummy);
        }
    }

    void setup_sigchild(void)
    {
        struct sigaction sa;
        sa.sa_handler = &sigchild_handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sigaction(SIGCHLD, &sa, NULL);
    }

    void daemonize(void)
    {
        if (daemon(1,0) < 0)
            printf("warning: unable to daemonize\n");
        // when daemon returns, we are the new child
        ofstream outfile(opts.pidFile.c_str(),
                         ios::out | ios::trunc);
        int pid = getpid();
        outfile << pid << endl;
    }

    bool setRawMode(void)
    {
        struct termios new_tios;
        if (tcgetattr(0, &old_tios) < 0)
        {
            char * errstring = strerror(errno);
            cerr << "failed to get termios: " << errstring << endl;
            return false;
        }
        new_tios = old_tios;
//       raw    same as -ignbrk -brkint -ignpar -parmrk  -inpck  -istrip  -inlcr
//              -igncr  -icrnl  -ixon  -ixoff -icanon -opost -isig -iuclc -ixany
//              -imaxbel -xcase min 1 time 0
        new_tios.c_iflag &= ~(IGNBRK | BRKINT | IGNPAR | INPCK   | IXANY |
                              PARMRK | ISTRIP | INPCK  | INLCR   | IGNCR |
                              ICRNL  | IXON   | IXOFF  | IMAXBEL | IUCLC);
        new_tios.c_oflag &= ~(OPOST);
#ifndef XCASE
#define XCASE 0 // cygwin doesnt define this.
#endif
        new_tios.c_lflag &= ~(XCASE | ICANON | ISIG | ECHO);
        new_tios.c_cc[VMIN] = 1;
        new_tios.c_cc[VTIME] = 0;
        if (tcsetattr(0, TCSANOW, &new_tios) < 0)
        {
            char * errstring = strerror(errno);
            cerr << "failed to set termios: " << errstring << endl;
            return false;
        }
        return true;
    }

    void restore_cooked_mode(void)
    {
        if (tcsetattr(0, TCSANOW, &old_tios) < 0)
        {
            char * errstring = strerror(errno);
            cerr << "failed to set termios: " << errstring << endl;
        }
    }

    // return true when done
    bool handle_master_sock(void)
    {
        bool done = false;
        char buffer[4096];
        int buflen;

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
                printError(errno, "read");
            // EAGAIN happens sometimes when the child process
            //  dies and causes SIGCHLD
            if (errno != EAGAIN)
                done = true;
        }
        else
        {
            logfile.addData(buffer,buflen);
            if (!opts.backgroundSpecified && !opts.noOutputSpecified)
                if (write(1, buffer, buflen) < 0)
                    fprintf(stderr, "handle_master_sock: write 1 failed\n");
            if (listenDataPortFd != -1)
                if (write(listenDataPortFd, buffer, buflen) < 0)
                    fprintf(stderr, "handle_master_sock: write 2 failed\n");
        }

        return done;
    }

    void handle_fd0(void)
    {
        char buffer[4096];
        int buflen;
        buflen = read(0, buffer, sizeof(buffer));
        if (buflen > 0)
        {
            if (write(master_fd, buffer, buflen) < 0)
                fprintf(stderr, "handle_fd0: write failed\n");
        }
    }

    void handle_listen_port(void)
    {
        struct sockaddr_in sa;
        socklen_t len = sizeof(sa);
        int newfd = accept4(listenPortFd, (struct sockaddr *)&sa,
                           &len, SOCK_CLOEXEC);
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

    void handle_listen_data_port(void)
    {
        char buffer[4096];
        int buflen;
        // discard data, just so the fd doesn't get stuffed up;
        // also, detect remote closures.
        buflen = read(listenDataPortFd, buffer, sizeof(buffer));
        if (buflen <= 0)
        {
            close(listenDataPortFd);
            listenDataPortFd = -1;
        }
    }

    void handle_control_sock(void)
    {
        PfkscriptMsg   msg;
        string  remote_path;
        if (control_sock.recv(msg.buf, remote_path))
        {
            switch (msg.m().type)
            {
            case PFKSCRIPT_CMD_GET_FILE_PATH:
            {
                msg.m().type = PFKSCRIPT_RESP_GET_FILE_PATH;
                bool isOpen = logfile.isOpen();
                int tail = 0;
                if (isOpen)
                {
                    tail = sizeof(msg.m().u.resp_get_file_path.path)-1;
                    strncpy(msg.m().u.resp_get_file_path.path,
                            logfile.getFilename().c_str(), tail);
                }
                msg.m().u.resp_get_file_path.path[tail] = 0;
                control_sock.send(msg.buf, remote_path);
                break;
            }
            case PFKSCRIPT_CMD_ROLLOVER_NOW:
            {
                msg.m().type = PFKSCRIPT_RESP_ROLLOVER_NOW;
                int tail = 0;
                string filename;
                if (logfile.isOpen())
                {
                    tail = sizeof(msg.m().u.resp_rollover_now.oldpath)-1;
                    filename = logfile.getFilename();
                    strncpy(msg.m().u.resp_rollover_now.oldpath,
                            filename.c_str(), tail);
                }
                msg.m().u.resp_rollover_now.oldpath[tail] = 0;
                bool zipping = logfile.rolloverNow();
                msg.m().u.resp_rollover_now.zipped = zipping;
                tail = 0;
                if (logfile.isOpen())
                {
                    tail = sizeof(msg.m().u.resp_rollover_now.newpath)-1;
                    strncpy(msg.m().u.resp_rollover_now.newpath,
                            logfile.getFilename().c_str(), tail);
                }
                msg.m().u.resp_rollover_now.newpath[tail] = 0;
                if (zipping)
                    remoteResponsePending[filename] =
                        remoteResponseInfo(msg,remote_path);
                else
                    control_sock.send(msg.buf, remote_path);
                break;
            }
            case PFKSCRIPT_CMD_CLOSE_NOW:
            {
                msg.m().type = PFKSCRIPT_RESP_CLOSE_NOW;
                int tail = 0;
                bool zipping = false;
                string filename;
                msg.m().u.resp_rollover_now.newpath[0] = 0;
                if (logfile.isOpen())
                {
                    filename = logfile.getFilename();
                    zipping = logfile.closeNow();
                    tail = sizeof(msg.m().u.resp_rollover_now.oldpath)-1;
                    strncpy(msg.m().u.resp_rollover_now.oldpath,
                            filename.c_str(), tail);
                }
                msg.m().u.resp_rollover_now.zipped = zipping;
                msg.m().u.resp_rollover_now.oldpath[tail] = 0;
                if (zipping)
                    remoteResponsePending[filename] =
                        remoteResponseInfo(msg,remote_path);
                else
                    control_sock.send(msg.buf, remote_path);
                break;
            }
            case PFKSCRIPT_CMD_OPEN_NOW:
            {
                msg.m().type = PFKSCRIPT_RESP_OPEN_NOW;
                logfile.openNow();
                int tail = 0;
                if (logfile.isOpen())
                {
                    tail = sizeof(msg.m().u.resp_get_file_path.path)-1;
                    strncpy(msg.m().u.resp_get_file_path.path,
                            logfile.getFilename().c_str(), tail);
                }
                msg.m().u.resp_get_file_path.path[tail] = 0;
                control_sock.send(msg.buf, remote_path);
                break;
            }
            default:
                cerr << "unknown msg type " << msg.m().type
                     << " received on ctrl sock" << endl;
            }
        }
    }
    void do_logfile_maintenance(void)
    {
        LogFile::FilenameList_t  list;
        logfile.periodic(list);
        uint32_t zipind;
        remotePending_t::iterator it = remoteResponsePending.end();
        for (zipind = 0; zipind < list.size(); zipind++)
        {
            const string &finished = list[zipind];
            size_t dotpos = finished.find_last_of('.');
            if (dotpos == string::npos)
            {
//                cerr << "file " << finished << " has no dot?\r\n";
                break;
            }
            const string &finishedOrig = finished.substr(0,dotpos);
//            cout << "file " << finishedOrig
//                 << " just finished zipping\r\n";
            it = remoteResponsePending.find(finishedOrig);
            if (it != remoteResponsePending.end())
            {
                break;
            }
//            else cout << "NOT FOUND\r\n";
        }
        if (it != remoteResponsePending.end())
        {
            const remoteResponseInfo &info = it->second;
//            cout << "FOUND\r\n";
            control_sock.send(info.msg.buf, info.remote_path);
            remoteResponsePending.erase(it);
        }
    }
    static void sigwinch_handler(int sig)
    {
        if (instance)
        {
            char c = 1;
            instance->winch_pipe.write(&c,1);
        }
    }
    void handle_sigwinch(void)
    {
        char c;
        winch_pipe.read(&c,1);
        set_winsize();
    }
    void set_winsize(void)
    {
        struct winsize sz;
        if (ioctl(0, TIOCGWINSZ, &sz) < 0)
        {
            return;
//            char * errstring = strerror(errno);
//            cerr << "failed to get winsz: " << errstring << endl;
        }
        if (ioctl(master_fd, TIOCSWINSZ, &sz) < 0)
        {
            return;
//            char * errstring = strerror(errno);
//            cerr << "failed to set winsz: " << errstring << endl;
        }
    }
public:
    Pfkscript_program(int argc, char ** argv)
        : opts(argc-1,argv+1),
          logfile(opts)
    {
        listenPortFd = -1;
        listenDataPortFd = -1;
        use_ctrl_sock = false;
        sttyWasRun = false;
        wait_status_collected = false;
        wait_status = 0;
        instance = this;
    }
    ~Pfkscript_program(void)
    {
        instance = NULL;
    }
    int main(void)
    {
        if (opts.isError)
        {
            opts.printHelp();
            return 1;
        }

        if (opts.debug) // debug
            opts.printOptions();

        if (opts.isRemoteCmd)
            return do_remote_cmd();

        logfile.init();

        if (opts.listenPortSpecified)
            if (makeListenPort() == false)
                return 1;

        int slave_fd;
        char ttyname[256];

        if (openpty(&master_fd, &slave_fd, ttyname,
                    NULL, NULL) < 0)
        {
            printError(errno, "openpty");
            return 1;
        }

        block_sigpipe();
        setup_sigchild();

        if (opts.backgroundSpecified)
            daemonize();

        if (control_sock.init())
        {
            use_ctrl_sock = true;
            if (opts.debug)
                printf("setting %s = %s\n",
                       pfkscript_ctrl::env_var_name,
                       control_sock.getPath().c_str());
            setenv(pfkscript_ctrl::env_var_name,
                   control_sock.getPath().c_str(), 1);
        }

        child_pid = fork();
        if (child_pid < 0)
        {
            printError(errno, "fork");
            return 1;
        }
        if (child_pid == 0)
        {
            if (login_tty(slave_fd) < 0)
            {
                printError(errno, "login_tty");
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
            if (opts.debug)
                printf("setting IN_PFKSCRIPT = %s\n",ppid);
            setenv("IN_PFKSCRIPT", ppid, 1);

            execvp(opts.command[0], (char *const*)opts.command.data());

            printError(errno, string("execvp: ") + opts.command[0]);

            _exit(99);
        }

        start_time.getNow();

        // parent
        close(slave_fd);

        pxfe_select  sel;
        bool done = false;

        fcntl(master_fd, F_SETFL, 
              fcntl(master_fd, F_GETFL, 0) | O_NONBLOCK);

        if (isatty(0) && !opts.backgroundSpecified && !opts.noReadSpecified)
        {
            if (setRawMode() == false)
                return -1;
            sttyWasRun = true;
        }

        struct sigaction act;
        act.sa_handler = &Pfkscript_program::sigwinch_handler;
        sigfillset(&act.sa_mask);
        act.sa_flags = SA_RESTART;
        sigaction(SIGWINCH, &act, NULL);

        set_winsize();

        ticker.start(0,150000);
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
            sel.rfds.set(winch_pipe.readEnd);
            sel.tv.set(1,0);

            cc = sel.select();

            if (cc <= 0)
                continue;

            if (sel.rfds.is_set(ticker.fd()))
            {
                ticker.doread();
                do_logfile_maintenance();
            }

            if (sel.rfds.is_set(master_fd))
                done = handle_master_sock();
            if (sel.rfds.is_set(0))
                handle_fd0();
            if (use_ctrl_sock && sel.rfds.is_set(control_sock.getFd()))
                handle_control_sock();
            if (listenPortFd > 0 && sel.rfds.is_set(listenPortFd))
                handle_listen_port();
            if (listenDataPortFd > 0 && sel.rfds.is_set(listenDataPortFd))
                handle_listen_data_port();
            if (sel.rfds.is_set(winch_pipe.readEnd))
                handle_sigwinch();
        }
        ticker.stopjoin();
        stop_time.getNow();

        {
            pxfe_timeval  diffTime = stop_time - start_time;
            uint32_t h,m,s,u;
            char timeBuffer1[64] = {0};
            char timeBuffer2[64] = {0};
            char timeBuffer3[64] = {0};
            std::ostringstream ostr;
            struct rusage usage;

            // NOTE getrusage(children) doesn't do much if we dont
            //  call wait(2).  that's how CPU usage of children get
            //  into the parent's stats.

            int counter = 1000;
            while (counter-- > 0)
            {
                // might be a race in SIGCHILD handler being
                // called, so wait for it.
                if (wait_status_collected == false)
                    usleep(1);
            }
            ostr << "\n\n\n";
            const char * exit_reason = NULL;
            const char * dumped_core = NULL;
            int exit_value = 0;
            if (wait_status_collected)
            {
                if (WIFEXITED(wait_status))
                {
                    exit_reason = "normal exit with status";
                    exit_value = WEXITSTATUS(wait_status);
                }
                if (WIFSIGNALED(wait_status))
                {
                    exit_reason = "exit due to signal";
                    exit_value = WTERMSIG(wait_status);
                }
                if (WCOREDUMP(wait_status))
                    dumped_core = "CHILD PROCESS DUMPED CORE";
            }

            diffTime.breakdown(h,m,s,u);
            snprintf(timeBuffer1,sizeof(timeBuffer1),
                     "   %02u:%02u:%02u.%06u", h, m, s, u);

            pxfe_errno e;
            if (getrusage(RUSAGE_CHILDREN, &usage) < 0)
            {
                e.init(errno, "getrusage");
                ostr << "error: " << e.Format();
            }
            else
            {
                pxfe_timeval *rut = (pxfe_timeval *) &usage.ru_utime;
                rut->breakdown(h,m,s,u);
                snprintf(timeBuffer2,sizeof(timeBuffer2),
                         "   %02u:%02u:%02u.%06u", h, m, s, u);

                rut = (pxfe_timeval *) &usage.ru_stime;
                rut->breakdown(h,m,s,u);
                snprintf(timeBuffer3,sizeof(timeBuffer3),
                         "   %02u:%02u:%02u.%06u", h, m, s, u);
            }

#define F(s,v) \
        << setw(30) << setiosflags(std::ios_base::right) << s << ":" \
        << setw(9)  << setiosflags(std::ios_base::right) << v \
        << "\n"

            if (dumped_core)
                ostr << setw(30) << setiosflags(std::ios_base::right)
                     << dumped_core << "\n";
            if (exit_reason)
                ostr F(exit_reason, exit_value);

            ostr
                F(       "max resident set size",usage.ru_maxrss)
                F(               "page reclaims",usage.ru_minflt)
                F(                 "page faults",usage.ru_majflt)
                F(                       "swaps",usage.ru_nswap)
                F(             "block input ops",usage.ru_inblock)
                F(            "block output ops",usage.ru_oublock)
                F(                "signals rcvd",usage.ru_nsignals)
                F(  "voluntary context switches",usage.ru_nvcsw)
                F("involuntary context switches",usage.ru_nivcsw)
                F(           "elapsed real time",timeBuffer1)
                F(               "user CPU time",timeBuffer2)
                F(             "system CPU time",timeBuffer3)
                ;

#undef F

            logfile.addData(ostr.str());
        }

        if (listenDataPortFd > 0)
            close(listenDataPortFd);
        if (listenPortFd > 0)
            close(listenPortFd);
        if (opts.backgroundSpecified)
            unlink(opts.pidFile.c_str());
        if (sttyWasRun)
            restore_cooked_mode();

        return 0;
    }
};

Pfkscript_program * Pfkscript_program::instance = NULL;

extern "C" int
pfkscript_main(int argc, char ** argv)
{
    Pfkscript_program  program(argc,argv);
    return program.main();
}
