/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

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
#include <fstream>

using namespace std;

class Pfkscript_program {
    Options opts;
    LogFile logfile;
    int listenPortFd;
    int listenDataPortFd;
    bool use_ctrl_sock;
    pfk_unix_dgram_socket control_sock;
    struct termios old_tios;
    bool sttyWasRun;
    pfk_ticker ticker;
    int master_fd;
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
                (void) write(1, buffer, buflen);
            if (listenDataPortFd != -1)
                (void) write(listenDataPortFd, buffer, buflen);
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
            (void) write(master_fd, buffer, buflen);
        }
    }

    void handle_listen_port(void)
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
        int zipind, remoteind;
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
public:
    Pfkscript_program(int argc, char ** argv)
        : opts(argc-1,argv+1),
          logfile(opts)
    {
        listenPortFd = -1;
        listenDataPortFd = -1;
        use_ctrl_sock = false;
        sttyWasRun = false;
    }
    ~Pfkscript_program(void)
    {
    }
    int main(void)
    {
        if (opts.isError)
        {
            opts.printHelp();
            return 1;
        }

        if (0) // debug
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

        if (opts.backgroundSpecified)
            daemonize();

        if (control_sock.init())
        {
            use_ctrl_sock = true;
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
            setenv("IN_PFKSCRIPT", ppid, 1);

            execvp(opts.command[0], (char *const*)opts.command.data());

            printError(errno, string("execvp: ") + opts.command[0]);

            _exit(99);
        }

        // parent
        close(slave_fd);

        pfk_select  sel;
        bool done = false;

        fcntl(master_fd, F_SETFL, 
              fcntl(master_fd, F_GETFL, 0) | O_NONBLOCK);

        if (isatty(0) && !opts.backgroundSpecified && !opts.noReadSpecified)
        {
            if (setRawMode() == false)
                return -1;
            sttyWasRun = true;
        }

        // TODO forward window size changes to child PTY

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
            sel.tv.set(1,0);

            cc = sel.select();

            if (cc <= 0)
                continue;

            if (sel.rfds.isset(ticker.fd()))
            {
                char c;
                (void) read(ticker.fd(), &c, 1);
                do_logfile_maintenance();
            }

            if (sel.rfds.isset(master_fd));
                done = handle_master_sock();
            if (sel.rfds.isset(0))
                handle_fd0();
            if (use_ctrl_sock && sel.rfds.isset(control_sock.getFd()))
                handle_control_sock();
            if (listenPortFd > 0 && sel.rfds.isset(listenPortFd))
                handle_listen_port();
            if (listenDataPortFd > 0 && sel.rfds.isset(listenDataPortFd))
                handle_listen_data_port();
        }
        ticker.stopjoin();

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

extern "C" int
pfkscript_main(int argc, char ** argv)
{
    Pfkscript_program  program(argc,argv);
    return program.main();
}
