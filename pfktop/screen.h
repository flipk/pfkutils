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
        std::string header_color;
        std::string zero_cmd_color;
        std::string nonzero_cmd_color;
        std::string zero_color;
        std::string nonzero_color;
        std::string normal_color;
        int height(void) const;
        int start_winch(void);
        void stop_winch(void);
    };

};

#endif /* __screen_h__ */
