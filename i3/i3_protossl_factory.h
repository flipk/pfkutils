/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#ifndef __I3_PROTOSSL_FACTORY_H__
#define __I3_PROTOSSL_FACTORY_H__

#include "i3_options.h"
#include "i3_loop.h"
#include "libprotossl.h"
#include I3_PROTO_HDR

class i3protoFact : public ProtoSSL::ProtoSSLConnFactory
{
    const i3_options &opts;
    i3_loop &loop;
public:
    i3protoFact(const i3_options &_opts,
                i3_loop &loop);
    virtual ~i3protoFact(void);
    /*virtual*/ ProtoSSL::_ProtoSSLConn * newConnection(void);
};

#endif /* __I3_PROTOSSL_FACTORY_H__ */
