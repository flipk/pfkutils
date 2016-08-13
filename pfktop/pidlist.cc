
#include "pidlist.h"
#include "screen.h"
#include "pfkposix.h"

#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <algorithm>

using namespace pfktop;
using namespace std;

PidList :: PidList(const Options &_opts, Screen &_screen)
    : opts(_opts), screen(_screen),
      nl(_screen.nl), erase(_screen.erase), home(_screen.home),
      db(true)
{
}

PidList :: ~PidList(void)
{
}

void
PidList :: fetch(void)
{
    pfk_readdir p;
    dirent de;
    string   path = "/proc";

    if (!p.open(path))
    {
        cout << "cannot open /proc" << nl;
        return;
    }

    db.unstamp();

    while (p.read(de))
    {
        char * endpp = NULL;
        string de_name(de.d_name);
        unsigned long _pid = strtoul(de.d_name, &endpp, 10);
        if (*endpp != 0)
            // not an integer, must be one of those other words
            continue;

        pid_t pid = (pid_t) _pid;
        // this dirent is a valid pid. let's consume it.
        tidEntry * pe = db.find(pid);
        if (!pe)
        {
            pe = new tidEntry(pid, pid, path + "/" + de_name);
            db.add(pe);
        }
        pe->update();
        pfk_readdir t;
        string taskDir = string("/proc/") + de_name + "/task";
        t.open(taskDir); // ignore return cuz read will ret false anyway
        while (t.read(de))
        {
            char * endpt = NULL;
            unsigned long _tid = strtoul(de.d_name, &endpt, 10);
            if (*endpt != 0)
                // not an integer, must be one of those other words
                continue;

            if (_tid == _pid)
                // its the main thread which we already got, skip.
                continue;

            pid_t tid = (pid_t) _tid;
            tidEntry * te = db.find(tid);
            if (!te)
            {
                te = new tidEntry(tid, pid,
                                  taskDir + "/" + string(de.d_name),
                                  pe);
                db.add(te);
                pe->db.add(te);
            }
            te->update();
        }
    }

    pidList_t  notStampedList;
    db.find_not_stamped(notStampedList);

    for (pidList_t::iterator it = notStampedList.begin();
         it != notStampedList.end();
         it++)
    {
        tidEntry * te = *it;
        {
            te->update();
            if (te->any_nonzero_history() == false)
            {
                db.remove(te);
                if (te->pid != te->tid)
                {
                    // the case we're dealing with here is when a thread
                    // exits but the parent process does not -- must make
                    // sure parent.db doesn't point to an object since deleted.
                    tidEntry * pe = db.find(te->pid);
                    // not finding it would be quite normal,
                    // if a process exited and took all threads with it
                    // there would be no parent.db to search.
                    if (pe)
                    {
                        pe->update();
                        if (pe->any_nonzero_history() == false)
                            pe->db.remove(te);
                    }
                }
                delete te;
            }
        }
    }
}

struct mySorterClass {
    bool operator() (const tidEntry * a,
                     const tidEntry * b) {
        // sort first by prio
        if (a->prio < b->prio)
            return true;
        if (a->prio > b->prio)
            return false;
        // break a prio tie by thread id
        if (a->tid < b->tid)
            return true;
        return false;
    }
};

void
PidList :: print(void) const
{
    cout << home;

    int height = screen.height();
    if (height < 0)
    {
        cout << "ERROR" << nl;
        return;
    }

    pidList_t::const_iterator lit;
    pidVec_t printList;

    cout << "  pid   tid              cmd    "
         << "rss prio  time (10 sec history)       10av"
         << nl;
    for (lit = db.begin(); lit != db.end(); lit++)
    {
        tidEntry * te = *lit;
        if (te->any_nonzero_history())
            printList.push_back(te);
    }

    mySorterClass mySorter;
    std::sort(printList.begin(), printList.end(), mySorter);

    pidVec_t::iterator vit;
    for (vit = printList.begin(); vit != printList.end(); vit++)
    {
        tidEntry * te = *vit;
        cout
            << setw(5) << te->pid << " "
            << setw(5) << te->tid << " "
            << setw(16) << te->cmd << " "
            << setw(6) << te->rss << " "
            << setw(4) << te->prio << "  ";
        int s = 0;
        int c = 0;
        for (int ind = 0; ind < 10; ind++)
        {
            bool skip = false;
            int v = 0;

            if (ind >= te->history.size())
                skip = true;
            if (!skip)
            {
                v = te->history[ind];
                if (v < 0)
                    skip = true;
            }
            if (skip)
                cout << "   ";
            else
            {
                s += v;
                cout << setw(2) << v << " ";
                c++;
            }
        }
        if (c > 0)
            s /= c;
        cout << setw(2) << s;
        cout << nl;
    }

    cout << erase;
}
