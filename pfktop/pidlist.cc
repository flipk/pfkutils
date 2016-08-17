
#include "pidlist.h"
#include "pfkposix.h"

#include <stdlib.h>
#include <iomanip>
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
    string procDir = "/proc";

    if (!p.open(procDir))
    {
        cout << "cannot open /proc" << nl;
        return;
    }

    // clear the 'stamp' on everything in the db.
    db.unstamp();

    while (p.read(de))
    {
        char * endpp = NULL;
        string de_name(de.d_name);
        unsigned long _pid = strtoul(de.d_name, &endpp, 10);
        if (*endpp != 0)
            // not an integer, must be one of those other words
            // found in /proc.
            continue;

        pid_t pid = (pid_t) _pid;
        // this dirent is a valid pid. let's consume it.
        tidEntry * pe = db.find(pid);
        if (!pe)
        {
            // we've not seen this one before, add an entry
            // for it.
            pe = new tidEntry(pid, pid,
                              procDir + "/" + de_name + "/task/" + de_name);
            db.add(pe);
        }
        // this will set the stamp.
        pe->update();
        pfk_readdir t;
        string taskDir = procDir + "/" + de_name + "/task";
        t.open(taskDir); // ignore return cuz read will ret false anyway
        while (t.read(de))
        {
            char * endpt = NULL;
            unsigned long _tid = strtoul(de.d_name, &endpt, 10);
            if (*endpt != 0)
                // not an integer, must be one of those other words
                // found in /proc.
                continue;

            if (_tid == _pid)
                // its the main thread which we already got, skip.
                continue;

            pid_t tid = (pid_t) _tid;
            tidEntry * te = db.find(tid);
            if (!te)
            {
                // a thread we've not seen before, add an entry for it.
                te = new tidEntry(tid, pid,
                                  taskDir + "/" + de.d_name,
                                  pe);
                db.add(te);
                pe->db.add(te);
            }
            // this will set the stamp.
            te->update();
        }
    }

    // look for every entry with no stamp.
    // these entries represent threads or processes which
    // have exited and need to be cleaned up.
    pidList_t  notStampedList;
    db.find_not_stamped(notStampedList);

    for (pidList_t::iterator it = notStampedList.begin();
         it != notStampedList.end();
         it++)
    {
        tidEntry * te = *it;
        {
            // this entry didn't get updated above, so update it here.
            // basically all this does is right-shift the history,
            // for pretty display.
            te->update();
            // we dont delete the entry until the history has shifted
            // completely off. this way threads that have recently died
            // will still show up for a little while.
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
                        pe->db.remove(te);
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
    height--; // account for header row.
    height--; // account for TOTAL row at bottom

    // build list of things we want to print.
    // basically only things that have some CPU
    // time in the history to print.
    pidList_t::const_iterator lit;
    pidVec_t printList;
    for (lit = db.begin(); lit != db.end(); lit++)
    {
        tidEntry * te = *lit;
        if (te->any_nonzero_history())
            printList.push_back(te);
    }

    // sort the list by priority, highest prio at the top.
    mySorterClass mySorter;
    std::sort(printList.begin(), printList.end(), mySorter);

    bool more = false;
    if (printList.size() > height)
    {
        printList.resize(height);
        more = true;
    }

    // no, there IS no "nl" here. this code prints a nl
    // prior to each entry so the cursor always ends up
    // on the right of the last entry.
    cout
        << "  pid   tid              cmd    "
        << "rss prio  time (10 sec history)       10av";

    int totalCpu = 0;
    pidVec_t::iterator vit;
    for (vit = printList.begin(); vit != printList.end(); vit++)
    {
        tidEntry * te = *vit;
        cout
            << nl
            << setw(5)  << te->pid  << " "
            << setw(5)  << te->tid  << " "
            << setw(16) << te->cmd  << " "
            << setw(6)  << te->rss  << " "
            << setw(4)  << te->prio << "  ";

        // sum and count the CPU history, for the 'average' column.
        int s = 0;
        int c = 0;
        if (te->history.size() > 0)
            if (te->history[0] > 0)
                totalCpu += te->history[0];
        for (int ind = 0; ind < 10; ind++)
        {
            bool skip = false;
            int v = 0;

            if (ind >= te->history.size())
                skip = true;
            if (!skip)
            {
                v = te->history[ind];
                // negative value means we've been
                // updating in the unstamped case
                // and this entry should not be counted
                // in the average.
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
        cout << setw(2) << s; // average
    }

    if (more)
        cout << " MORE";

    cout << nl << "                       TOTAL ";
    cout << "            " << setw(3) << totalCpu;

    cout << erase;
}