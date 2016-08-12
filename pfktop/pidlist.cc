
#include "pidlist.h"
#include "screen.h"
#include "pfkposix.h"

#include <iostream>
#include <fstream>
#include <stdlib.h>

using namespace pfktop;
using namespace std;

PidList :: PidList(const Options &_opts, Screen &_screen)
    : opts(_opts), screen(_screen),
      nl(_screen.nl), erase(_screen.erase),
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
                pe->db.remove(te);
            }
        }
        delete te;
    }
}



void
PidList :: print(void) const
{
    cout << erase;
    int height = screen.height();
    if (height < 0)
    {
        cout << "ERROR" << nl;
        return;
    }

    pidList_t::const_iterator it;

    cout << "pid tid cmd time rss prio" << nl;
    for (it = db.begin(); it != db.end(); it++)
    {
        tidEntry * te = *it;
        if (te->inError)
            // skip
            continue;
        if (te->diffsum > 0)
        {
//            cout << te->stat_line << nl;
            cout
//                << " ****** " 
                << te->pid << " "
                << te->tid << " "
                << te->cmd << " "
                << te->diffsum << " "
                << te->rss << " "
                << te->prio << nl;
        }
    }

}
