/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#ifndef __screen_h__
#define __screen_h__ 1

#include <string>
#include <termios.h>
#include <unistd.h>

namespace pfktop {

    class Screen {
        static Screen * instance;
        static void sigwinch_handler(int);
        int fds[2];
        bool started;
        std::string outbuffer; // references held by cout.
        struct termios old_tios;
    public:
        Screen(void);
        ~Screen(void);
        std::string home;
        std::string erase;
        std::string nl;
        int height(void) const;
        int start_winch(void);
        void stop_winch(void);
    };

};

#endif /* __screen_h__ */
