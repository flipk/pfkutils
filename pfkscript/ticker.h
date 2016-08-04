/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#ifndef __ticker_h__
#define __ticker_h__

#include "pfkpthread.h"

class Ticker : pfk_pthread {
    int closer_pipe_fds[2];
    int pipe_fds[2];
    /*virtual*/ void entry(void) {
        char c = 1;
        int clfd = closer_pipe_fds[0];
        pfk_select   sel;
        while (1) {
            sel.tv.set(1,0);
            sel.rfds.zero();
            sel.rfds.set(clfd);
            if (sel.select() <= 0)
            {
                (void) write(pipe_fds[1], &c, 1);
                continue;
            }
            if (sel.rfds.isset(clfd))
            {
                (void) read(clfd, &c, 1);
                break;
            }
        }
    }
    bool running;
public:
    Ticker(void) {
        pipe(pipe_fds);
        pipe(closer_pipe_fds);
        running = false;
    }
    ~Ticker(void) {
        stop();
        close(closer_pipe_fds[0]);
        close(closer_pipe_fds[1]);
        close(pipe_fds[0]);
        close(pipe_fds[1]);
    }
    void start(void) {
        if (running)
            return;
        create();
        running = true;
    }
    void stop(void) {
        if (!running)
            return;
        char c = 1;
        (void) write(closer_pipe_fds[1], &c, 1);
        join();
        running = false;
    }
    int get_fd(void) { return pipe_fds[0]; }
};

#endif /* __ticker_h__ */
