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

#ifndef __tiddb_h__
#define __tiddb_h__ 1

#include <sys/types.h>
#include <string>
#include <list>
#include <vector>
#include <map>

namespace pfktop {

    struct tidEntry;

    typedef std::list<tidEntry*> pidList_t;
    typedef std::vector<tidEntry*> pidVec_t;
    typedef std::map<pid_t,tidEntry*> pidMap_t;

    struct tidDb {
        bool forPids;
        pidList_t  tidList;
        pidMap_t  tidMap;
        tidDb(bool _forPids);
        ~tidDb(void);
        tidEntry * find(pid_t tid);
        void add(tidEntry * pe);
        void remove(tidEntry *pe);
        void unstamp(void);
        void find_not_stamped(pidList_t &notStampedList);
        pidList_t::const_iterator begin(void) const { return tidList.begin(); }
        pidList_t::const_iterator end(void) const { return tidList.end(); }
    };

    struct tidEntry {
        pid_t tid;
        pid_t pid;
        std::string  pathToDir;
        std::string  cmd;
        std::string  stat_line;
        tidEntry *parent; // null if i am parent
        bool stamp;
        bool first_update;
        char state;
        unsigned long vsz;
        unsigned long rss;
        long prio;
        unsigned long long utime;
        unsigned long long stime;
        unsigned long long utime_prev;
        unsigned long long stime_prev;
        unsigned long long utime_diff;
        unsigned long long stime_diff;
        int diffsum;
        std::vector<int> history;
        tidDb  db; // all threads of a process, ptr shared with pidlist.db
        tidEntry(pid_t tid, pid_t pid,
                 const std::string &path, tidEntry *_parent = NULL);
        ~tidEntry(void);
        void update(void);
        bool any_nonzero_history(void) const {
            for (int ind = 0; ind < history.size(); ind++)
                if (history[ind] > 0)
                    return true;
            return false;
        }
    };

};

#endif /* __tiddb_h__ */
