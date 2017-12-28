/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#ifndef __I3_READER_H__
#define __I3_READER_H__

#include "i3_options.h"
#include "i3_loop.h"
#include "posix_fe.h"

class i3_reader : pxfe_pthread {
    const i3_options &opts;
    i3_loop &loop;
    /*virtual*/ void * entry(void *arg);
    /*virtual*/ void send_stop(void);
    pxfe_pipe  exitPipe;
public:
    i3_reader(const i3_options &_opts, i3_loop &_loop);
    virtual ~i3_reader(void);
    void start(void);
};

#endif /* __I3_READER_H__ */
