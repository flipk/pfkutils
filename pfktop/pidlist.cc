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

#include "pidlist.h"
#include "posix_fe.h"

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
    self = getpid();
}

PidList :: ~PidList(void)
{
}

void
PidList :: fetch(void)
{
    pxfe_readdir p;
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

        if (pid == self)
            // don't display ourselves.
            continue;

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
        pxfe_readdir t;
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
    Options::sort_type type;
    bool reverse;
    mySorterClass(Options::sort_type _type, bool _reverse)
        : type(_type), reverse(_reverse) { }
    bool operator() (const tidEntry * a,
                     const tidEntry * b) {
        switch (type) {

        case Options::SORT_TID:
            if (reverse)
                return (a->tid > b->tid);
            // else
            return (a->tid < b->tid);

        case Options::SORT_PRIO:
            // sort first by prio
            if (reverse)
            {
                if (a->prio > b->prio)
                    return true;
                if (a->prio < b->prio)
                    return false;
            }
            else
            {
                if (a->prio < b->prio)
                    return true;
                if (a->prio > b->prio)
                    return false;
            }
            // break a tie by 10AV
            return (a->avg10s > b->avg10s);

        case Options::SORT_RSS:
            // first by rss
            if (reverse)
            {
                if (a->rss < b->rss)
                    return true;
                if (a->rss > b->rss)
                    return false;
            }
            else
            {
                if (a->rss > b->rss)
                    return true;
                if (a->rss < b->rss)
                    return false;
            }
            // break a tie by 10AV
            return (a->avg10s > b->avg10s);

        case Options::SORT_TIME:
            if (reverse)
            {
                if (a->history[0] < b->history[0])
                    return true;
                if (a->history[0] > b->history[0])
                    return false;
            }
            else
            {
                if (a->history[0] > b->history[0])
                    return true;
                if (a->history[0] < b->history[0])
                    return false;
            }
            // break a tie by 10AV
            return (a->avg10s > b->avg10s);

        case Options::SORT_10AV:
            if (reverse)
            {
                if (a->avg10s < b->avg10s)
                    return true;
                if (a->avg10s > b->avg10s)
                    return false;
            }
            else
            {
                if (a->avg10s > b->avg10s)
                    return true;
                if (a->avg10s < b->avg10s)
                    return false;
            }
            return (a->tid > b->tid);

        case Options::SORT_CMD:
            if (reverse)
            {
                if (a->cmd > b->cmd)
                    return true;
                if (a->cmd < b->cmd)
                    return false;
            }
            else
            {
                if (a->cmd < b->cmd)
                    return true;
                if (a->cmd > b->cmd)
                    return false;
            }
            // break a tie by 10AV
            return (a->avg10s > b->avg10s);
        }
        // shouldn't be reached
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
    mySorterClass mySorter(opts.sort, opts.reverse);
    std::sort(printList.begin(), printList.end(), mySorter);

    bool more = false;
    if ((int) printList.size() > height)
    {
        printList.resize(height);
        more = true;
    }

    // no, there IS no "nl" here. this code prints a nl
    // prior to each entry so the cursor always ends up
    // on the right of the last entry.
    cout << screen.header_color << "     ";
    cout << ((opts.sort == Options::SORT_TID) ? "TID" : "tId");
    cout << "  ";
    cout << ((opts.reverse) ? "(R)-fwd  " : "(R)everse");
    cout << "   ";
    cout << ((opts.sort == Options::SORT_CMD) ? "CMD" : "Cmd");
    cout << "       ";
    cout << ((opts.sort == Options::SORT_RSS) ? "RSS" : "Rss");
    cout << " ";
    cout << ((opts.sort == Options::SORT_PRIO) ? "PRIO" : "Prio");
    cout << "   ";
    cout << ((opts.sort == Options::SORT_TIME) ? "TIME" : "Time");
    cout << " (10 sec history)      ";
    cout << ((opts.sort == Options::SORT_10AV) ? "10AV" : "10av");
    cout << screen.normal_color;

    int totalCpu = 0;
    pidVec_t::iterator vit;
    for (vit = printList.begin(); vit != printList.end(); vit++)
    {
        tidEntry * te = *vit;
        cout
            << nl
            << setw(8)  << te->tid;

        if (te->tid == te->pid)
            cout << "+";
        else
            cout << " ";

        if (te->history[0] > 0)
            cout << screen.nonzero_cmd_color;
        else
            cout << screen.zero_cmd_color;

        cout
            << setw(16) << te->cmd  << " " << screen.normal_color
            << setw(9)  << te->rss  << " "
            << setw(4)  << te->prio << "  ";

        if (te->history.size() > 0)
            if (te->history[0] > 0)
                totalCpu += te->history[0];
        for (int ind = 0; ind < 10; ind++)
        {
            bool skip = false;
            int v = 0;

            if (ind >= (int) te->history.size())
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
                if (v == 0)
                    cout << screen.zero_color;
                else
                    cout << screen.nonzero_color;
                cout << setw(2) << v << " " << screen.normal_color;
            }
        }
        // the +50 is to round up to the next integer.
        cout << setw(2) << ((te->avg10s + 50) / 100); // average
    }

    cout
        << nl
        << screen.header_color
        << " 'h' for help            TOTAL "
        << "                " << setw(3) << totalCpu;

    if (more)
        cout
            << "                   MORE";
    else
        cout
            << "                       ";

    cout
        << screen.normal_color;

    cout << erase;
}
