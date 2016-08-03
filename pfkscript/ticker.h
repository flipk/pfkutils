/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#ifndef __ticker_h__
#define __ticker_h__

class Ticker {
    int closer_pipe_fds[2];
    int pipe_fds[2];
    pthread_t thr_id;
    static void * entry(void * arg) { ((class Ticker *)arg)->_entry(); }
    void _entry(void) {
        char c = 1;
        int clfd = closer_pipe_fds[0];
        fd_set rfds;
        while (1) {
            struct timeval tv = { 1, 0 };
            FD_ZERO(&rfds);
            FD_SET(clfd, &rfds);
            select(clfd+1, &rfds, NULL, NULL, &tv);
            if (FD_ISSET(clfd, &rfds))
            {
                (void) read(clfd, &c, 1);
                break;
            }
            (void) write(pipe_fds[1], &c, 1);
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
        pthread_attr_t  attr;
        pthread_attr_init(&attr);
        pthread_create(&thr_id, &attr, entry, this);
        pthread_attr_destroy(&attr);
        running = true;
    }
    void stop(void) {
        if (!running)
            return;
        char c = 1;
        (void) write(closer_pipe_fds[1], &c, 1);
        void * dummy = NULL;
        pthread_join(thr_id,&dummy);
        running = false;
    }
    int get_fd(void) { return pipe_fds[0]; }
};

#endif /* __ticker_h__ */
