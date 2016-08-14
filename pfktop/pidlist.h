/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#ifndef __pidlist_h__
#define __pidlist_h__ 1

#include <string>
#include "options.h"
#include "screen.h"
#include "tiddb.h"

namespace pfktop {

    class PidList {
        const Options &opts;
        Screen &screen;
        std::string &nl;
        std::string &erase;
        std::string &home;
        tidDb  db; // includes all threads of all processes
    public:
        PidList(const Options &_opts, Screen &screen);
        ~PidList(void);
        void fetch(void);
        void print(void) const;
    };

};

#endif /* __pidlist_h__ */
