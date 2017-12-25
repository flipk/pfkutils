/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#ifndef __I3_PROTOSSL_CONN_H__
#define __I3_PROTOSSL_CONN_H__

#include "i3_options.h"
#include "i3_loop.h"
#include "libprotossl.h"
#include I3_PROTO_HDR

class i3protoConn : public ProtoSSL::ProtoSSLConn<PFK::i3::i3Msg,
                                                  PFK::i3::i3Msg>
{
    const i3_options &opts;
    i3_loop &loop;
    // the user's handler should return false to kill the connection.
    /*virtual*/ bool messageHandler(const PFK::i3::i3Msg &);
    /*virtual*/ void handleConnect(void);
    /*virtual*/ void handleDisconnect(void);
public:
    i3protoConn(const i3_options &_opts,
                i3_loop &_loop);
    virtual ~i3protoConn(void);
    void send_read_data(const std::string &data);
    void send_read_done(uint64_t file_size, const std::string &sha256_hash);
};

#endif /* __I3_PROTOSSL_CONN_H__ */
