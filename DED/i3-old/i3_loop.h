/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#ifndef __I3_LOOP_H__
#define __I3_LOOP_H__

#include "i3_options.h"
#include <string>
#include "thread_slinger.h"
#include "posix_fe.h"
#include <mbedtls/sha256.h>
#include "libprotossl.h"
#include I3_PROTO_HDR

class i3protoConn; // forward to avoid circular dependencies

#define I3_EVT_TYPE_LIST                        \
    I3_EVT_TYPE(CONNECT)    /*no arg*/          \
    I3_EVT_TYPE(DISCONNECT) /*no arg*/          \
    I3_EVT_TYPE(READ)       /*read_buffer*/     \
    I3_EVT_TYPE(READ_DONE)  /*no arg*/          \
    I3_EVT_TYPE(RCVMSG)     /*msg*/             \
    I3_EVT_TYPE(DIE)        /*no arg*/

struct i3_evt : ThreadSlinger::thread_slinger_message {
    enum type_e {
#define I3_EVT_TYPE(x) x ,
        I3_EVT_TYPE_LIST
#undef  I3_EVT_TYPE
        NUM_EVTS
    } type;
    static const std::string evt_type_names[NUM_EVTS];
    const std::string &type_name(void) { return evt_type_names[type]; }
    static const std::string &type_name(type_e t) { return evt_type_names[t]; }
    pxfe_string read_buffer;
    PFK::i3::i3Msg * msg;
    i3protoConn * conn;
    void set_connect(i3protoConn * _conn);
    void set_disconnect(i3protoConn * _conn);
    void set_read(std::string &buffer);
    void set_readdone(void);
    void set_rcvmsg(PFK::i3::i3Msg *msg);
    void set_die(void);
private:
    void init(type_e);
};
typedef ThreadSlinger::thread_slinger_queue<i3_evt> i3_evt_q_t;
typedef ThreadSlinger::thread_slinger_pool<i3_evt> i3_evt_pool_t;

class i3_reader; // forward, to avoid circular header dependencies

class i3_loop : pxfe_pthread {
    const i3_options &opts;
    /*virtual*/ void * entry(void *arg);
    /*virtual*/ void send_stop(void);
    i3_evt_q_t  q;
    i3_evt_pool_t  p;
    i3_reader * reader;
    // return true if 'done'
    bool handle_rcvmsg(const PFK::i3::i3Msg *msg);
    i3protoConn * conn;
    uint64_t bytes_sent;
    uint64_t bytes_received;
    mbedtls_sha256_context recv_hash;
    mbedtls_sha256_context send_hash;
public:
    i3_loop(const i3_options &_opts);
    virtual ~i3_loop(void);
    i3_evt * alloc_evt(void) { return p.alloc(-1); }
    void enqueue_evt(i3_evt *evt) { q.enqueue(evt); }
    void set_reader(i3_reader *_reader) { reader = _reader; }
    void start(void);
};

#endif /* __I3_LOOP_H__ */
