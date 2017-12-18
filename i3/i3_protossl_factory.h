/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#ifndef __I3_PROTOSSL_FACTORY_H__
#define __I3_PROTOSSL_FACTORY_H__

#include "i3_options.h"
#include "libprotossl.h"
#include I3_PROTO_HDR

class i3protoFact : public ProtoSSL::ProtoSSLConnFactory
{
    const i3_options &opts;
public:
    i3protoFact(const i3_options &_opts);
    virtual ~i3protoFact(void);
    /*virtual*/ ProtoSSL::_ProtoSSLConn * newConnection(void);
};

class i3protoConn : public ProtoSSL::ProtoSSLConn<PFK::i3::i3Msg,
                                                  PFK::i3::i3Msg>
{
    const i3_options &opts;
public:
    i3protoConn(const i3_options &_opts);
    virtual ~i3protoConn(void);
    // the user's handler should return false to kill the connection.
    /*virtual*/ bool messageHandler(const PFK::i3::i3Msg &);
    /*virtual*/ void handleConnect(void);
};

#endif /* __I3_PROTOSSL_FACTORY_H__ */
