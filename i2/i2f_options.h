/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#ifndef __I2F_OPTIONS_H__
#define __I2F_OPTIONS_H__

#include <string>
#include <list>
#include "posix_fe.h"

class i2f_options {
    void print_help(void);
public:
    bool ok;
    bool stats_at_end;
    bool verbose;
    int debug_flag;

    struct forw_port {
        uint16_t port;
        std::string remote_host;
        uint32_t remote_addr;
        uint16_t remote_port;
        pxfe_tcp_stream_socket  listen_socket;
    };
    typedef std::list<forw_port*> portlist_t;
    typedef portlist_t::iterator  ports_iter_t;
    portlist_t ports;

    i2f_options(int argc, char ** argv);
    ~i2f_options(void);
    void print(void);
};

#endif /* __I2_OPTIONS_H__ */
