
#include <iostream>
#include <list>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <errno.h>
#include <string.h>

using namespace std;

struct ReniceOptions {
    bool get_int(int &value, const char * s) {
        char * endptr = NULL;
        long v = strtol(s, &endptr, 10);
        if (endptr && *endptr == 0)
        {
            value = (int) v;
            return true; // ok
        }
        return false; // not an integer
    }
    ReniceOptions(int argc, char ** argv) {
        ok = false;
        if (argc < 3)
            return;
        if (get_int(prio, argv[1]) == false)
            return;
        for (int ind = 2; ind < argc; ind++)
        {
            int v;
            if (get_int(v, argv[ind]) == false)
                return;
            pids.push_back(v);
        }
        ok = true;
    }
    void usage(void) {
        cerr << "usage: pfkrenice prio pid [pid..]\n"
             << "   prio:  -99 (hi) to +19 (lo)\n"
             << "       :  -99 is max prio realtime, -2 is min prio realtime\n"
             << "       :  +1  is niced +1, +19 is niced +19\n";
    }
    void print(void) {
        cout << "ok = " << (ok ? "true" : "false") << endl;
        if (ok)
        {
            cout << "prio = " << prio << endl << "pids = ";
            list<int>::iterator it;
            for (it = pids.begin(); it != pids.end(); it++)
            {
                int v = *it;
                cout << v << " ";
            }
            cout << endl;
        }
    }
    bool ok;
    int prio;
    list<int> pids;
};

static void
print_error(const char * what)
{
    int e = errno;
    char * err = strerror(errno);
    cout << what << ": " << err << endl;
}

extern "C" int
pfkrenice_main(int argc, char ** argv)
{
    ReniceOptions  opts(argc, argv);
    if (!opts.ok)
    {
        opts.usage();
        return 1;
    }

//    opts.print();

    list<int>::iterator it;
    struct sched_param par;

    for (it = opts.pids.begin(); it != opts.pids.end(); it++)
    {
        pid_t pid = (pid_t) *it;

        cout << "pid " << pid << ": ";

        int sched = sched_getscheduler(pid);
        if (sched < 0)
        {
            print_error("getsched");
            continue;
        }
        bool getparam = false;
        const char * sched_type = "";
        switch (sched) {
        case SCHED_OTHER: sched_type = "OTHER";                  break;
#ifdef SCHED_IDLE
        case SCHED_IDLE:  sched_type = "IDLE" ;                  break;
#endif
        case SCHED_BATCH: sched_type = "BATCH";                  break;
        case SCHED_FIFO:  sched_type = "FIFO" ; getparam = true; break;
        case SCHED_RR:    sched_type = "RR"   ; getparam = true; break;
        }
        cout << "was sched " << sched_type;
        if (getparam)
        {
            if (sched_getparam(pid, &par) < 0)
            {
                print_error("getparam");
                continue;
            }
            else
                cout << " rtprio " << par.sched_priority + 1;
        }
        else
        {
            errno = 0;
            int prio = getpriority(PRIO_PROCESS, pid);
            if (errno != 0)
            {
                print_error("getpriority");
                continue;
            }
            cout << " prio " << prio;
        }
        cout << ", ";

        if (opts.prio < -2)
            par.sched_priority = -opts.prio - 1;
        else
            par.sched_priority = 0;

        if (opts.prio < -2)
        {
            if (sched_setscheduler(pid, SCHED_FIFO, &par) < 0)
                print_error("setsched FIFO");
            else
                cout << "now FIFO rtprio " << par.sched_priority + 1 << endl;
        }
        else
        {
            if (sched_setscheduler(pid, SCHED_OTHER, &par) < 0)
                print_error("setsched OTHER");
            else
            {
                if (setpriority(PRIO_PROCESS, pid, opts.prio) < 0)
                    print_error("setpriority");
                else
                    cout << "now OTHER prio " << opts.prio << endl;
            }
        }
    }

    return 0;
}



//int setpriority(PRIO_PROCESS, int pid, int prio);
// prio is -20 to +19.

//       pthread_setschedparam(pthread_t thread, int policy,
//                             const struct sched_param *param);
// policy = SCHED_OTHER or SCHED_FIFO
// param.sched_priority
