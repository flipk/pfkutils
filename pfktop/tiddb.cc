
#include "pidlist.h"
#include "screen.h"
#include "pfkposix.h"

#include <iostream>
#include <fstream>
#include <stdlib.h>

using namespace pfktop;
using namespace std;

tidDb :: tidDb( bool _forPids )
{
    forPids = _forPids;
}

tidDb :: ~tidDb( void )
{
    if (forPids)
    {
        pidList_t::iterator it = tidList.begin();
        while (it != tidList.end())
            it = tidList.erase(it);
    }
}

tidEntry *
tidDb :: find(pid_t tid)
{
    pidMap_t::iterator it = tidMap.find(tid);
    if (it == tidMap.end())
        return NULL;
    return it->second;
}

void
tidDb :: add(tidEntry * pe)
{
    tidList.push_back(pe);
    tidMap[pe->tid] = pe;
}

void
tidDb :: remove(tidEntry *pe)
{
    tidList.remove(pe);
    tidMap.erase(pe->tid);
}

void
tidDb :: unstamp(void)
{
    for (pidList_t::iterator it = tidList.begin();
         it != tidList.end();
         it++)
    {
        tidEntry * te = *it;
        te->stamp = false;
    }
}

void
tidDb :: find_not_stamped(pidList_t &notStampedList)
{
    for (pidList_t::iterator it = tidList.begin();
         it != tidList.end();
         it++)
    {
        tidEntry * te = *it;
        if (te->stamp == false)
            notStampedList.push_back(te);
    }
}
