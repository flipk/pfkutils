
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include "posix_fe.h"

#include <vector>
#include <list>

static void
usage(void)
{
    fprintf(stderr, "usage:\n"
            "    tickler start\n"
            "          start server daemon\n"
            "    tickler add <interval> <path>\n"
            "          add a path to poll on an interval\n"
            "    tickler remove <path>\n"
            "          remove a path to poll\n"
            "    tickler status\n"
            "          print status\n"
            "    tickler stop\n"
            "          stop existing tickler daemon\n");
}

static bool keep_running;

static void sig_hand(int s)
{
    keep_running = false;
}

struct tickler_thread : public pxfe_pthread
{
    int fd;
    bool _ok;
    bool go;
    std::string tp;
    pxfe_pipe closer_pipe;
public:
    std::string tickle_path;
    int interval;
    int count;

    tickler_thread(const std::string &_tickle_path, int _interval)
        : tickle_path(_tickle_path), interval(_interval)
    {
        fd = -1;
        _ok = go = false;

        tp = tickle_path + "/.tickle_path";
        unlink(tp.c_str());
        fd = open(tp.c_str(), O_RDWR | O_CREAT, 0600);
        if (fd < 0)
        {
            int e = errno;
            error_string << "ERROR: open " << tickle_path
                         << ": " << e << " ("
                         << strerror(e) << ")";
        }
        else
        {
            _ok = true;
            go = true;
        }
    }
    ~tickler_thread(void)
    {
        if (fd > 0)
            close(fd);
        unlink(tp.c_str());
    }
    bool ok(void) const { return _ok; }
    std::ostringstream error_string;
private:
    /*virtual*/ void * entry(void *arg)
    {
        pxfe_timeval tv;

        count = 0;
        while (go)
        {
            tv.getNow();
            std::string now = tv.Format();
            write(fd, now.c_str(), now.size());
            write(fd, "\n", 1);

            count ++;
            if (count > 1000)
            {
                count = 0;
                lseek(fd, 0, SEEK_SET);
            }

            pxfe_select  sel;
            sel.rfds.set(closer_pipe.readEnd);
            sel.tv.set(interval, 0);
            sel.select();
        }

        return NULL;
    }
    /*virtual*/ void send_stop(void)
    {
        go = false;
        closer_pipe.write(" ");
    }
};

static void * _tickler_entry(void *);

static bool init_socket(pxfe_unix_dgram_socket  &control_sock,
                        const char *sock_path)
{
    pxfe_errno e;

    if (!control_sock.init(&e))
    {
        fprintf(stderr, "initializing socket: %s\n", e.Format().c_str());
        return false;
    }
    if (!control_sock.connect(sock_path, &e))
    {
        fprintf(stderr, "connect socket: %s\n", e.Format().c_str());
        if (e.e == ECONNREFUSED)
            // the socket exists but the process does not exist
            // behind it. delete the socket.
            unlink(sock_path);
        return false;
    }

    return true;
}

static inline void splitString(std::vector<std::string> &out,
                               const std::string &line )
{
    out.clear();
    size_t pos;
    for (pos = 0; ;)
    {
        size_t found = line.find_first_of(" ",pos);
        if (found == std::string::npos)
        {
            if (pos != line.size())
                out.push_back(line.substr(pos,line.size()-pos));
            break;
        }
        if (found > pos)
            out.push_back(line.substr(pos,found-pos));
        pos = found+1;
    }
}

extern "C"
int tickler_main(int argc, char ** argv)
{
    char * sock_path;
    pxfe_errno e;
    pxfe_unix_dgram_socket  control_sock;
    std::string msg;
    std::string remote_path;
    std::vector<std::string>  args;
    std::list<tickler_thread*>  ticklers;
    std::list<tickler_thread*>::iterator  tickler_it;
    struct sigaction act;
    act.sa_handler = SIG_IGN;

    sock_path = getenv("TICKLER_SOCK");
    if (sock_path == NULL)
    {
        fprintf(stderr, "please set TICKLER_SOCK\n");
        usage();
        return 1;
    }

    sigaction(SIGPIPE, &act, NULL); // IGNORE

    for (int ind = 1; ind < argc; ind++)
        args.push_back(argv[ind]);

    if (args.size() == 1 && args[0] == "start")
    {
        printf("doing start\n");

        if (!control_sock.init(sock_path, &e))
        {
            fprintf(stderr, "initializing socket: %s\n", e.Format().c_str());
            return 1;
        }

        daemon(0,0);

        act.sa_handler = &sig_hand;
        sigfillset(&act.sa_mask);
        act.sa_flags = 0;
        sigaction(SIGHUP, &act, NULL);
        sigaction(SIGINT, &act, NULL);
        sigaction(SIGQUIT, &act, NULL);
        sigaction(SIGTERM, &act, NULL);

        keep_running = true;
        while (keep_running)
        {
            std::vector<std::string>  cmd_args;

            if (!control_sock.recv(msg, remote_path, &e))
            {
                fprintf(stderr, "socket recv: %s\n", e.Format().c_str());
                break;
            }
            splitString(cmd_args, msg);
#if 0
            printf("command: ", msg.c_str());
            for (auto &a : cmd_args)
                printf(" '%s'", a.c_str());
            printf("\n");
#endif
            if (cmd_args[0] == "add")
            {
                tickler_thread * t = new tickler_thread(
                    cmd_args[2], atoi(cmd_args[1].c_str()));
                if (t->ok())
                {
                    t->create();
                    ticklers.push_back(t);
                    if (!control_sock.send("started", remote_path, &e))
                    {
                        fprintf(stderr, "send stop: %s\n", e.Format().c_str());
                        return 1;
                    }
                }
                else
                {
                    bool ret = control_sock.send(t->error_string.str(),
                                                 remote_path, &e);
                    delete t;
                    if (!ret)
                    {
                        fprintf(stderr, "send stop: %s\n", e.Format().c_str());
                        return 1;
                    }
                }
            }
            else if (cmd_args[0] == "remove")
            {
                bool found = false;
                tickler_thread * t = NULL;
                for (tickler_it = ticklers.begin();
                     tickler_it != ticklers.end();
                     tickler_it++)
                {
                    t = *tickler_it;
                    if (t->tickle_path == cmd_args[1])
                    {
                        found = true;
                        break;
                    }
                }
                if (found)
                {
                    ticklers.erase(tickler_it);
                    t->stopjoin();
                    delete t;
                    if (!control_sock.send("removed", remote_path, &e))
                    {
                        fprintf(stderr, "send stop: %s\n", e.Format().c_str());
                        return 1;
                    }
                }
                else
                {
                    if (!control_sock.send("not found", remote_path, &e))
                    {
                        fprintf(stderr, "send stop: %s\n", e.Format().c_str());
                        return 1;
                    }
                }
            }
            else if (cmd_args[0] == "status")
            {
                std::ostringstream  str;
                for (auto t : ticklers)
                    str << "interval " << t->interval
                        << " path " << t->tickle_path
                        << " count " << t->count
                        << "\n";
                if (!control_sock.send(str.str(), remote_path, &e))
                {
                    fprintf(stderr, "send stop: %s\n", e.Format().c_str());
                    return 1;
                }
            }
            else if (cmd_args[0] == "stop")
            {
                // response not sent until threads are killed.
                for (auto t : ticklers)
                {
                    t->stopjoin();
                    delete t;
                }
                ticklers.clear();

                if (!control_sock.send("stopped", remote_path, &e))
                {
                    fprintf(stderr, "send stop: %s\n", e.Format().c_str());
                    return 1;
                }
                keep_running = false;
            }
        }
        unlink(sock_path);
    }
    else if (args.size() == 3 && args[0] == "add")
    {
        if (!init_socket(control_sock, sock_path))
            return 1;

        std::string cmd = "add " + args[1] + " " + args[2];

        if (!control_sock.send(cmd, &e))
        {
            fprintf(stderr, "send stop: %s\n", e.Format().c_str());
            return 1;
        }

        printf("sent add command; waiting for response\n");
        if (!control_sock.recv(msg, &e))
        {
            fprintf(stderr, "recv: %s\n", e.Format().c_str());
            return 1;
        }
        printf("response:\n%s\n", msg.c_str());
    }
    else if (args.size() == 2 && args[0] == "remove")
    {
        if (!init_socket(control_sock, sock_path))
            return 1;

        std::string cmd = "remove " + args[1];

        if (!control_sock.send(cmd, &e))
        {
            fprintf(stderr, "send stop: %s\n", e.Format().c_str());
            return 1;
        }

        printf("sent remove command; waiting for response\n");
        if (!control_sock.recv(msg, &e))
        {
            fprintf(stderr, "recv: %s\n", e.Format().c_str());
            return 1;
        }
        printf("response:\n%s\n", msg.c_str());
    }
    else if (args.size() == 1 && args[0] == "status")
    {
        if (!init_socket(control_sock, sock_path))
            return 1;

        if (!control_sock.send("status", &e))
        {
            fprintf(stderr, "send stop: %s\n", e.Format().c_str());
            return 1;
        }

        printf("sent status command; waiting for response\n");
        if (!control_sock.recv(msg, &e))
        {
            fprintf(stderr, "recv: %s\n", e.Format().c_str());
            return 1;
        }
        printf("response:\n%s\n", msg.c_str());
    }
    else if (args.size() == 1 && args[0] == "stop")
    {
        if (!init_socket(control_sock, sock_path))
            return 1;

        if (!control_sock.send("stop", &e))
        {
            fprintf(stderr, "send stop: %s\n", e.Format().c_str());
            return 1;
        }

        printf("sent stop command; waiting for response\n");
        if (!control_sock.recv(msg, &e))
        {
            fprintf(stderr, "recv: %s\n", e.Format().c_str());
            return 1;
        }
        printf("response: %s\n", msg.c_str());
    }
    else
    {
        usage();
        return 1;
    }

    return 0;
}
